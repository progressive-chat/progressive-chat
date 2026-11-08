import { chromium } from 'playwright';

const page_mirror = () => document.getElementById('skiff').innerText;

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);

const read = async (label) => {
  const said = await page.evaluate(page_mirror);
  console.log(label, said.length, JSON.stringify(said.slice(0, 120)));
  return said;
};
await read('start');
for (const [name, x] of [['new', 1250], ['add', 1206], ['system', 1166], ['fork', 1126], ['share', 1086]]) {
  await page.mouse.click(x, 26);
  await page.waitForTimeout(900);
  const said = await page.evaluate(page_mirror);
  // The one answerer this sample's room names, said by the room itself:
  // asked here for a model this page never says, this was `false` for a
  // whole night and looked like a room that would not name anything.
  console.log(name, 'said:', said.length, 'named:', said.includes('Claude Haiku 4'),
              'newchat:', said.includes('New chat'), 'card:', said.includes('Told to every model'),
              'forked:', said.includes('forked'));
}
const end = await page.evaluate(page_mirror);
console.log('tail:', JSON.stringify(end.slice(-400)));
await browser.close();
