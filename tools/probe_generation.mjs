// pchat.probe.generation -- What the Generation row says under Responses,
// and whether a press either side of one of the five moves it: the settings
// opened, the five read back out of the text mirror, and then each row found
// by pressing for it -- where a row stands in the column is not what a probe
// is told, so it is found by what answering to a press looks like.
import { chromium } from 'playwright';

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
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto('https://progressive-chat.github.io/progressive-chat/', { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(6000);
await page.waitForTimeout(1500);
const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
// What the page says of the five: every line that is one of them, as the
// mirror keeps its own line for each row of the column.
const names = ['Temperature', 'Top P', 'Maximum tokens', 'Presence penalty',
  'Frequency penalty'];
const five = async () => {
  const said = (await mirror()).split('\n').map((one) => one.trim()).filter(Boolean);
  return names.map((name) => {
    const at = said.indexOf(name);
    return `${name} = ${at < 0 ? 'not said' : (said[at + 1] ?? 'nothing under it')}`;
  });
};
// The settings plate, opened by the word "Settings" itself, read where it
// stands rather than guessed at: the square this was once pressed at is one
// the word left long ago.
const entry = await page.evaluate(() => {
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const own = [...one.childNodes].map((n) => (n.nodeType === 3 ? n.textContent : ''))
        .join(' ').replace(/\s+/g, ' ').trim();
    if (own !== 'Settings') continue;
    const b = one.getBoundingClientRect();
    if (b.width < 1) continue;
    return [b.x + b.width / 2, b.y + b.height / 2];
  }
  return null;
});
if (!entry) { console.log('nothing says Settings on the page'); await browser.close(); process.exit(1); }
await page.mouse.move(entry[0], entry[1]);
await page.waitForTimeout(250);
await page.mouse.click(entry[0], entry[1]);
await page.waitForTimeout(1200);
console.log(`said: ${JSON.stringify(await five())}`);
// Where each of the five rows stands, said by itself: the leaf that says its
// name, and the row around it, which is the widest thing that holds it. Then
// into the room -- a row below the fold is said nowhere at any breadth, so it
// is wheeled up until it stands between the room's own edges.
const rows_of = () => page.evaluate((names) => {
  const found = {};
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const words = (one.textContent || '').trim();
    if (one.children.length !== 0 || !names.includes(words))
      continue;
    let row = one.parentElement;
    while (row && row.getBoundingClientRect().width < 300)
      row = row.parentElement;
    if (!row)
      continue;
    const box = row.getBoundingClientRect();
    found[words] = {x: Math.round(box.x), width: Math.round(box.width),
                    y: Math.round(box.y + box.height / 2.0)};
  }
  return found;
}, names);

const found = [];
for (const name of names) {
  let rows = await rows_of();
  for (let tried = 0; tried < 25 && rows[name] &&
                     (rows[name].y < 120 || rows[name].y > 780); ++tried) {
    await page.mouse.move(640, 450);
    await page.mouse.wheel(0, rows[name].y - 500);
    await page.waitForTimeout(400);
    rows = await rows_of();
  }
  if (!rows[name] || rows[name].y < 120 || rows[name].y > 780) {
    console.log(`${name} was not wheeled into the room`);
    continue;
  }
  const before = await five();
  // Pressed twice over the breadth of its own row -- the value stands at the
  // row's far side, and what answers there is whichever part of the row was
  // reached, said by what the five then say.
  // Sixty in raises it; eighty back from the row's far end lowers it, which
  // is the only way one that stands at its own ceiling can be moved at all.
  for (const along of [60, rows[name].width - 80, rows[name].width - 30,
                       rows[name].width - 60]) {
    const x = rows[name].x + along;
    await page.mouse.move(x, rows[name].y);
    await page.waitForTimeout(250);
    await page.mouse.click(x, rows[name].y);
    await page.waitForTimeout(500);
    const after = await five();
    const which = after.findIndex((one, at) => one !== before[at]);
    if (which < 0)
      continue;
    found.push(`${names[which]} at ${x},${rows[name].y}: ` +
               `${before[which]} -> ${after[which]}`);
    break;
  }
}

// Five things are said, and whether a press moves them is what comes next:
// the rows stand below the fold of the card, and nothing here has reached
// them yet, so an empty answer below is not the page saying no.
console.log(`said and moved by a press: ${found.length ? JSON.stringify(found) : 'no row of the five answers a press in its own row'}`);
console.log(`after: ${JSON.stringify(await five())}`);
await page.screenshot({ path: 'generation.png' }).catch(() => {});
await browser.close();
