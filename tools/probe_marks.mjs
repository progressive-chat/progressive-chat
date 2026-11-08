import { chromium } from 'playwright';

// What the marks in a line come to on the glass: said in the style they said
// and not said as themselves. A message this probe writes itself, because
// what is asked is what the page does with marks and not what the sample
// happens to say: ** around one word, a backtick either side of two more,
// and _ around a last one.
//
// Nothing here counts pixels: what the mirror says of a text's runs -- which
// is what the page drew -- is what this asks.

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

// Written into the field and said out of it with Enter, as a reader would.
await page.mouse.click(640, 862);
await page.waitForTimeout(500);
await page.keyboard.type(
    '**bold** and `code with p * p in it` and _slanted_ and ~~gone~~ and' +
    ' __under__ said');
await page.waitForTimeout(600);
await page.keyboard.press('Enter');
await page.waitForTimeout(2500);

// Every run the page drew of it: what each says, and how each is drawn.
const runs = await page.evaluate(() => Array.from(
    document.querySelectorAll('#skiff span'))
    .filter((one) => one.textContent.trim() !== '')
    .map((one) => {
      const way = getComputedStyle(one);
      return [one.textContent, parseInt(way.fontWeight),
              way.fontFamily.toLowerCase().includes('mono'), way.fontStyle,
              way.textDecorationLine];
    }));
const in_code = runs.filter(([, , mono]) => mono).map(([said]) => said);
const strong = runs.filter(([, weight]) => weight >= 700).map(([said]) => said);
const slanted = runs.filter(([, , , way]) => way === 'italic').map(([said]) => said);
const struck = runs.filter(([, , , , over]) => over.includes('line-through'))
    .map(([said]) => said);
const under = runs.filter(([, , , , over]) => over.includes('underline'))
    .map(([said]) => said);
console.log('said in code:', JSON.stringify(in_code.slice(-3)));
console.log('said bold:', JSON.stringify(strong.slice(-3)));
console.log('said slanted:', JSON.stringify(slanted.slice(-3)));
console.log('said struck out:', JSON.stringify(struck.slice(-3)));
console.log('said underlined:', JSON.stringify(under.slice(-3)));

// And that none of the marks themselves was left anywhere in the line: what
// is read back out of the page is what a reader reads.
const says = await page.evaluate(() => document.getElementById('skiff')
    .innerText.replace(/\s+/g, ' '));
const said_all =
    says.includes('bold and code with p * p in it and slanted and gone and' +
                  ' under said');
console.log('read without its marks:', said_all);
console.log('kept what the code said inside it:', says.includes('p * p'));

await browser.close();
const drawn = in_code.some((one) => one === 'code with p * p in it') &&
              strong.some((one) => one === 'bold') &&
              slanted.some((one) => one === 'slanted') &&
              struck.some((one) => one === 'gone') &&
              under.some((one) => one === 'under');
console.log(drawn && said_all
    ? 'the marks are drawn as what they said, and said as themselves never'
    : `NOT: drawn ${drawn}, said whole ${said_all}`);
process.exit(drawn && said_all ? 0 : 1);
