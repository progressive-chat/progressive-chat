<!-- SPDX-License-Identifier: AGPL-3.0-only -->

# How a change here is known to be right

Nothing in this program can be built on the machine it is written at: it
wants `clang` not older than 23, and what is here has none. So every change
is built elsewhere -- by a workflow in this repository's own actions -- and
known to be right only once something has read the page it builds and said
what it saw. This is what that whole road looks like, written down as it is
walked, because none of it is written anywhere else and each step of it was
learned once by getting it wrong.

## The four workflows, and what each of them answers

- `check` -- build, tidy and tests. It answers nothing about the page: it
  says only that what was pushed is said in the language's own way and that
  what is tested is still so. Ask for it before anything reaches `main`.
- `web` -- the browser build. Its minutes end with an artifact named
  `github-pages`, which is the whole page as a server would keep it.
- `probe` -- never run by a push. A node script drives the *deployed* page
  and says what it saw; a script written by hand can be given to the run to
  ask one question without a push or a build at all.
- `desktop`, `android`, `crash` -- the other two builds, and sanitizers.

A change made on a branch of its own is asked for like this:

```sh
gh workflow run web.yml -R progressive-chat/progressive-chat --ref <branch>
```

and the run is read either from that URL or with

```sh
gh api repos/progressive-chat/progressive-chat/actions/runs/<id> \
  --jq '.status, .conclusion'
```

Note for whoever asks: `--ref` names a branch, not a commit, and the run
builds what the branch says *then*. Anything committed after the run was
asked for is not in it, which is one good reason not to ask twice about two
things.

## Reading what a run said, when it said no

`gh run view --log` and `gh run download -n job-logs` do not give what is
wanted here. What does is the whole log, which is a zip of its own:

```sh
gh api repos/progressive-chat/progressive-chat/actions/runs/<id>/logs > logs.zip
python3 -c 'import zipfile; zipfile.ZipFile("logs.zip").extractall("logs")'
```

Then read `logs/` for the first `error:`. Most failures are compile errors,
and most compile errors here were an edit typed from memory into a file
whose surroundings had moved on: read the file, then write the edit.

## Having the page at hand

The artifact is a tar of its own inside the zip that `gh run download`
makes, so:

```sh
gh run download -R progressive-chat/progressive-chat <id> -n github-pages -D art
tar xf art/artifact.tar -C /home/user/page   # whatever it is served from
python3 -m http.server 8126 --bind 127.0.0.1 --directory /home/user/page
```

One page kept that way can be asked anything, as often as wanted, and no
question costs a build. `index.html` and its `index.js`/`index.wasm` are all
of it; the probes in `tools/` ask it of the deployed page instead, which is
what they are written for and why their URL is swapped for `127.0.0.1:8126`
when they are driven here.

## What a probe is, and what makes one answer

The page paints nothing until it is told to, and says so when it has:

```js
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(2500);
```

The wait is not politeness: the tree is filled from `again()` and only then
laid out, and the mirror writes what it read. Two and a half seconds is
enough for the first frames; six is safer when nothing has been asked before.

The browser needs swiftshader here, or it is told `--disable-gpu` and asks
Skia for GL that is not there, and the program goes before it paints:

```js
chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader',
         '--enable-unsafe-swiftshader'] });
```

Two things are read off a page: what it says, and where every box stands.

- What it says is `document.getElementById('skiff').innerText`, or -- when
  one node is wanted and not the whole column -- the own text of each
  element, which is its text nodes joined and never its children's.
- Where things stand comes from `getBoundingClientRect()` on each element of
  `#skiff`. The mirror writes `data-skiff-h="N"` on every element, which is
  the same number the app's own scene knows the node by, so a box read out
  of the page can be talked about in the app's terms too.

## Mutations are the only honest answer

"Something happened" cannot be read out of a console line: this program
prints little, and what it prints it prints once. What is read instead is
how much of the tree the browser rewrote:

```js
await page.evaluate(() => {
  window.__mut = 0;
  window.__obs = new MutationObserver((all) => { window.__mut += all.length; });
  window.__obs.observe(document.getElementById('skiff'), {
    childList: true, subtree: true, characterData: true, attributes: true });
});
await page.mouse.click(x, y);
await page.waitForTimeout(1200);
const how_much = await page.evaluate(() => window.__mut);
```

Zero means the press asked nothing, whatever the design says of the press.
Zero in the middle of a button whose left edge answered is the shape of the
bug written up in `docs/skiff-notes.md` under *What nothing measures reads
as nothing wide*: a node that grew to no size of its own is pressed nowhere
but in the padding around nothing.

When even that is not enough to say where the press went, patch `ccall` and
read the pointer call itself -- which node the browser decided was under the
pointer, and where in it:

```js
await page.evaluate(() => {
  const was = Module.ccall;
  Module.ccall = (...all) => {
    if (/pointer|web_event/.test(all[0])) console.log(all[0], all[2]);
    return was(...all);
  };
});
```

## Two coordinates, two frames

Two things are never the same number and nothing has gone wrong:

- The *scene* and the *document* keep different y for the same box. The
  mirror maps per element -- a press inside an element is said as that
  element's scene place plus how far into it the pointer was -- so both are
  right, and only mixing one for the other is a mistake.
- A value printed from inside `again()` is read *before* the layout that
  answers it; the same value read in a `before_frame` hook is the layout of
  the frame before. Nothing prints its own layout before it has one.
- The page moves once, and not when you think: the column beside the
  messages does not keep the breadth it began with, so everything in the bar
  over them slides and grows somewhere in the first few seconds. A point
  read at four seconds is a point in nothing at six. So nothing here waits a
  fixed time for a layout it means to press on: `tools/probe_top_bar.mjs`
  reads where the plates stand every second until two readings say the same
  thing, and presses after that. A probe that presses a point written down
  from another run is how a whole afternoon was spent proving a working
  press was broken.

## A probe kept in the tree

What was learned once is kept as a probe in `tools/`, and `tools/probe_fold.mjs`
is the one written for the fold above an answer: it finds the line by the
words in it rather than by a point written down, presses it in the middle,
and says whether what the model thought was read and then folded away again.
Every probe there can be run by the `probe` workflow without a push, or
here against a page served as above, and each says what it saw in one line
so that a run's log is its own answer. `tools/probe_fold_calls.mjs` asks one
thing of such a line pressed open: that it names each call the line counts
while it is open, and none of them before it was pressed or after it was
pressed shut. `tools/probe_top_bar.mjs` is the one
that asks whether every press in the bar answers: it finds each plate by
its size and place rather than by a point, presses each in a page of its
own -- pressing one asks for something that changes what the next is
pressed over -- and takes as evidence either that something else was said
or, for the press that gives a conversation away, that what was held to be
pasted changed.

## Every square, pressed once

A press nothing answers is either not a control -- a word, a field's own
hint -- or a stub standing where something should be, and only asking tells
which. `tools/probe_presses.mjs` asks of all of them: it reads every square
of a control's size that is whole inside the room, presses each one in a
page of its own, and says which answered and which took the press and did
nothing with it. Read against the tree it is a list of what to look at next,
and read again after a change it says whether the change reached the page.

What it found the first time it was run, on 2026-11-07: thirty squares, and
five answering nothing -- the word at the head of the drawer, and four in
the card at the foot of the messages: the ring that says how full a model's
window is, the clip, the face, and the round button beside the field, which
sent nothing though a message was written in the field beside it. That last
one sends now, and empties the field as Enter does; the ring is said and not
pressed, which is what it is for, and the clip and the face ask for what
nothing answers yet, and are said so in `TODO.md`.

## Press what you mean to press

What takes a press in a place is what was painted last there, and not what
was read there. The six chips under an answer are a row of small plates
with their marks and words painted over them: the words of one are read at
a point where the *mark* of another is what a press lands on, and a probe
that pressed the middle of every chip's words pressed something else each
time and came back saying a chip that works answers nothing. So: before
pressing a point read off an element, ask `document.elementFromPoint` what
stands there, and press only when it is that element or something in it --
`tools/probe_presses.mjs` says `covered` for the rest. A mark is no
exception: the middle of a chip is its icon, and that is the chip's own to
climb to, so a press there is right and a press four pixels off it may be
the plate beside it.

A last one before anyone writes another probe: what says a press was
answered is `skiff_web_event` with a node's handle and `click`. The pointer
being said to something means nothing -- it is said even of a press made in
empty air -- and a probe that counts those has proved a press that never
happened (`docs/skiff-notes.md`, *What the page says was pressed is what it
painted last*).

Nothing here is tested by anything but these. What is not asked of a page
has not been said at all.
