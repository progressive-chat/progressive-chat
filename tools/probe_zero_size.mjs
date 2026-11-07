// SPDX-License-Identifier: GPL-3.0-only
// What nothing measured: every node of the page whose size came of nothing.
// A node that never measured its words is drawn nowhere and answers no
// press, and it is not always where it can be seen missing -- six chips and
// the fold above an answer were both found this way (docs/skiff-notes.md,
// *What nothing measures reads as nothing wide*). So this asks the whole
// tree, and says what it found, rather than one control at a time.
//
// Run against a page put out by .github/workflows/web.yml:
//   node tools/probe_zero_size.mjs [http://127.0.0.1:8126/index.html]

import { chromium } from 'playwright';

const url = process.argv[2] ?? 'https://progressive-chat.github.io/progressive-chat/';

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`${url}?asked=${Date.now()}`, { waitUntil: 'load' });
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(9000);

// Every node under the page's own tree, said by how it stands: a node with
// no breadth or no height at all, whose parent has some of both, is what
// this is looking for -- what nothing measured inside something measured.
const small = await page.evaluate(() => {
  const out = [];
  const walk = (one, depth) => {
    if (!one) return;
    for (const child of one.children ?? []) {
      const mine = child.getBoundingClientRect();
      const theirs = one.getBoundingClientRect();
      // Put away whole, by the tree itself, is nothing measured at all and
      // no mistake: what the drawer keeps folded, what a card has hidden
      // until it is asked about -- each stands in the page with `display:
      // none` said of it, and is left alone here. What this is looking for
      // is the other kind: still shown, and still nowhere.
      const shown = getComputedStyle(child);
      if ((mine.width < 1 || mine.height < 1) && theirs.width >= 1 &&
          theirs.height >= 1 && shown.display !== 'none' &&
          shown.visibility !== 'hidden') {
        const said = (child.innerText ?? '').replace(/\s+/g, ' ').trim();
        out.push({
          tag: child.tagName,
          width: Math.round(mine.width),
          height: Math.round(mine.height),
          x: Math.round(mine.x),
          y: Math.round(mine.y),
          said: said.slice(0, 40),
          depth,
        });
      }
      walk(child, depth + 1);
    }
  };
  walk(document.getElementById('skiff'), 0);
  return out;
});

console.log(`${small.length} nodes inside something measured, measuring nothing:`);
for (const one of small.slice(0, 60))
  console.log(`  ${one.tag} ${one.width}x${one.height} at ${one.x},${one.y}` +
              (one.said ? ` "${one.said}"` : ''));

await browser.close();
