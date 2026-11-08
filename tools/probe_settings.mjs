// pchat.probe.settings -- What the settings page says, and whether what is
// said further down in it can be reached: the page opened, the names of the
// nine groups read back out of the text mirror, the column scrolled to its
// foot, and the row that offers the design pressed there.
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
const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
// Long enough for the room to have said everything it says: read too soon,
// even the word "Settings" has not been drawn yet.
await page.waitForTimeout(6000);
// The settings plate: opened by the word "Settings" itself, which stands in
// the drawer -- read where it stands rather than guessed at, because the
// square it was once pressed at is one it left long ago.
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
await page.mouse.move(entry[0], entry[1]);   // a press reaches what the pointer stood on first
await page.waitForTimeout(250);
await page.mouse.click(entry[0], entry[1]);
await page.waitForTimeout(800);
const said = await mirror();
console.log(`settings: ${said.length} chars`);
for (const name of ['Services', 'Providers', 'Models', 'Responses',
  'Context', 'Multimodal', 'Image transcription', 'Tools', 'Web search',
  'Automation', 'Network', 'Proxy', 'Memory & Data', 'Skills',
  'Notifications', 'Send a test notification',
  'Appearance & Language', 'Language', 'About'])
  console.log(`said ${name}: ${said.includes(name)}`);
// The card stands in the middle of the room and what it holds scrolls: to
// the foot of the column, where the row that offers the design is.
await page.mouse.move(640, 500);
await page.mouse.wheel(0, 4000);
await page.waitForTimeout(800);
const foot = await mirror();
console.log(`scrolled: ${foot.length} chars | moved: ${foot !== said}`);
console.log(`seeds still said: ${foot.includes('Midnight')}`);
console.log(`row offered: ${foot.includes('Read the design')}`);
await page.screenshot({ path: 'settings.png' }).catch(() => {});
// The same for the row that offers the design: read where it stands now that
// the column has been scrolled to its foot, rather than pressing a square
// that belonged to the room and not to this page.
const row = await page.evaluate(() => {
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const own = [...one.childNodes].map((n) => (n.nodeType === 3 ? n.textContent : ''))
        .join(' ').replace(/\s+/g, ' ').trim();
    if (!own.includes('Read the design')) continue;
    const b = one.getBoundingClientRect();
    if (b.width < 1) continue;
    return [b.x + b.width / 2, b.y + b.height / 2];
  }
  return null;
});
if (!row) { console.log('no row offers the design'); await browser.close(); process.exit(1); }
console.log(`row box, said of layout: x ${Math.round(row[0])}, y ${Math.round(row[1])}`);
// Whether it can be pressed, and by what: the square the mirror gives for
// the row is a square of layout and not of the glass -- it says y 2417 in a
// page 900 high, which no press reaches -- so the page is scrolled a step at
// a time and pressed where a row stands at each step, until the design is
// said. Found so: wheeled 4000, then pressed at x 450, y 480.
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
console.log(opened_at
    ? `the row offers the design, and pressed it opens: ${opened_at}, and the page says "${title}"`
    : `NOT: no press of the row opened the design (wheeled as far as ${wheeled})`);
await page.screenshot({ path: 'settings-design.png' }).catch(() => {});
await browser.close();
