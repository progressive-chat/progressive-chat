import { chromium } from 'playwright';

// Every square in the page small enough to be a control, pressed once: which
// of them something answers, and which take a press and do nothing with it.
// A square that answers nothing is either not a control -- a word, a field's
// own hint -- or a stub standing where something should be, and only reading
// what the page says tells which.
//
// Each press is made on a page of its own: one press opens a card or empties
// a field, and pressing the next square in the room that card left would be
// pressing something else than what was found. So the page is read again
// after every start, and the squares are read by what is drawn and not by a
// point written down.

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });

// Every square of a control's size, whole inside the room, said once each:
// its middle, and how big it is.
const squares = () => page.evaluate(() => {
  const found = [];
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const box = one.getBoundingClientRect();
    const width = Math.round(box.width), height = Math.round(box.height);
    if (width < 18 || height < 18 || width > 70 || height > 70)
      continue;
    if (box.y < 0 || box.y + box.height > window.innerHeight)
      continue;
    found.push([Math.round(box.x + box.width / 2.0),
                Math.round(box.y + box.height / 2.0), width, height]);
  }
  const seen = new Set(), out = [];
  for (const one of found.sort((a, b) => (a[1] - b[1]) || (a[0] - b[0]))) {
    const said = `${one[0]},${one[1]}`;
    if (seen.has(said))
      continue;
    seen.add(said);
    out.push(one);
  }
  return out;
});

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

await page.goto(
    `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(8000);
const every = await squares();
console.log(`${every.length} squares: ${every.map((one) => one[0] + ',' + one[1]).join(' ')}`);

let dead = 0;
for (const [at, one] of every.entries()) {
  await page.goto(
      `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
  await page.waitForFunction('window.__pchat_started === true', null,
                             { timeout: 180000 });
  await page.waitForTimeout(8000);
  await heard();
  await page.mouse.click(one[0], one[1]);
  await page.waitForTimeout(900);
  const said = await page.evaluate(() => window.__heard.join(' '));
  const answered = said.includes('answered');
  if (!answered)
    dead += 1;
  console.log(`${at + 1}/${every.length} ${one[0]},${one[1]} (${one[2]}x${one[3]}): ${answered ? 'answers' : 'nothing'} | ${said}`);
}
await browser.close();
console.log(dead === 0 ? 'every square answers'
                       : `${dead} of ${every.length} squares answer nothing`);
process.exit(0);
