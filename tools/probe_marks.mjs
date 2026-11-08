import { chromium } from 'playwright';

// What the marks in a line come to on the glass: said in the style they said
// and not said as themselves. A line the sample has both in it is read out
// of the page -- a backticked stretch that is drawn in the monospace face on
// a plate of its own, with no backtick left anywhere -- and a star inside
// one is read back as a star, because nothing inside code is a mark.
//
// Nothing here counts pixels: what the mirror says of a text's runs, which
// is what the page drew, is what this asks.

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

// Every stretch drawn in the monospace face, which is every stretch said in
// code and nothing else on the screen.
const code = await page.evaluate(() => Array.from(
    document.querySelectorAll('#skiff span'))
    .filter((one) => {
      const face = getComputedStyle(one).fontFamily.toLowerCase();
      return face.includes('mono') && one.textContent.trim() !== '';
    })
    .map((one) => one.textContent));
console.log('said in code:', JSON.stringify(code));

// Everything else: what the page says as text, breaks and all.
const says = await page.evaluate(() => document.getElementById('skiff')
    .innerText.replace(/\s+/g, ' '));
console.log('a star said in code kept:', says.includes('p * p'));
console.log('backticks left in it:', says.includes('`'));

await browser.close();
const drawn = code.length > 0 && code.some((one) => one.includes('vector<bool>'));
const kept = says.includes('p * p');
const clean = !says.includes('`');
console.log(drawn && kept && clean
    ? 'the marks are drawn and not said, and what is in code stands as it was'
    : `NOT: drawn ${drawn}, star kept ${kept}, no backtick ${clean}`);
process.exit(drawn && kept && clean ? 0 : 1);
