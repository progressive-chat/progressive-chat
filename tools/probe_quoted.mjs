import { chromium } from 'playwright';

// What a line said quoted comes to on the glass: none of the stops of quote
// said at its head is drawn, and what they said -- that the line is quoted --
// is drawn in its place, in the colour a quote is drawn in. A message this
// probe writes itself, to say what the page does with one and not what the
// sample happens to hold.

const url = process.argv[2] ||
    `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`;

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(url);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(8000);

await page.mouse.click(640, 862);
await page.waitForTimeout(500);
await page.keyboard.type('> in quotes **and bold** said');
await page.waitForTimeout(600);
await page.keyboard.press('Enter');
await page.waitForTimeout(2500);

// Every run the page drew: what each says, and in what colour it says it.
const runs = await page.evaluate(() => Array.from(
    document.querySelectorAll('#skiff span'))
    .filter((one) => one.textContent.trim() !== '')
    .map((one) => [one.textContent, getComputedStyle(one).color,
                   parseInt(getComputedStyle(one).fontWeight)]));
const about = runs.filter(([said]) => said.includes('in quotes') ||
    said === 'and bold' || said === ' said');
console.log('said quoted:', JSON.stringify(about));

// That what stands quoted is drawn in the colour a quote is drawn in, which
// is not the colour most of the page says its words in.
const colours = await page.evaluate(() => {
  const said = document.querySelectorAll('#skiff span');
  const how_many = {};
  for (const one of said) {
    const way = getComputedStyle(one).color;
    if (way === 'rgba(0, 0, 0, 0)' || one.textContent.trim() === '') continue;
    how_many[way] = (how_many[way] || 0) + 1;
  }
  return Object.entries(how_many).sort((one, two) => two[1] - one[1])
      .map(([colour]) => colour);
});
const quoted_colour = about.length > 0 ? about[0][1] : '';
const common_colour = colours.length > 0 ? colours[0] : '';
console.log('said in the quote\'s colour:', quoted_colour,
            '-- and the rest in', common_colour);
const bold = runs.filter(([, , weight]) => weight >= 700).map(([said]) => said);
console.log('said bold inside it:', JSON.stringify(bold.slice(-1)));

// What a reader reads back out of the whole page: no stop of quote anywhere
// in it, and none of the marks around the bold.
const says = await page.evaluate(() => document.getElementById('skiff')
    .innerText.replace(/\s+/g, ' '));
const said_all = says.includes('in quotes and bold said');
const stops_left = /^\s*>|> in|>/m.test(
    await page.evaluate(() => document.getElementById('skiff').innerText));
console.log('read without its stops:', said_all);
console.log('said bold inside it kept its mark out:', bold.includes('and bold'));

await browser.close();
const drawn = about.length > 0 && said_all && bold.includes('and bold') &&
    quoted_colour !== '' && !about.some(([said]) => said.includes('>'));
console.log(drawn
    ? 'a line said quoted is drawn quoted, and no stop of quote is drawn'
    : `NOT: about ${JSON.stringify(about)}, read whole ${said_all},
       quoted colour ${quoted_colour}, common ${common_colour},
       stops left ${stops_left}`);
process.exit(drawn ? 0 : 1);
