import { chromium } from 'playwright';

// Whether what a Copy chip copies is really given to the machine whose
// browser this page is read in, and not only kept in the program. Nothing
// says so from inside: the host's clipboard is asked for by permission, and
// what is in it is read back with the words that were there before, so that
// a clipboard nobody wrote to says "before anything is pressed" and not the
// answer.

const browser = await chromium.launch();
const context = await browser.newContext({
  viewport: { width: 1280, height: 900 },
  permissions: ['clipboard-read', 'clipboard-write'],
});
const page = await context.newPage();
await page.goto(
    `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
// Long enough for the room to have said everything it says of itself and put
// every chip where it stands: read too soon, the box a chip was read with is
// one it has already left.
await page.waitForTimeout(6000);

const read = async () => await page.evaluate(
    () => document.getElementById('skiff').innerText);
// The first Copy chip on the page: its own words, whichever answer it stands
// under. Pressed where it stands rather than in its left padding: a press
// that moves nowhere first is not one the room answers, and what reaches the
// chip is what is under the pointer.
const chip = await page.evaluate(() => {
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const own = [...one.childNodes]
                    .map((n) => (n.nodeType === 3 ? n.textContent : ''))
                    .join(' ')
                    .replace(/\s+/g, ' ')
                    .trim();
    if (own !== 'Copy') continue;
    const box = one.getBoundingClientRect();
    if (box.width < 1) continue;
    return [box.x, box.y, box.width, box.height];
  }
  return null;
});
if (!chip) {
  console.log('no Copy was said on the page at all');
  await browser.close();
  process.exit(1);
}

// What the answer above the chip says, which is what is copied: the line of
// it that stands alone, answer's own, above the six chips.
const answer = await read();
await page.evaluate(
    () => navigator.clipboard.writeText('before anything is pressed'));
await page.mouse.move(chip[0] + chip[2] / 2, chip[1] + chip[3] / 2);
await page.waitForTimeout(250);
await page.mouse.click(chip[0] + chip[2] / 2, chip[1] + chip[3] / 2);
await page.waitForTimeout(900);
const held = await page.evaluate(
    () => navigator.clipboard.readText().catch((e) => 'read refused: ' + e.message));
console.log('pressed: the clipboard keeps', JSON.stringify(String(held).slice(0, 120)));
// What it keeps is not what was there before, and it is what the answer
// above the chip says: copied out of the program and into the machine.
const said_before = String(held) !== 'before anything is pressed';
const of_the_answer = String(held).length > 8 &&
    answer.includes(String(held).slice(0, 20));
console.log(said_before && of_the_answer
    ? 'what a Copy chip copies is given to the machine, not only kept inside'
    : `NOT: not the sentinel ${said_before}, of the answer ${of_the_answer}`);
await browser.close();
