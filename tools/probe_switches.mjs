import { chromium } from 'playwright';

// What a press on each of the seven switches does: the page gives the three
// groups that stood empty -- Multimodal, Network, and Memory & Data -- a
// row each to say whether something is offered at all, and a press asks for
// the other. This is the probe that asks whether each of them was drawn and
// whether each one moves.
//
// The rows are read by the words in them and not by a point written down,
// and each is pressed where it is read after it has been wheeled into the
// room: a row read below the window is not pressed where it was read. What
// says a switch moved is its own row not being said the same way twice --
// its pill is lit or it is not, and that is all it has to say.

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });

// What each of the seven is called, said as the page says them.
const names = ['Read a picture into words', 'Make a picture out of words',
               'Speak through another program',
               'Be served as a page of its own', 'Keep what has been said here',
               'Keep what it has been taught', 'Keep what has been given to it'];

// Every one of them that is drawn, said by name: where its words stand, and
// everything its row is, so that a row said again can be said the same way
// or not.
const said = () => page.evaluate((names) => {
  const found = {};
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const words = (one.textContent || '').trim();
    if (one.children.length !== 0 || !names.includes(words))
      continue;
    let row = one.parentElement;
    while (row && row.getBoundingClientRect().width < 300)
      row = row.parentElement;
    if (!row)
      continue;
    const box = row.getBoundingClientRect();
    found[words] = {x: Math.round(box.x + 60.0),
                    y: Math.round(box.y + box.height / 2.0),
                    row: (row.innerHTML.match(/background[^;"]*/g) || [])
                             .join('|')};
  }
  return found;
}, names);

// The drawer's gear opens the settings: the only way in, and a press in its
// middle that answers is a press on the node that holds it.
await page.goto(
    `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
let at = null;
for (let tried = 0; tried < 30 && !at; ++tried) {
  await page.waitForTimeout(1000);
  at = await page.evaluate(() => {
    for (const one of document.getElementById('skiff').querySelectorAll('*')) {
      const box = one.getBoundingClientRect();
      if (Math.round(box.width) === 36 && Math.round(box.height) === 36 &&
          box.y < 24 && box.x < window.innerWidth / 2)
        return [Math.round(box.x + 18), Math.round(box.y + 18)];
    }
    return null;
  });
}
await page.mouse.click(at[0], at[1]);
await page.waitForTimeout(1500);

const drawn = await said();
for (const one of names)
  console.log(`${one}: ${drawn[one] ? 'drawn' : 'NOT DRAWN'}`);

let moved = 0, still = 0;
for (const one of names) {
  let now = await said();
  for (let tried = 0; tried < 25 && now[one] &&
                     (now[one].y < 120 || now[one].y > 780);
       ++tried) {
    await page.mouse.move(640, 450);
    await page.mouse.wheel(0, now[one].y - 500);
    await page.waitForTimeout(400);
    now = await said();
  }
  if (!now[one] || now[one].y < 120 || now[one].y > 780) {
    console.log(`${one} was not wheeled into the room`);
    still += 1;
    continue;
  }
  await page.mouse.click(now[one].x, now[one].y);
  await page.waitForTimeout(700);
  const after = await said();
  const same = after[one] && after[one].row === now[one].row;
  console.log(`pressed ${one}: ${same ? 'nothing moved' : 'its pill moved'}`);
  if (same)
    still += 1;
  else
    moved += 1;
}
await browser.close();
console.log(still === 0 ? `every one of the ${moved} switches moves`
                        : `${still} switch(es) did not move`);
process.exit(still === 0 ? 0 : 1);
