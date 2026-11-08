import { chromium } from 'playwright';

// What the three dots beside the card's ring hold, read off the painted page:
// thinking, how hard it is asked to think, and the way into the large
// settings (design-doc.md, "The editor's row").
//
// Nothing here is read off a point written down. The dots are found by
// pressing the card's own top row until a card of rows opens above it, which
// is the only thing drawn there that opens anything; then every row is found
// by the word it is called, and read again after it was pressed -- what it
// says beside it is what it was asked for, and a row whose word never moves
// was pressed for nothing at all.

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });

const start = async () => {
  await page.goto(
      `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
  await page.waitForFunction('window.__pchat_started === true', null,
                             { timeout: 180000 });
  await page.waitForTimeout(8000);
};

// Every square of a control's size in the card's top row: what the ring is
// followed by is one of them, and only one of them opens anything.
const top_row = () => page.evaluate(() => {
  const found = [];
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const box = one.getBoundingClientRect();
    const width = Math.round(box.width), height = Math.round(box.height);
    if (width < 18 || height < 18 || width > 70 || height > 70)
      continue;
    const middle = [Math.round(box.x + box.width / 2.0),
                    Math.round(box.y + box.height / 2.0)];
    // The card stands at the foot of the room, and what is said above its
    // own field is said in its top row: the ring, and what follows it.
    if (middle[1] < window.innerHeight - 130 ||
        middle[1] > window.innerHeight - 60)
      continue;
    const over = document.elementFromPoint(middle[0], middle[1]);
    if (!(over && (one === over || one.contains(over))))
      continue;
    found.push(middle);
  }
  const seen = new Set(), out = [];
  for (const one of found.sort((a, b) => (b[0] - a[0]) || (a[1] - b[1]))) {
    const said = one.join(',');
    if (seen.has(said))
      continue;
    seen.add(said);
    out.push(one);
  }
  return out;
});

// What the open card says: each row by the word it is called, and the word
// said at its right hand.
const menu = () => page.evaluate(() => {
  const rows = [];
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    if (one.children.length)
      continue;
    const said = (one.textContent || '').trim();
    if (!['Thinking', 'Effort', 'Advanced'].includes(said) &&
        !['off', 'low', 'medium', 'high', 'all'].includes(said))
      continue;
    const box = one.getBoundingClientRect();
    // What a fold keeps folded away is said nowhere and measured at
    // nothing: a row put away is still in the page, of no width at all.
    if (box.width < 1)
      continue;
    rows.push([said, Math.round(box.x), Math.round(box.y + box.height / 2.0),
               Math.round(box.width)]);
  }
  return rows.sort((a, b) => (a[2] - b[2]) || (a[1] - b[1]))
      .map((one) => `${one[0]}@${one[1]},${one[2]}`);
});

// One row, pressed in its own words: twenty points in from where the row
// begins, which is inside whichever of them was meant.
const press_row = async (name) => {
  const at = await page.evaluate((called) => {
    for (const one of document.getElementById('skiff').querySelectorAll('*')) {
      if (one.children.length || (one.textContent || '').trim() !== called)
        continue;
      const box = one.getBoundingClientRect();
      if (box.width < 1)
        continue;
      return [Math.round(box.x + 20.0), Math.round(box.y + box.height / 2.0)];
    }
    return null;
  }, name);
  if (!at)
    return false;
  await page.mouse.move(at[0], at[1]);
  await page.waitForTimeout(250);
  await page.mouse.click(at[0], at[1]);
  await page.waitForTimeout(900);
  return true;
};

const said = (rows, name) => {
  for (const one of rows)
    if (one.startsWith(name + '@'))
      return one.split('@')[0];
  return null;
};
// What a row says at its right hand: the words kept to the row's own right,
// read by standing further along the row than its name reaches.
const value_of = (rows, name) => {
  const own = rows.find((one) => one.startsWith(name + '@'));
  if (!own)
    return null;
  const row_y = Number(own.split('@')[1].split(',')[1]);
  for (const one of rows) {
    if (one.includes(name))
      continue;
    const parts = one.split('@')[1].split(',');
    if (Number(parts[1]) === row_y)
      return one.split('@')[0];
  }
  return null;
};

await start();
// Which square beside the ring opens the card, said by trying each of them.
const every = await top_row();
let dots = null;
for (const one of every) {
  await start();
  // Pointed at first: a press put down where nothing had been pointed at is
  // not answered, and every one of these squares is small.
  await page.mouse.move(one[0], one[1]);
  await page.waitForTimeout(250);
  await page.mouse.click(one[0], one[1]);
  await page.waitForTimeout(900);
  const opened = await menu();
  if (opened.length) {
    dots = one;
    break;
  }
}
if (!dots) {
  console.log('nothing in the card\'s top row opens anything: three dots said '
              + 'in the design (design-doc.md) are not drawn, or they answer '
              + 'nothing');
  await browser.close();
  process.exit(1);
}
console.log(`three dots at ${dots.join(',')}`);

const opened = await menu();
console.log(`open (${opened.length} words): ${opened.join(' ')}`);

// Thinking: off where it was any of the four, and the one it stood at again
// where it was off.
const was = value_of(opened, 'Thinking', 0);
await press_row('Thinking');
const one_off = await menu();
await press_row('Thinking');
const two_back = await menu();
console.log(`thinking: ${was} -> ${value_of(one_off, 'Thinking')} -> `
            + `${value_of(two_back, 'Thinking')}`);

// How hard: each press asks for the next of the four, and the fourth press
// is the first of them again.
const steps = [value_of(two_back, 'Effort')];
for (let press = 0; press < 4; ++press) {
  await press_row('Effort');
  steps.push(value_of(await menu(), 'Effort'));
}
console.log(`effort: ${steps.join(' -> ')}`);

// Advanced: the large settings, opened, and the card put away with it.
await press_row('Advanced');
await page.waitForTimeout(1200);
const gone = await menu();
const settings = await page.evaluate(
    () => document.getElementById('skiff').innerText.includes('Appearance'));
console.log(`advanced: settings ${settings ? 'open' : 'shut'}, `
            + `${gone.length} words of the card left`);

// And the dots themselves again: what they opened is put away by one press
// of them.
await start();
await page.mouse.move(dots[0], dots[1]);
await page.waitForTimeout(250);
await page.mouse.click(dots[0], dots[1]);
await page.waitForTimeout(900);
const again_open = (await menu()).length;
await page.mouse.move(dots[0], dots[1]);
await page.waitForTimeout(250);
await page.mouse.click(dots[0], dots[1]);
await page.waitForTimeout(900);
const again_shut = (await menu()).length;
console.log(`${again_open} words open, ${again_shut} words behind a second `
            + 'press of the same dots');

// And by a press anywhere else in the room at all: what nothing painted
// takes, standing whole over the room behind the card, takes it and puts
// the card away with nothing more asked of it.
await start();
await page.mouse.move(dots[0], dots[1]);
await page.waitForTimeout(250);
await page.mouse.click(dots[0], dots[1]);
await page.waitForTimeout(900);
const away_before = (await menu()).length;
await page.mouse.click(640, 400);
await page.waitForTimeout(900);
const away_after = (await menu()).length;
console.log(`${away_before} words open, ${away_after} words behind a press `
            + 'in the middle of the messages');

const moved = new Set([was, value_of(one_off, 'Thinking'),
                       value_of(two_back, 'Thinking')].filter(Boolean));
const walked = new Set(steps.filter(Boolean));
await browser.close();
if (again_shut === 0 && again_open > 0 && settings && gone.length === 0 &&
    away_before > 0 && away_after === 0 &&
    moved.size === 2 && walked.size === 4) {
  console.log('what the dots hold answers every row of it: thinking is off '
              + 'and the four again, and the settings are opened from it');
  process.exit(0);
}
console.log('something behind the three dots answered nothing');
process.exit(1);
