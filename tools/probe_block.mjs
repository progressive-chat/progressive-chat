import { chromium } from 'playwright';

// What a block of code comes to on the glass: three backticks at the head of
// a line open one and three at the head of another shut it, and what stands
// between them is drawn as code -- in the monospace face, and as it stands,
// so that nothing in it is read as a mark. Neither fence is drawn, nor the
// word said on the first of them.
//
// This one is asked of what the room says before anything is written in it,
// which is where a block is said: the answer already standing in the room
// says one at the end of it. Nothing here counts pixels: what the mirror
// says of a text's runs -- which is what the page drew -- is what this asks.

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

// Every run the page drew that is not empty: what it says, whether it is in
// the monospace face, and whether it sits on a plate of its own.
const runs = await page.evaluate(() => Array.from(
    document.querySelectorAll('#skiff span'))
    .filter((one) => one.textContent.trim() !== '')
    .map((one) => {
      const way = getComputedStyle(one);
      return [one.textContent,
              way.fontFamily.toLowerCase().includes('mono'),
              way.backgroundColor,
              way.whiteSpace];
    }));
const in_code = runs.filter(([, mono]) => mono);
// What the block says is four lines of it, and each of them is said.
const said_lines = ['butter in 0:00', 'eggs in 0:30', 'stir slow 0:45',
                    'rest beaten 3:30'];
const drawn = said_lines.filter((one) =>
    in_code.some(([said]) => said.includes(one)));
console.log('said in code:', JSON.stringify(drawn));

// What the page says of the whole answer: none of the fences, and none of
// the word written on the opening one.
const says = await page.evaluate(() => document.getElementById('skiff')
    .innerText.replace(/\s+/g, ' '));
const no_fence = !says.includes('```') && !says.includes('text butter in');
console.log('read without its fences:', no_fence);

const ok = drawn.length === said_lines.length && no_fence &&
    in_code.length > 0;
console.log(ok ? 'three backticks say a block of code, and no fence of them is drawn'
               : `NOT: in code ${JSON.stringify(drawn)}, no fence ${no_fence}`);
await browser.close();
