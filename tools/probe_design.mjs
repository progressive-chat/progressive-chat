// pchat.probe.design -- What the design said in the room reads like: the
// page opened, the settings opened on it, the row said Read the design
// pressed, and every word of the column read back out of the text mirror.
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
const context = await browser.newContext({ viewport: { width: 1280, height: 900 } });
const page = await context.newPage();
page.on('console', (m) => console.log(`page: ${m.text()}`));
page.on('pageerror', (e) => console.log(`pageerror: ${e.message}`));
await page.goto('https://progressive-chat.github.io/progressive-chat/', { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });

const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
console.log(`chat: ${(await mirror()).length} chars`);

// Long enough for the room to have said everything it says: read too soon,
// even the word that opens this page has not been drawn yet.
await page.waitForTimeout(6000);
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
let said = await mirror();
console.log(`settings: ${said.length} chars | row offered: ${said.includes('Read the design')}`);

// The row itself: not where the mirror says it stands -- that square is one
// of layout and not of the glass -- but found by pressing, a step of scroll
// at a time, until the design is said.
const title = 'The design of a chat';
let wheeled = 0;
let opened_at = null;
for (let pass = 0; pass < 4 && !opened_at; ++pass) {
  if (pass) {
    await page.mouse.move(450, 500);
    await page.mouse.wheel(0, 2000);
    wheeled += 2000;
    await page.waitForTimeout(900);
  }
  for (const y of [480, 440, 520, 400, 560]) {
    await page.mouse.move(450, y);
    await page.waitForTimeout(200);
    await page.mouse.click(450, y);
    await page.waitForTimeout(900);
    if ((await mirror()).includes(title)) { opened_at = `${y} after ${wheeled}`; break; }
  }
}
said = await mirror();
console.log(`opened: ${opened_at ? `pressed the row at ${opened_at}` : 'NOT pressed'}`);
console.log(`design: ${said.length} chars`);
console.log(`title said: ${said.includes(title)}`);
// Not only opened: read to its later lines, which nothing can be said of
// until the whole column has been said.
console.log(`read to its later lines: ${said.includes('Everything here is drawn by the same tree')}`);
console.log(`first: ${JSON.stringify(said.slice(0, 400))}`);
console.log(`said whole: ${JSON.stringify(said.slice(0, 6000))}`);
await page.screenshot({ path: 'design.png' }).catch(() => {});
await browser.close();
