// What the page says where it cannot be isolated: the worker it needs is
// what makes cross-origin isolation, and isolation is what gives it threads
// (`web/bootstrap.js`, `web/worker.js`). A browser told to block service
// workers neither takes one nor says it did not, so what is asked here is
// that the page says so itself -- at the width it is served at, and at the
// width of a phone, which is not the different case it once looked like.
//
//   node probe_worker_blocked.mjs [url]
import { chromium } from 'playwright';

const url = process.argv[2] ?? 'https://progressive-chat.github.io/progressive-chat/';
// Longer than the clock the page takes its worker with, which is fifteen
// seconds: what is read below is what it has said by the time that ran out.
const waited = 20000;

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});

for (const viewport of [{ width: 1280, height: 900 }, { width: 390, height: 844 }]) {
  const context = await browser.newContext({ viewport, serviceWorkers: 'block' });
  const page = await context.newPage();
  await page.goto(url.includes('?') ? url : `${url}?asked=${Date.now()}`,
                  { waitUntil: 'load' });
  await page.waitForTimeout(waited);
  const said = await page.evaluate(
      () => document.getElementById('waiting')?.textContent ?? '');
  console.log(`${viewport.width}x${viewport.height}:`, JSON.stringify(said.trim()));
  await context.close();
}
await browser.close();
