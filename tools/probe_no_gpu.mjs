// What a machine with no GPU to draw through does: nothing before its first
// frame, or the room drawn in software instead. `pchat.platform.drawn`
// (`src/platform/drawn.cc`) asks SDL what there is to draw with, and
// `src/main.cc` asks the window to draw in software where the answer is
// nothing -- so what this probe says is whether that reached the page.
//
//   node probe_no_gpu.mjs [url]
//
// Whether there really is no GL is said first, for a browser told to have
// none may have it just the same, and a page that painted nothing is not a
// page drawn in software.
import { chromium } from 'playwright';

const url =
  process.argv[2] ?? 'https://progressive-chat.github.io/progressive-chat/';

const browser = await chromium.launch({
  args: ['--disable-gpu', '--disable-webgl', '--disable-webgl2'],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
page.on('pageerror', (e) => console.log('pageerror:', String(e).slice(0, 200)));
page.on('crash', () => console.log('the page went'));
await page.goto(url.includes('?') ? url : `${url}?asked=${Date.now()}`,
                { waitUntil: 'load' });

const gl = await page.evaluate(() => ({
  webgl: !!document.createElement('canvas').getContext('webgl'),
  webgl2: !!document.createElement('canvas').getContext('webgl2'),
}));
console.log('gl to be had:', JSON.stringify(gl));

let started = false;
for (let i = 0; i < 40; i++) {
  if (await page.evaluate(() => !!window.__pchat_started)) {
    started = true;
    break;
  }
  await page.waitForTimeout(1000);
}
await page.waitForTimeout(3000);
const said = await page.evaluate(
    () => document.getElementById('skiff')?.innerText?.length ?? -1);
console.log('started:', started, 'said:', said);
// What it comes to: no GL at all, and the room painted the same whole as it
// paints with one.
console.log('drawn in software:',
            started && !gl.webgl && !gl.webgl2 && said > 1000);
await browser.close();
