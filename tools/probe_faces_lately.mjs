// What the card of faces keeps: that the ones pressed most lately stand in a
// row above the rest of them, newest first, from the press that wrote them
// into the field to the card opened again.
//
//   node probe_faces_lately.mjs [url]
//
// The two faces it presses are the first two squares of the card, which is
// where they stand in it before any of them has been pressed.
import { chromium } from 'playwright';

const url = process.argv[2] ?? 'https://progressive-chat.github.io/progressive-chat/';
const face = { x: 1194, y: 864 };
const chips = [{ x: 959, y: 707 }, { x: 995, y: 707 }];

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

const said = () => page.evaluate(() => document.getElementById('skiff')?.innerText ?? '');
const field = () =>
  page.evaluate(() =>
    Array.from(document.querySelectorAll('#skiff textarea, #skiff input'))
      .map((n) => `${n.getAttribute('aria-label') ?? '?'}:${n.value ?? ''}`)
      .filter((s) => s.startsWith('Message:'))
      .join(''));

const asked = async (x, y) => {
  await page.mouse.click(x, y);
  await page.waitForTimeout(1200);
};

console.log('nothing pressed yet:', (await said()).includes('😀'));
await asked(face.x, face.y);
await asked(chips[0].x, chips[0].y);
await asked(chips[1].x, chips[1].y);
const written = await field();
console.log('two written into the field:', JSON.stringify(written));
// Put away by the room, and opened again: the row above is read of a card
// that has been opened since the faces were pressed.
await asked(640, 400);
await asked(face.x, face.y);
// The mirror says one node to a line, so the row is read as its own run of
// faces with the breaks taken out of it.
const open = (await said()).replace(/\s+/g, '');
// Newest first, which is not the order they were written into the field in.
const lately = open.includes('😀🙂');
console.log('opened again, the row above says:', lately);
console.log('the field says:', JSON.stringify(await field()));
await browser.close();
