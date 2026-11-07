import { chromium } from 'playwright';

// What a line folded above an answer says when it is opened: the calls it
// counted on its way. The sample keeps three of them under its first two
// answers -- asked for what was said about butter, lemon and eggs, and for
// what was said about curd -- and each is looked for in the page while the
// fold is shut, once it has been pressed open, and once it has been pressed
// shut again. A fold that said its calls while shut, or kept saying them
// after it was shut, says nothing it should.
//
// The press is put on the words of the fold's line and not on its mark: a
// press lands on what was painted there last, and a whole in the room is
// not always its own.

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
const url = `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`;

await page.goto(url);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(8000);

// Where the first fold's line stands, and what is said in it.
const foldAt = () => page.evaluate(() => {
  const glass = document.getElementById('skiff');
  for (const one of glass.querySelectorAll('*')) {
    if (one.children.length !== 0)
      continue;
    if (!(one.textContent || '').startsWith('Thought for'))
      continue;
    const whole = one.getBoundingClientRect();
    return [Math.round(whole.x + 40), Math.round(whole.y + whole.height / 2)];
  }
  return [0, 0];
});
// Each call's own words: every leaf that names one of the two tools the
// sample's answers reached for, said whole, for one such leaf holds all of
// the calls of one answer.
const calls = () => page.evaluate(() => {
  const said = [];
  const glass = document.getElementById('skiff');
  for (const one of glass.querySelectorAll('*')) {
    if (one.children.length !== 0)
      continue;
    const words = (one.textContent || '').trim();
    if (!words.includes('conversation_search'))
      continue;
    said.push(words);
  }
  return said;
});

const each = ['butter', 'lemon', 'eggs', 'curd'];
const kept = (words) => each.filter((one) => words.some((said) =>
    said.includes(one))).length;

const fold = await foldAt();
let wrong = 0;
const standing = async (what) => {
  const said = await calls();
  console.log(`${what}: ${kept(said)} of ${each.length} calls said` +
              ` (${said.length} line(s) naming a tool)`);
  return kept(said);
};

if (await standing('shut') !== 0) {
  console.log('no call is said while the fold is shut: WRONG');
  wrong += 1;
}
await page.mouse.click(fold[0], fold[1]);
await page.waitForTimeout(1200);
// Our own answer's three calls are on this page; curd's belongs to the next
// answer and may not yet have been painted at this size.
if (await standing('open') < 3) {
  console.log('an opened fold says each call it counted: WRONG');
  wrong += 1;
}
await page.mouse.click(fold[0], fold[1]);
await page.waitForTimeout(1200);
if (await standing('shut again') !== 0) {
  console.log('a fold shut again says no call: WRONG');
  wrong += 1;
}

await browser.close();
console.log(wrong === 0 ? 'an opened fold says each call, and shut it says none'
                        : `${wrong} answer(s) wrong`);
process.exit(wrong === 0 ? 0 : 1);
