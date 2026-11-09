// pchat.probe.asked -- What a question carries to the wire: the reader's own
// settings whole in the body of the request the page makes -- the five
// numbers, the thinking where the model takes one, and the tools left on --
// read off the request itself, which is the one place all of them are said
// together. The page cannot say what it wrote; the request can.
//
// The answer is made here, in the frames the wire this sample asks through
// writes them (an anthropic one -- the opened conversation answers through
// haiku), so that nothing waits on a server to see what went out. What is
// asked for is checked against what the settings rows say, and the probe ends
// with a code that says whether all of them held.
import { chromium } from 'playwright';

const url = `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`;

// One whole answer as anthropic writes it: a message that begins, a text
// block, and the message said over with what it cost.
const answer = [
  'data: {"type":"message_start","message":{"id":"msg_1","type":"message","role":"assistant","model":"haiku-4","usage":{"input_tokens":7}}}',
  '',
  'event: content_block_start',
  'data: {"type":"content_block_start","index":0,"content_block":{"type":"text","text":""}}',
  '',
  'data: {"type":"content_block_delta","index":0,"delta":{"type":"text_delta","text":"answered-by-wire"}}',
  '',
  'data: {"type":"content_block_stop","index":0}',
  '',
  'data: {"type":"message_delta","delta":{"stop_reason":"end_turn"},"usage":{"output_tokens":5}}',
  '',
  'data: {"type":"message_stop"}',
  '',
].join('\n');

const checks = [];
const that = (holds, said) => checks.push(`${holds ? 'ok  ' : 'FAIL'} ${said}`);

const browser = await chromium.launch({
  args: [
    '--no-sandbox',
    '--disable-dev-shm-usage',
    '--use-gl=angle',
    '--use-angle=swiftshader',
    '--enable-unsafe-swiftshader',
    '--js-flags=--max-old-space-size=3072',
  ],
});
const context = await browser.newContext({ viewport: { width: 1280, height: 900 } });
const page = await context.newPage();
page.on('pageerror', error => console.log(`pageerror: ${String(error).slice(0, 400)}`));
let asked_url = null;
let asked_body = null;
await context.route(() => true, async route => {
  const request = route.request();
  if (request.method() !== 'POST' || !request.url().includes('/v1/'))
    return route.continue();
  asked_url = request.url();
  asked_body = request.postData() ?? '';
  return route.fulfill({
    status: 200,
    headers: { 'Content-Type': 'text/event-stream', 'Access-Control-Allow-Origin': '*' },
    body: answer,
  });
});

await page.goto(url, { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(4000);
// Said into the composer the way a reader says something: the window is one
// canvas, so where the field is is put to the bottom of it by pointer.
await page.mouse.click(640, 855);
await page.keyboard.type('Say something, anything.');
await page.keyboard.press('Enter');
await page.waitForTimeout(12000);

console.log(`asked: ${asked_url ?? 'nothing was asked'}`);
if (asked_body === null) {
  console.log('FAIL no request left the page at all');
  await browser.close();
  process.exit(1);
}
console.log(`body: ${asked_body.replace(/\s+/g, ' ').slice(0, 1600)}`);
let body = null;
try {
  body = JSON.parse(asked_body);
} catch (error) {
  console.log(`FAIL the body is not JSON: ${String(error).slice(0, 200)}`);
  await browser.close();
  process.exit(1);
}

// What the settings rows say, checked against what went out: the conversation
// opens on haiku, so this is an anthropic body -- thinking beside a budget,
// sampling taken off beside it, and the numbers the generation rows move.
that(body.model === 'haiku-4', `asked of haiku (said ${body.model})`);
that(!!body.thinking, 'thinking is asked for');
if (body.thinking) {
  that(body.thinking.type === 'enabled', `thinking is enabled (said ${body.thinking.type})`);
  that((body.thinking.budget_tokens ?? 0) >= 1024,
       `thinking carries a budget of at least 1024 (said ${body.thinking.budget_tokens})`);
  that(body.temperature === undefined,
       `temperature is not said beside a thinking (said ${body.temperature})`);
  that((body.max_tokens ?? 0) > (body.thinking.budget_tokens ?? 0),
       `max_tokens holds the budget and the answer (said ${body.max_tokens})`);
}
that(body.top_p === 1.0, `top_p is the 1.0 a thinking still allows (said ${body.top_p})`);
that(Array.isArray(body.tools) && body.tools.length === 3,
     `the three tools are offered (said ${JSON.stringify((body.tools ?? []).map(one => one.name))})`);
that(typeof body.system === 'string' && body.system.length > 0, 'what it is told goes with it');
that(Array.isArray(body.messages) && body.messages.length > 0,
     `the conversation goes with it (${(body.messages ?? []).length} messages)`);

for (const line of checks) console.log(line);
const failed = checks.filter(line => line.startsWith('FAIL')).length;
console.log(failed === 0 ? `\n${checks.length} checks held.` : `\n${failed} of ${checks.length} checks did not hold.`);
await browser.close();
process.exit(failed === 0 ? 0 : 1);
