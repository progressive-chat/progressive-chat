import { chromium } from 'playwright';

// What each field is for is said in its label, which is what it shows empty:
// that, and what it holds, said for every field on the page.
const values = () =>
  Array.from(document.querySelectorAll('textarea, input')).map(
      (t) => `${t.getAttribute('aria-label') || t.type} = ${t.value}`);
const focused = () => {
  const one = document.activeElement;
  return `${one.tagName}:${one.getAttribute('aria-label') || ''}`;
};
const said_of = (all) =>
    all.find((one) => one.startsWith('You are answering in')) || '';

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
// Long enough for the room to have said everything it says.
await page.waitForTimeout(6000);
console.log('fields at start:', JSON.stringify(await page.evaluate(values)));

// The card the system words are written in is opened by the second plate of
// the room's own top row, said as the others are by pressing it rather than
// by a point remembered: pointed at first, then pressed. With it shut the
// field is in the page at no breadth at all, which is why it cannot be
// pressed shut.
const plate = [1206, 26];
await page.mouse.move(plate[0], plate[1]);
await page.waitForTimeout(250);
await page.mouse.click(plate[0], plate[1]);
await page.waitForTimeout(1200);
console.log('fields open:', JSON.stringify(await page.evaluate(values)));

// Where the field stands, said by itself: a textarea keeps what is in it
// where innerText of the page cannot see it, and only a press on it gives it
// the keying.
const field = await page.evaluate(() => {
  for (const one of document.querySelectorAll('[aria-label]'))
    if ((one.getAttribute('aria-label') || '').startsWith('You are answering')) {
      const box = one.getBoundingClientRect();
      if (box.width < 1)
        continue;
      return [Math.round(box.x + box.width / 2.0),
              Math.round(box.y + box.height / 2.0)];
    }
  return null;
});
console.log('the field stands at', JSON.stringify(field));
if (field) {
  await page.mouse.move(field[0], field[1]);
  await page.waitForTimeout(250);
  await page.mouse.click(field[0], field[1]);
  await page.waitForTimeout(600);
}
console.log('focus:', await page.evaluate(focused));
await page.keyboard.press('Control+a');
await page.keyboard.type('Answer in one line.');
await page.waitForTimeout(900);
const typed = await page.evaluate(values);
console.log('fields typed:', JSON.stringify(typed));
console.log('wrote:', said_of(typed));

// Put away, opened again: what the conversation keeps is what the card is
// filled with, which is only what this wrote into it.
await page.mouse.move(plate[0], plate[1]);
await page.waitForTimeout(250);
await page.mouse.click(plate[0], plate[1]);
await page.waitForTimeout(900);
const put = await page.evaluate(values);
console.log('fields put away:', JSON.stringify(put),
            '| focus:', await page.evaluate(focused));
await page.mouse.move(plate[0], plate[1]);
await page.waitForTimeout(250);
await page.mouse.click(plate[0], plate[1]);
await page.waitForTimeout(1000);
const opened = await page.evaluate(values);
console.log('fields opened again:', JSON.stringify(opened));
console.log('kept:', said_of(opened).includes('Answer in one line.'),
            'gone while shut:', said_of(put));
await browser.close();
