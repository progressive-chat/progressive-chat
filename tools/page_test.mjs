// SPDX-License-Identifier: AGPL-3.0-only
// End-to-end test of the web build.
//
// Serves the built page and a fake model API from one local port, drives the
// page in a headless browser, saves screenshots, and exits 1 if a required
// check fails. The fake API answers the three request formats the program
// sends: OpenAI chat completions, Anthropic messages and Gemini.
//
//   node tools/page_test.mjs <page-dir> [<shots-dir>]
import { chromium } from 'playwright';
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';

const page_dir = path.resolve(process.argv[2] ?? 'build-web/web');
const shots_dir = path.resolve(process.argv[3] ?? 'page-test');
fs.mkdirSync(shots_dir, { recursive: true });

// Each answer of the fake model carries one word that is unique to its
// request, so a check can look for exactly that answer. A single word stays
// whole even if the page splits a text into several elements.
const mark_of = number => 'quokka' + number + 'x';

const mime = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.json': 'application/json',
  '.wasm': 'application/wasm',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.css': 'text/css; charset=utf-8',
  '.ttf': 'font/ttf',
  '.woff2': 'font/woff2',
};

// Every request the page made to the fake API, oldest first.
const asked = [];

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

// The answer text cut into three pieces, sent with a pause between them.
const pieces_of = text => {
  const third = Math.ceil(text.length / 3);
  return [text.slice(0, third), text.slice(third, 2 * third), text.slice(2 * third)];
};

const openai_frames = text => [
  ...pieces_of(text).map(piece => 'data: ' + JSON.stringify({
    id: 'chatcmpl-test', object: 'chat.completion.chunk',
    choices: [{ index: 0, delta: { role: 'assistant', content: piece }, finish_reason: null }],
  }) + '\n\n'),
  'data: ' + JSON.stringify({
    id: 'chatcmpl-test', object: 'chat.completion.chunk',
    choices: [{ index: 0, delta: {}, finish_reason: 'stop' }],
    usage: { prompt_tokens: 12, completion_tokens: 9, total_tokens: 21 },
  }) + '\n\n',
  'data: [DONE]\n\n',
];

const anthropic_event = (type, data) =>
  'event: ' + type + '\ndata: ' + JSON.stringify({ type, ...data }) + '\n\n';

const anthropic_frames = (text, model) => [
  anthropic_event('message_start', {
    message: {
      id: 'msg_test', type: 'message', role: 'assistant', model, content: [],
      stop_reason: null, stop_sequence: null, usage: { input_tokens: 12, output_tokens: 1 },
    },
  }),
  anthropic_event('content_block_start', { index: 0, content_block: { type: 'text', text: '' } }),
  ...pieces_of(text).map(piece =>
    anthropic_event('content_block_delta', { index: 0, delta: { type: 'text_delta', text: piece } })),
  anthropic_event('content_block_stop', { index: 0 }),
  anthropic_event('message_delta', {
    delta: { stop_reason: 'end_turn', stop_sequence: null }, usage: { output_tokens: 9 },
  }),
  anthropic_event('message_stop', {}),
];

const gemini_frames = text => {
  const pieces = pieces_of(text);
  return pieces.map((piece, at) => 'data: ' + JSON.stringify({
    candidates: [{
      content: { role: 'model', parts: [{ text: piece }] }, index: 0,
      ...(at === pieces.length - 1 ? { finishReason: 'STOP' } : {}),
    }],
    ...(at === pieces.length - 1
      ? { usageMetadata: { promptTokenCount: 12, candidatesTokenCount: 9, totalTokenCount: 21 } }
      : {}),
  }) + '\n\n');
};

const answer_model = async (request, response, url) => {
  const chunks = [];
  for await (const chunk of request) chunks.push(chunk);
  const body = Buffer.concat(chunks).toString('utf8');
  let parsed = null;
  try { parsed = JSON.parse(body); } catch { /* kept as text */ }
  const number = asked.length + 1;
  const mark = mark_of(number);
  asked.push({ number, mark, path: url.pathname + url.search, headers: request.headers, body, parsed });
  const text = 'The fake model answers request ' + number + ' with the word ' + mark + ' in it.';
  let frames;
  if (url.pathname.endsWith('/messages')) frames = anthropic_frames(text, parsed?.model ?? 'unknown');
  else if (url.pathname.includes(':streamGenerateContent')) frames = gemini_frames(text);
  else frames = openai_frames(text);
  response.writeHead(200, {
    'Content-Type': 'text/event-stream', 'Cache-Control': 'no-cache', 'Connection': 'close',
  });
  for (const frame of frames) {
    response.write(frame);
    await sleep(120);
  }
  response.end();
};

const serve_file = (response, url) => {
  const wanted = decodeURIComponent(url.pathname === '/' ? '/index.html' : url.pathname);
  const file = path.join(page_dir, wanted);
  if (!file.startsWith(page_dir + path.sep) || !fs.existsSync(file) || !fs.statSync(file).isFile()) {
    response.writeHead(404, { 'Content-Type': 'text/plain' });
    response.end('not found');
    return;
  }
  response.writeHead(200, {
    'Content-Type': mime[path.extname(file)] ?? 'application/octet-stream',
    'Cache-Control': 'no-store',
  });
  fs.createReadStream(file).pipe(response);
};

const server = http.createServer((request, response) => {
  const url = new URL(request.url, 'http://localhost');
  if (url.pathname.startsWith('/v1/') || url.pathname.startsWith('/v1beta/')) {
    if (request.method === 'POST') {
      answer_model(request, response, url).catch(error => {
        console.log('fake api error: ' + error);
        response.destroy();
      });
      return;
    }
    response.writeHead(405, { 'Content-Type': 'text/plain' });
    response.end('method not allowed');
    return;
  }
  serve_file(response, url);
});
await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
const origin = 'http://localhost:' + server.address().port;
console.log('serving ' + page_dir + ' at ' + origin);

// Results: one line per check. A check with required=false is reported but
// does not fail the run; it marks a known gap that a later change closes.
const results = [];
const check = (name, held, required, detail = '') => {
  results.push({ name, held, required, detail });
  console.log((held ? 'ok    ' : required ? 'FAIL  ' : 'gap   ') + name + (detail ? ' -- ' + detail : ''));
};

const browser = await chromium.launch({
  args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-gl=angle', '--use-angle=swiftshader',
         '--enable-unsafe-swiftshader'],
});

const page_text = page => page.evaluate(() => {
  const host = document.getElementById('skiff');
  return (host && (host.innerText || host.textContent)) || '';
});

const shoot = async (page, name) => {
  try { await page.screenshot({ path: path.join(shots_dir, name + '.png') }); }
  catch (error) { console.log('no screenshot ' + name + ': ' + String(error).slice(0, 200)); }
};

// Opens the page in a new browser context and waits until the program runs.
const open_page = async (name, viewport, phone) => {
  const context = await browser.newContext({
    viewport, deviceScaleFactor: phone ? 2 : 1, isMobile: phone, hasTouch: phone,
  });
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', error => errors.push(String(error).slice(0, 400)));
  page.on('console', message => {
    if (message.type() === 'error') console.log('[' + name + '] console error: ' + message.text().slice(0, 300));
  });
  await page.goto(origin + '/', { waitUntil: 'load' });
  let started = true;
  try { await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 }); }
  catch { started = false; }
  await page.waitForTimeout(4000);
  return { context, page, errors, started };
};

// Puts the keyboard focus into the message field. Tries the field's
// placeholder first and falls back to a point near the bottom of the window.
const focus_field = async (page, viewport, phone) => {
  const by_text = page.getByText('Message', { exact: true }).first();
  let box = null;
  try { box = await by_text.boundingBox({ timeout: 3000 }); } catch { box = null; }
  const x = box ? box.x + box.width / 2 : viewport.width / 2;
  const y = box ? box.y + box.height / 2 : viewport.height - 45;
  if (phone) await page.touchscreen.tap(x, y);
  else await page.mouse.click(x, y);
  await page.waitForTimeout(800);
  return box ? 'placeholder' : 'fallback point';
};

// True if `text` can be read in the message list without scrolling: some
// element holding it lies wholly inside the window, is not covered by another
// element, and is not inside the docked conversation list. From 960 points of
// width the conversation list is docked at the left, 360 points wide
// (src/ui/base.cc); a match there is that list's preview line.
const in_view = async (page, viewport, text) => {
  const list_right = viewport.width >= 960 ? 360 : 0;
  return page.evaluate(([wanted, left, width, height]) => {
    const host = document.getElementById('skiff');
    if (!host) return false;
    for (const element of host.querySelectorAll('*')) {
      const own = Array.from(element.childNodes)
        .filter(node => node.nodeType === Node.TEXT_NODE).map(node => node.textContent).join('');
      if (!own.includes(wanted)) continue;
      const box = element.getBoundingClientRect();
      if (box.width === 0 || box.height === 0) continue;
      if (box.left < left || box.top < 0 || box.right > width || box.bottom > height) continue;
      const top = document.elementFromPoint(box.left + box.width / 2, box.top + box.height / 2);
      if (top && (top === element || element.contains(top) || top.contains(element))) return true;
    }
    return false;
  }, [text, list_right, viewport.width, viewport.height]);
};

// Types a message, sends it with Enter, and checks that the fake API was asked
// and that its answer shows on the page.
const send_and_expect = async (name, viewport, phone, required) => {
  const { context, page, errors, started } = await open_page(name, viewport, phone);
  await shoot(page, name + '-0-start');
  check(name + ': the program starts', started, true);
  if (!started) { await context.close(); return; }
  const before = asked.length;
  const typed_mark = 'pagetest' + name + 'x';
  const typed = 'A message from the page test with the word ' + typed_mark + ' in it';
  const how = await focus_field(page, viewport, phone);
  await page.keyboard.type(typed);
  await page.waitForTimeout(500);
  await shoot(page, name + '-1-typed');
  await page.keyboard.press('Enter');
  let request = null;
  for (let waited = 0; waited < 15000 && !request; waited += 250) {
    request = asked.slice(before).find(one => one.body.includes(typed_mark)) ?? null;
    if (!request) await sleep(250);
  }
  check(name + ': sending asks the model API', request !== null, required,
        request ? 'POST ' + request.path + ', field found by ' + how
                : asked.length - before + ' requests, none with the typed text; field found by ' + how);
  let shown = false;
  if (request) {
    const expected = request.mark;
    for (let waited = 0; waited < 30000 && !shown; waited += 500) {
      shown = (await page_text(page)).includes(expected);
      if (!shown) await page.waitForTimeout(500);
    }
    check(name + ': the answer is in the page', shown, required, 'expected "' + expected + '"');
    // Known gap: the message list does not scroll to a new message, so the
    // text is in the page but below the visible part of the list.
    check(name + ': the sent message is visible without scrolling',
          await in_view(page, viewport, typed_mark), false);
    check(name + ': the answer is visible without scrolling',
          shown && await in_view(page, viewport, expected), false);
  }
  await shoot(page, name + '-2-answered');
  check(name + ': no uncaught page error', errors.length === 0, required, errors.join(' | '));
  if (!shown) console.log('[' + name + '] page text ends: ' + (await page_text(page)).slice(-600).replace(/\n/g, ' | '));
  await context.close();
};

await send_and_expect('wide', { width: 1280, height: 900 }, false, true);
await send_and_expect('phone', { width: 390, height: 844 }, true, true);

await browser.close();
server.close();

console.log('\nrequests the page made to the fake API: ' + asked.length);
for (const one of asked) {
  const keys = Object.keys(one.headers).filter(key =>
    ['authorization', 'x-api-key', 'x-goog-api-key', 'anthropic-version', 'content-type'].includes(key));
  console.log('  ' + one.number + '. POST ' + one.path + ' model=' + (one.parsed?.model ?? '?') +
              ' headers=' + keys.join(',') + ' bytes=' + one.body.length);
}
const failed = results.filter(one => !one.held && one.required);
const gaps = results.filter(one => !one.held && !one.required);
console.log('\n' + results.length + ' checks, ' + failed.length + ' failed, ' + gaps.length + ' known gaps');
process.exit(failed.length ? 1 : 0);
