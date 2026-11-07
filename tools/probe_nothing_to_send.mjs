// Whether the field at the foot says it has anything to send: one rule asked
// of `pchat.logic.composer` (`can_send`), read off the page at the two places
// it is asked -- the round button beside the field, and Enter pressed in the
// field itself. Spaces and newlines are written in and say nothing, so what
// is proven here is that neither way out of the field sends them.
//
//   node probe_nothing_to_send.mjs [url]
import { chromium } from 'playwright';

const url = process.argv[2] ?? 'https://progressive-chat.github.io/progressive-chat/';
const field = { x: 640, y: 862 };
const send = { x: 1233, y: 864 };

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
page.on('pageerror', (e) => console.log('pageerror:', String(e).slice(0, 200)));
await page.goto(url.includes('?') ? url : `${url}?asked=${Date.now()}`, { waitUntil: 'load' });
for (let i = 0; i < 40; i++) {
  if (await page.evaluate(() => !!window.__pchat_started)) break;
  await page.waitForTimeout(1000);
}
await page.waitForTimeout(3000);

// What the room says, and how many messages it says it of: sending a field
// with nothing in it would put another on the column and another thought
// under it.
const said = () => page.evaluate(() => document.getElementById('skiff')?.innerText ?? '');
const answers = async () => (await said()).split('Thought for').length - 1;
const values = () =>
  page.evaluate(() =>
    Array.from(document.querySelectorAll('#skiff textarea, #skiff input'))
      .map((n) => `${n.getAttribute('aria-label') ?? '?'}:${n.value ?? ''}`)
      .filter((s) => s.startsWith('Message:')));

const before = await answers();

await page.mouse.click(field.x, field.y);
await page.keyboard.type('   ');
await page.waitForTimeout(800);
const written = await values();
console.log('three spaces written:', JSON.stringify(written));
// The round button, which lights only where there is something to send: a
// press on it here asks for nothing, and nothing comes of it.
await page.mouse.click(send.x, send.y);
await page.waitForTimeout(1500);
const after_button = await answers();
console.log('no message from the button:', after_button === before);

// Enter in the field: the other way out of it, asking the same rule.
await page.mouse.click(field.x, field.y);
await page.keyboard.press('Enter');
await page.waitForTimeout(1500);
console.log('no message from enter:', (await answers()) === before);

// Something real, so that the two above are not the field refusing to be
// written in at all: a word, the same button, and one more answer coming.
await page.keyboard.type('send this');
await page.waitForTimeout(800);
console.log('a word written:', JSON.stringify(await values()));
await page.mouse.click(send.x, send.y);
await page.waitForTimeout(2500);
const after_word = await answers();
console.log('one more answer:', after_word > before);
await browser.close();
