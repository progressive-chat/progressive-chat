import { chromium } from 'playwright';

// What the two ways out of a conversation's field do: Enter pressed in it,
// and the round button beside it. Both are asked with something written in
// the field, and each is held to the same two answers -- the words are said
// in the conversation, and nothing of them is left in the field -- and the
// button is pressed once with nothing written, to ask for nothing.
//
// The words are written by the keyboard and read back out of the hidden
// field the page keeps for it, which is what writing puts them in; whether
// they were said as a message is read off the page itself.

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
const url = `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`;

// Kept as a page settles: what each press was said to, and what answered it.
const heard = () => page.evaluate(() => {
  window.__heard = [];
  const first = window.Module.ccall.bind(window.Module);
  window.Module.ccall = (name, back, kinds, args) => {
    if (name === 'skiff_browser_pointer')
      window.__heard.push(`said:${args[0]}`);
    else if (name === 'skiff_web_event')
      window.__heard.push(`answered:${args[0]}`);
    return first(name, back, kinds, args);
  };
});
// A page of its own for each press, and nothing read until the room has
// settled into the layout it keeps.
const fresh = async () => {
  await page.goto(url);
  await page.waitForFunction('window.__pchat_started === true', null,
                             { timeout: 180000 });
  await page.waitForTimeout(8000);
  await heard();
};
// What the field holds: read out of the field itself and not off the glass,
// where what is written may not be said twice.
const field = () => page.evaluate(() => {
  const one = Array.from(document.querySelectorAll('textarea, input'))
                   .find((one) => one.getAttribute('aria-label') === 'Message');
  return one ? one.value : 'no field';
});
const saidIn = () => page.evaluate(() => document.getElementById('skiff')
    .innerText.replace(/\s+/g, ' ')
    .includes('said by a press'));
const calls = () => page.evaluate(() => window.__heard.join(' '));

// One written thing, said by each way out of the field in turn.
const written = 'said by a press';
let wrong = 0;
const ask = async (what, how) => {
  await fresh();
  await page.mouse.click(640, 862);
  await page.waitForTimeout(500);
  await page.keyboard.type(written);
  await page.waitForTimeout(600);
  await how();
  await page.waitForTimeout(1500);
  const said = await saidIn(), left = await field(), heardNow = await calls();
  const answered = heardNow.includes('answered');
  console.log(`${what}: answered ${answered ? 'yes' : 'NO'} | said ${said ? 'yes' : 'NO'} | field left "${left}"`);
  if (!said || left !== '')
    wrong += 1;
};

// What the round button does with something written.
await ask('round press', async () => { await page.mouse.click(1233, 864); });
// The same, asked by Enter in the field instead.
await ask('enter', async () => { await page.keyboard.press('Enter'); });

// And the button pressed before anything is written asks for nothing.
await fresh();
await page.mouse.click(1233, 864);
await page.waitForTimeout(1200);
const empty = await saidIn();
console.log(`nothing written: ${empty ? 'something said' : 'nothing said'} (${await calls()})`);
if (empty)
  wrong += 1;
await browser.close();
console.log(wrong === 0 ? 'both ways out of the field say it and empty it'
                        : `${wrong} answer(s) wrong`);
process.exit(wrong === 0 ? 0 : 1);
