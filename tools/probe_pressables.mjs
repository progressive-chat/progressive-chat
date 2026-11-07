// pchat.probe.pressables -- Two presses the rest of the fleet does not ask:
// a room's row (the room changes, and changes back when the first room's row
// is pressed again) and the Info chip under an answer (the line saying what
// the answer came to stands up under it and goes under it again). The fold is
// pressed first as the control every other answer is read against -- the
// fleet's own probe_fold asks the fold alone.
// Everything is placed by what the page says about itself: its cursor says
// what is pressable, its own words say which is which, and a candidate is
// believed only when the mirror answers the press -- a title met in a message
// is a title, not a room, until the room changes. The gear at the head of the
// list is not asked here: probe_settings already presses it.
// No backticks, no dollar-brace sequences and no double quotes anywhere in
// this file: the probe workflow hands it to the shell inside double quotes.
import { chromium } from 'playwright';

const browser = await chromium.launch({
  args: [
    '--no-sandbox',
    '--disable-dev-shm-usage',
    '--use-gl=angle',
    '--use-angle=swiftshader',
    '--enable-unsafe-swiftshader',
    '--js-flags=--max-old-space-size=3072',
  ],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
const loud = [];
page.on('console', (m) => {
  const kind = m.type();
  if (kind === 'error' || kind === 'warning') loud.push(kind + ': ' + m.text());
});
page.on('pageerror', (e) => loud.push('pageerror: ' + String(e).slice(0, 300)));
const target = process.env.PCHAT_PAGE || 'https://progressive-chat.github.io/progressive-chat/';
const asked = target + (target.includes('?') ? '&' : '?') + 'asked=' + Date.now();
await page.goto(asked, { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(2500);

const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
const pressables = () => page.evaluate(() => {
  const out = [];
  for (const el of document.querySelectorAll('[data-skiff-h]')) {
    if (getComputedStyle(el).cursor !== 'pointer') continue;
    const r = el.getBoundingClientRect();
    if (r.width < 4 || r.height < 4) continue;
    const own = [...el.childNodes].filter((n) => n.nodeType === 3)
        .map((n) => n.textContent).join(' ').replace(/\s+/g, ' ').trim();
    out.push({
      h: Number(el.getAttribute('data-skiff-h')),
      x: Math.round(r.x), y: Math.round(r.y),
      w: Math.round(r.width), t: Math.round(r.height),
      own: own.slice(0, 40),
      all: (el.innerText || '').replace(/\s+/g, ' ').trim().slice(0, 70),
    });
  }
  return out;
});
const clickBox = async (b) => {
  await page.mouse.click(b.x + b.w / 2, b.y + b.t / 2);
  await page.waitForTimeout(1200);
};

const all = await pressables();
console.log('pressables: ' + all.length);

// The fold: the control, pressed on its words the way the fleet's own probe
// presses it -- open, and shut again.
const fold = page.locator('text=Thought for').first();
const fbox = await fold.boundingBox();
await page.mouse.click(fbox.x + fbox.width / 2, fbox.y + fbox.height / 2);
await page.waitForTimeout(1200);
const foldOpen = (await mirror()).includes('no recipe of its own known');
await page.mouse.click(fbox.x + fbox.width / 2, fbox.y + fbox.height / 2);
await page.waitForTimeout(1200);
const foldShut = !(await mirror()).includes('no recipe of its own known');
console.log('fold answers: ' + foldOpen + ', and shuts again: ' + foldShut);

// The Info chip: its own word says which it is. Under it, the line saying
// what the answer came to -- digits, comma, digits and the word tokens,
// which stands nowhere else the same way.
const chip = all.find((p) => p.own === 'Info') || all.find((p) => p.all === 'Info');
if (chip) {
  await clickBox(chip);
  const on = /\d{1,3}(,\d{3})+ tokens/.test(await mirror());
  await clickBox(chip);
  const off = !/\d{1,3}(,\d{3})+ tokens/.test(await mirror());
  console.log('info chip answers: ' + on + ', and takes it back: ' + off);
} else {
  console.log('info chip: no pressable says Info');
}
await page.screenshot({ path: 'press-chip.png' }).catch(() => {});

// A room's row: candidates are the pressables that say a room's title other
// than the one open -- and a candidate is believed only when the mirror says
// the room changed. The title met inside a message is not a room's row: the
// press answers nothing, and the next candidate is tried.
const home = await mirror();
const candidates = all.filter((p) => /\bNotices\b|\bWeekend\b/.test(p.all));
let moved = false;
let pressed = null;
for (const cand of candidates) {
  await clickBox(cand);
  if ((await mirror()) !== home) {
    moved = true;
    pressed = cand.all.slice(0, 30);
    break;
  }
}
console.log('room row answers (room changed): ' + moved +
            (pressed ? ' by ' + JSON.stringify(pressed) : '') +
            ' of ' + candidates.length + ' candidates');
if (moved) {
  const away = await mirror();
  const backs = (await pressables()).filter((p) => p.all.includes('What is in the kitchen'));
  let back = false;
  let backBy = null;
  for (const cand of backs) {
    await clickBox(cand);
    if ((await mirror()) === home) {
      back = true;
      backBy = cand.all.slice(0, 30);
      break;
    }
  }
  console.log('and back to the first room: ' + back +
              (backBy ? ' by ' + JSON.stringify(backBy) : '') +
              ' of ' + backs.length + ' candidates');
  if (!back) console.log('mirror differs from start: ' + ((await mirror()) !== home));
}
await page.screenshot({ path: 'press-rooms.png' }).catch(() => {});
console.log('page said: ' + JSON.stringify(loud.slice(0, 8)));
await browser.close();
console.log('done');
