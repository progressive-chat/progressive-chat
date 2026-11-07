// SPDX-License-Identifier: GPL-3.0-only
// What is written in the field belongs to one conversation: what was being
// written when its reader left it is kept to it and comes back when that
// reader does, and nothing written for one is read in another.
//
// What the page keeps of a field is said in a hidden input beside it, and
// that is what this reads: the field's own words, without asking the room
// what it drew.
//
// Run against a page put out by .github/workflows/web.yml:
//   node tools/probe_drafts.mjs [http://127.0.0.1:8126/index.html]

import { chromium } from 'playwright';

const url = process.argv[2] ?? 'https://progressive-chat.github.io/progressive-chat/';

const fields = () =>
    Array.from(document.querySelectorAll('textarea, input'))
        .map((one) => `${one.getAttribute('aria-label') || one.type}:${one.value}`);

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`${url}?asked=${Date.now()}`, { waitUntil: 'load' });
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(2500);

const read = async () => await page.evaluate(fields);

// Written to the first conversation, and not sent.
await page.mouse.click(640, 862);
await page.waitForTimeout(700);
await page.keyboard.type('written to one conversation');
await page.waitForTimeout(900);
const written = await read();
console.log('written:', JSON.stringify(written));

// Another conversation opened: what was written stayed with the first.
await page.mouse.click(180, 380);
await page.waitForTimeout(1200);
const other = await read();
console.log('in another:', JSON.stringify(other));

// And back again: it comes back with it.
await page.mouse.click(180, 300);
await page.waitForTimeout(1200);
const again = await read();
console.log('back again:', JSON.stringify(again));

// Sent, and the conversation it went to has nothing kept of it: away and
// back once more, the field is empty.
await page.mouse.click(640, 862);
await page.waitForTimeout(500);
await page.keyboard.press('Enter');
await page.waitForTimeout(1200);
const sent = await read();
await page.mouse.click(180, 380);
await page.waitForTimeout(900);
await page.mouse.click(180, 300);
await page.waitForTimeout(1200);
const after = await read();
console.log('sent:', JSON.stringify(sent), '-> back again:', JSON.stringify(after));
console.log('kept: written once, gone in another, back here again, sent and',
            'kept no more:',
            written.some((one) => one.endsWith(':written to one conversation')) &&
            other.every((one) => !one.includes('written to one')) &&
            again.some((one) => one.endsWith(':written to one conversation')) &&
            after.every((one) => !one.includes('written to one')));
await browser.close();
