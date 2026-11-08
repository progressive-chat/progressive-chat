// pchat.probe.now -- What the page served now says, whole: whether the build
// that stands on the net is the one that was made last.
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
await page.goto('https://progressive-chat.github.io/progressive-chat/?asked=' +
  Date.now(), { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(6000);
const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
console.log(`chat: ${JSON.stringify((await mirror()).slice(0, 200))}`);
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
const said = await mirror();
console.log(`said: ${said.length}`);
console.log(`Services: ${said.includes('Services')}`);
console.log(`Appearance and Language: ${said.includes('Appearance & Language')}`);
console.log(`Read the design: ${said.includes('Read the design')}`);
console.log(`build: ${JSON.stringify((said.match(/progressive-chat 0\.1[^\n]*/g) || []).join(' | '))}`);
console.log(`tail: ${JSON.stringify(said.slice(-700))}`);
await browser.close();
