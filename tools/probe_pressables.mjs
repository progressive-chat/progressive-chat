// pchat.probe.pressables -- Which presses answer on the page, beside the fold
// and the chips already spoken for: the rooms of the list and the settings
// plate with them -- each answer read back out of the mirror's own words,
// each place found by what the page says about itself (its own cursor, boxes
// and labels) and never by a coordinate written down here.
// The drawer stands open on its own at this width; its settings plate is the
// small empty square at the head of it, pressed last because what it opens
// stands over everything.
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
console.log('pressables: ' + all.length + '; top of the drawer and bar: ' +
            JSON.stringify(all.filter((p) => p.y < 56 && p.w <= 44)));

// The fold: the control every other answer is read against.
const fold = page.locator('text=Thought for').first();
const fbox = await fold.boundingBox();
await page.mouse.click(fbox.x + fbox.width / 2, fbox.y + fbox.height / 2);
await page.waitForTimeout(1200);
const opened = (await mirror()).includes('no recipe of its own known');
await page.mouse.click(fbox.x + fbox.width / 2, fbox.y + fbox.height / 2);
await page.waitForTimeout(1200);
const closed = !(await mirror()).includes('no recipe of its own known');
console.log('fold answers: ' + opened + ', and shuts again: ' + closed);

// The Info chip: the line saying what the answer came to stands up under it
// and goes away under it again -- digits, comma, digits, and the word tokens,
// which stands nowhere else the same way.
const chip = all.find((p) => /^Info$/.test(p.own) || /^Info\b/.test(p.all));
if (chip) {
  await clickBox(chip);
  const on = /\d{1,3}(,\d{3})+ tokens/.test(await mirror());
  await clickBox(chip);
  const off = !/\d{1,3}(,\d{3})+ tokens/.test(await mirror());
  console.log('info chip answers: ' + on + ', and takes it back: ' + off);
} else {
  console.log('info chip: not found');
}

// Another room of the list, and back: the mirror says the room changed, and
// says it changed back when the first room's row is pressed.
const drawerRows = (list) => list.filter((p) => p.y >= 56 && p.x <= 420 && p.w >= 200);
const other = drawerRows(all).find((p) => /\bWeekend\b/.test(p.all));
const before = await mirror();
if (other) {
  await clickBox(other);
  const away = (await mirror()) !== before;
  const home = drawerRows(await pressables())
                   .find((p) => /\bWhat is in the kitchen\b/.test(p.all));
  let back = false;
  if (home) {
    await clickBox(home);
    back = (await mirror()) === before;
  }
  console.log('room row answers (room changed): ' + away + ', and back again: ' + back);
} else {
  console.log('weekend row: not found');
}
await page.screenshot({ path: 'press-rooms.png' }).catch(() => {});

// The settings plate: the small empty square at the head of the drawer,
// pressed last -- what it opens stands over everything and says
// Appearance & Language, which stands nowhere else.
const plate = all.find((p) => p.y < 56 && p.x < 1000 && p.w <= 40 && p.t <= 40 &&
                        p.all === '');
if (plate) {
  await clickBox(plate);
  const went = (await mirror()).includes('Appearance & Language');
  console.log('settings plate answers (settings came): ' + went +
              ' by ' + JSON.stringify({h: plate.h, x: plate.x, y: plate.y}));
} else {
  console.log('settings plate: not found');
}
await page.screenshot({ path: 'press-settings.png' }).catch(() => {});
console.log('page said: ' + JSON.stringify(loud.slice(0, 8)));
await browser.close();
console.log('done');
