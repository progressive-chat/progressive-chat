// What the face beside the clip answers, read off the page: does it open a
// card of faces, does a press on one of them write it where the pen stands,
// and does a press below the card put it away again.
//
//   node probe_faces.mjs [url]
//
// Where nothing is said of a face, the room has none to press.
import { chromium } from 'playwright';

const url = process.argv[2] ?? 'https://progressive-chat.github.io/progressive-chat/';

// Read from the field the way everything else does: what it is given is a
// value in a box beside the drawing, not a node in it.
async function values(page) {
  return page.evaluate(() =>
    Array.from(document.querySelectorAll('#skiff textarea, #skiff input'))
      .map((n) => `${n.getAttribute('aria-label') ?? '?'}:${n.value ?? ''}`)
      .filter((s) => s.startsWith('Message:') || s.startsWith('Search')));
}

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

const text = () => page.evaluate(() => document.getElementById('skiff')?.innerText ?? '');
const faces_out = async () => (await text()).includes('😀');

// The face beside the clip, and the two first squares of the card it opens.
const face = { x: 1194, y: 864 };
const chips = [{ x: 959, y: 707 }, { x: 995, y: 707 }];

const asked = async (x, y) => {
  await page.mouse.click(x, y);
  await page.waitForTimeout(1200);
};

console.log('before:', await faces_out(), JSON.stringify(await values(page)));
await asked(face.x, face.y);
const opened = await faces_out();
console.log('opened by the face:', opened, JSON.stringify(await values(page)));

if (opened) {
  await asked(chips[0].x, chips[0].y);
  const one = await values(page);
  console.log('one face pressed:', JSON.stringify(one), 'still open:', await faces_out());
  await asked(chips[1].x, chips[1].y);
  const two = await values(page);
  console.log('another pressed:', JSON.stringify(two));
  // Somewhere below the card, in the middle of the messages: putting the
  // card away is what a press anywhere there asks for.
  await page.mouse.click(640, 400);
  await page.waitForTimeout(1200);
  console.log('put away by the room:', !(await faces_out()), JSON.stringify(await values(page)));
  const written = two.find((s) => s.startsWith('Message:')) ?? '';
  console.log(
    'said it all:',
    opened && written.includes('🙂') && written.includes('😀')
  );
}
await browser.close();
