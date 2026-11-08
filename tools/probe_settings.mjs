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
// Whether it can be pressed: only if it stands inside the page at all. Read
// it out rather than pressing a square that may be far below the window, for
// a press made there reaches nothing and says nothing either way.
const reachable = row[1] > 0 && row[1] < 900;
console.log(`row stands at: x ${Math.round(row[0])}, y ${Math.round(row[1])} | `
            + `in the page: ${reachable}`);
if (reachable) {
  await page.mouse.move(row[0], row[1]);
  await page.waitForTimeout(250);
  await page.mouse.click(row[0], row[1]);
  await page.waitForTimeout(1200);
  const design = await mirror();
  console.log(`design: ${design.length} chars | title said: ${design.includes('The design of a chat')}`);
} else {
  console.log(`not pressed: nothing reaches ${Math.round(row[1])} in a page `
              + `900 high, and no wheel or drag has moved it yet`);
}
await page.screenshot({ path: 'settings-design.png' }).catch(() => {});
await page.screenshot({ path: 'settings-design.png' }).catch(() => {});
await browser.close();
