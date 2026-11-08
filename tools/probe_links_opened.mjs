import { chromium } from 'playwright';

// What a place said in a message amounts to on the page: drawn as itself --
// its own colour, underlined, and an <a> of its own in what the mirror says
// of the glass -- and opened where it goes when a press lands on it.
//
// The message is written by this probe and not read out of the sample,
// because nothing the sample says names a place anywhere.

const url = process.argv[2] ||
    `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`;

// Two messages, both written by this probe: a place said on its own, which
// is a link because it names somewhere, and one written the way writing
// writes one -- what it is called, and where it goes -- which is a link too
// and drawn as nothing but what it is called.

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const context = await browser.newContext({ viewport: { width: 1280, height: 900 } });
// Every tab this page opens while it is asked: what a press on a link comes
// to, seen from outside the page.
const opened = [];
context.on('page', (one) => opened.push(one.url()));
const page = await context.newPage();
await page.goto(url);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(8000);

// The field, written into and said out of with Enter, as a reader would.
const said = 'read https://example.com/now-and-here then tell me';
await page.mouse.click(640, 862);
await page.waitForTimeout(500);
await page.keyboard.type(said);
await page.waitForTimeout(600);
await page.keyboard.press('Enter');
await page.waitForTimeout(2000);
await page.keyboard.type('said at [called this](https://example.com/called-this) then');
await page.waitForTimeout(600);
await page.keyboard.press('Enter');
await page.waitForTimeout(2500);

// What the glass says of the place: its own element, where it goes, and
// that nothing either side of it was taken into it.
const links = await page.evaluate(() => Array.from(
    document.querySelectorAll('#skiff a[href]'))
    .map((one) => [one.getAttribute('href'), one.textContent]));
console.log('said as links:', JSON.stringify(links));

// The one written with stops around it: read back as what it is called and
// nothing of either pair of them.
const said_whole = await page.evaluate(() => document.getElementById('skiff')
    .innerText.replace(/\s+/g, ' '));
console.log('its stops left in the page:', said_whole.includes(']('));

// A press that lands on it and does not move: where it goes.
const where = await page.locator('#skiff a[href]').first().boundingBox();
let pressed = false;
if (where) {
  await page.mouse.click(where.x + where.width / 2, where.y + where.height / 2);
  await page.waitForTimeout(3000);
  pressed = true;
}
console.log('press landed:', pressed ? 'yes' : 'NO LINK TO PRESS');
console.log('opened by it:', JSON.stringify(opened));

await browser.close();
const written = links.some(([href, text]) =>
    href === 'https://example.com/called-this' && text === 'called this');
const drawn = links.some(([href, text]) =>
    href === 'https://example.com/now-and-here' &&
    text === 'https://example.com/now-and-here') && written;
const went = opened.some((one) => one.startsWith('https://example.com/now-and-here'));
console.log(drawn && went ? 'the place said is drawn as a link and opened'
                          : 'NOT: drawn ' + drawn + ', opened ' + went);
process.exit(drawn && went ? 0 : 1);
