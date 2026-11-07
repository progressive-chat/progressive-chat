import { chromium } from 'playwright';

// What a press on each plate over the messages does: something read
// afterwards, every one of them and not one of them. A plate here is the
// one that begins a chat, the three that ask for what every model is told,
// a conversation taken into one of its own, and this one given away, and --
// where the window is too narrow to keep the drawer open -- the pill that
// opens it.
//
// Where each stands is read off the page and not written here, for two
// reasons: what the page says was pressed is whatever element it painted
// last over that point -- so where nothing is pressed, nothing was painted
// over nothing -- and the bar itself moves once, the column beside the
// messages not keeping the breadth it begins with. So the bar is waited on
// to stop moving: two readings a second apart that say the same thing.
//
// Each plate is pressed in a page of its own: pressing one asks the program
// for something that changes what the next would be pressed over -- a card
// comes out, or a conversation is taken into one of its own.

const browser = await chromium.launch();
// One of the four gives this conversation away, which is to say it puts it
// where anything else can be pasted: so this asks to read what is held
// there, for that press changes nothing anyone can see.
const context = await browser.newContext(
    { viewport: { width: 1280, height: 900 },
      permissions: ['clipboard-read', 'clipboard-write'] });
const page = await context.newPage();

// Every plate in the bar, said left to right, one line to each: thirty-six
// across and thirty-six down, along the top of the window and on its right,
// which is where the bar's own presses stand. Two elements stand in one
// place -- a plate and the mark painted in it -- and the same place said
// twice is one press, not two.
const places = () => page.evaluate(() => {
  const found = [];
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const box = one.getBoundingClientRect();
    if (Math.round(box.width) === 36 && Math.round(box.height) === 36 &&
        box.y < 20 && box.x > window.innerWidth / 2)
      found.push([box.x + 18, box.y + 18]);
  }
  return found.sort((a, b) => a[0] - b[0])
      .filter((one, at, all) => at === 0 || one[0] - all[at - 1][0] > 8);
});
const said = () => page.evaluate(
    () => document.getElementById('skiff').innerText);

const settle = async () => {
  await page.goto(
      `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
  await page.waitForFunction('window.__pchat_started === true', null,
                             { timeout: 180000 });
  let was = await places();
  for (let tried = 0; tried < 30; ++tried) {
    await page.waitForTimeout(1000);
    const now = await places();
    if (JSON.stringify(now) === JSON.stringify(was))
      break;
    was = now;
  }
  // What every press is said to, and what answered it: the page tells the
  // program which node was pressed, and what was pressed tells it back.
  await page.evaluate(() => navigator.clipboard.writeText(
      'before anything is pressed'));
  await page.evaluate(() => {
    window.__said = [];
    const first = window.Module.ccall.bind(window.Module);
    window.Module.ccall = (name, returns, kinds, args) => {
      if (name === 'skiff_browser_pointer' || name === 'skiff_web_event')
        window.__said.push(
            `${name === 'skiff_web_event' ? 'answered by' : 'said to'} ${args[0]}`);
      return first(name, returns, kinds, args);
    };
  });
};

let good = 0, unanswered = 0;
await settle();
const every = await places();
console.log(`${every.length} plates stand in the bar, at ` +
            every.map(([x, y]) => `${x},${y}`).join(' '));
for (const at of every.keys()) {
  if (at !== 0)
    await settle();
  const where = await places();
  const [x, y] = where[at];
  const was = await said();
  await page.mouse.click(x, y);
  await page.waitForTimeout(1300);
  const calls = await page.evaluate(() => window.__said.join(', '));
  const answered = calls.includes('answered by');
  const said_now = (await said()) !== was;
  const held = await page.evaluate(
      () => navigator.clipboard.readText().catch(() => 'it was refused'));
  const given = held !== 'before anything is pressed';
  const changed = said_now || given;
  console.log(`the plate ${at + 1} of ${where.length}, pressed at ${x},${y}: ` +
              `${answered ? 'answered' : 'UNANSWERED'} (${calls})` +
              `${said_now ? ', and something else was said'
                          : (given ? ', and this conversation is held where' +
                                     ' anything can be pasted'
                                   : ', and nothing changed')}`);
  if (answered && changed)
    good += 1;
  else
    unanswered += 1;
}
await browser.close();
console.log(unanswered === 0 ? `every one of the ${good} plates answers`
                             : `${unanswered} press(es) went unanswered`);
process.exit(unanswered === 0 ? 0 : 1);
