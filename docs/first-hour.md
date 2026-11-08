# A first hour in this tree

This is the page to read first. Nothing in it is the only place a thing is
said -- every part of it points at the doc that says it properly -- but it
says what the tree is made of, which end of it to open first, and what is
expected of a change, so that the rest can be read in any order afterwards.

## What this program is

A chat window: conversations down one side, messages down the middle, a card
at the foot to write in, and everything else -- settings, faces, the search,
what every model is told -- in plates over and beside those three. One C++
program, written in C++26 modules, drawn by Skia through
[skiff](https://github.com/MaurerAnt/skiff) on SDL3, and built four ways from
one cmake: a window on a desk, a page in a browser, an APK, and nothing at all
but its tests. Nothing in the tree says which of them it is in.

No back end answers anything yet: what a model would say is written in
`src/app/sample.cc` and read out of it, so that everything around an answer --
how it streams, what it thought, what it stopped to ask for, what it cost --
can be written and asked about before there is a wire to ask. What would go
out and what would come back when there is one is written
(`docs/PROTOCOL.md`, `src/provider/`).

## The map

| where | what it is |
| --- | --- |
| `src/core.cc` (`pchat.core`) | what a conversation *is*: accounts, conversations, messages, which is open |
| `src/config.cc` (`pchat.config`) | what is kept between runs: how it looks, what an answer is asked with |
| `src/logic/` (`pchat.logic.*`) | questions the drawing has of the model, answered by functions that keep nothing |
| `src/app/` | the program between two frames: every request, and its answer |
| `src/ui/` | what is drawn, and how wide and in what colour (`docs/ui.md`) |
| `src/provider*/` | what is said to a wire, and what is read back off one |
| `src/platform/` (`pchat.platform.*`) | what only the machine can do, one overlay per machine |
| `test/` | the parts that answer without a window: logic, protocol, tokens, theme |
| `tools/` | what asks a built page questions, and what puts a build together |
| `web/` | the four files that are a page's own beside the client |

Two rules keep that table short, and nothing in the tree breaks either of
them.

**No source says `#if` about a platform.** Where a machine has to be asked
itself -- to open a place, to carry bytes, to keep something between runs --
the module that asks it is written once and the machine that differs writes an
overlay beside it: `src/platform/opened.cc` for every machine, and
`src/platform/opened_web.cc` for the page, which is the same module by the
same name with different words in it. CMake picks one of them; nobody `#if`s.

**One module per file, named for what it is.** `pchat.logic.formatting`,
`pchat.platform.opened`, `pchat.ui`. The file's first line says which it is,
and what it exports is all that another file may use of it.

## How a frame happens

`src/main*.cc` makes the window and hands it to `app::program`
(`src/app/program.cc`), which keeps two things going at once: what the machine
says happened to it, and what is drawn. Nothing drawn is kept: the window is
told everything again every frame, and nothing in `src/ui/` holds a thing to
say later that it was not told in this one.

Between two frames, `app::screens::before_frame` (`src/app/screens.cc`) reads
everything that was asked since the last one -- presses, requests, what a
stream brought -- and says what is true now. Nothing changes while a frame is
being drawn, which is why a press never argues with what is already painted.

## What a press does

One press asks for one thing (`app::request`, `src/app/requests.cc`), one
place answers all of them (`app::screens::before_frame`), and the answer is
drawn in the next frame. Step by step, with the newest thing in the program as
the example:

1. The drawing is told what a press on a node asks for: a text's link says
   where it goes, and skiff puts it in `scene::hostWork().links` when a press
   lands on it and does not move.
2. Between frames the program reads what gathered there
   (`src/app/screens.cc`) and hands each one to `pchat.platform.opened`.
3. That module asks the machine: `SDL_OpenURL` on a desk and a phone,
   `window.open` on a page (`opened_web.cc`). Nothing anywhere else asks.

The whole table of press to request to answer is in `docs/ui.md`; what the
program keeps, and what it answers, is `docs/app.md`.

## What is expected of a change

- **Nothing keeps a colour of its own.** Every colour is a `paints`
  (`src/ui/theme.cc`), named for what it is for and filled from one scheme, so
  that light and dark are the same drawing.
- **No press that asks nothing.** A control drawn and pressed that changes
  nothing is a stub, and a stub is worse than nothing drawn: it says a thing
  is offered and offers nothing. Either wire it or leave it out -- which is
  why nothing was added to the card beside the field while the clip already
  drawn there asks for what nothing answers yet.
- **What has no window, and answers nothing asked of the page, is said by a
  test.** Anything a function can be asked without a window goes in `test/`
  and is said with `SAID(group, what)`: how a line is read into runs, how a
  body is written for a wire, how much a message is worth in tokens.
- **What can only be seen is asked by a probe.** Nothing inside the page says
  what was drawn, so a question about what was drawn is asked from outside,
  in a browser, by something in `tools/` (`docs/verifying.md`, and what each
  probe already asks is `docs/probes.md`).
- **Every change that can be seen, is.** The four workflows (`check`, `web`,
  `desktop`, `android`) build everything and the page besides; nothing is
  pushed to `main` that a check has not built, and nothing about the page is
  written down that was not read off the build standing on the net.
- **The prose is spoken, not formatted.** Short sentences, the passive kept
  where it keeps the thing that matters in front, no bullet that could have
  been a line of it, and no line that says a thing is easy.

## How a small change is made

Adding the links above took seven steps, which is what any change here takes:

1. What is true of it read once as a function, where it can be tested:
   `links_of` in `src/logic/formatting.cc`.
2. Said by a test: one `SAID` in `test/logic_test.cc`, nine things asked of
   that one function -- thirteen things asked of it.
3. Wired into what is drawn: `src/ui/message_list.cc` gives the text the
   links and lets it be selected, which is what a text pressed for a link
   has to be.
4. Given the colour links are drawn in, out of `paints` and nowhere else
   (`src/ui/theme.cc`).
5. Answered where the answer belongs: the machine, so
   `src/platform/opened.cc` and its overlay for the page.
6. Read off the built page by a new probe, `tools/probe_links_opened.mjs`,
   which says a link is its own `<a href>` in what the mirror says of the
   glass, and that a press on it opened where it says.
7. Written down: the paragraph in `docs/ui.md`, the row in `docs/probes.md`,
   and the same said of the marks beside them.

The order matters only in this: nothing is said in a doc that the page has
not been asked yet, and nothing is pushed before a check has built it.

## Where to read next

| ask it of | because |
| --- | --- |
| `docs/ui.md` | what is drawn, by what measure, in what colour, and every press |
| `docs/app.md` | what the program keeps, what it answers, and what is not real yet |
| `docs/PROTOCOL.md` | what a server would be asked, and what it would say back |
| `docs/verifying.md` | how a question is asked of a built page, and what one can ask |
| `docs/probes.md` | what every probe in `tools/` already asks |
| `docs/tests.md` | what every test asks, without a window and without a wire |
| `docs/skiff-notes.md` | what skiff does that nothing else does, and the two traps in it |
| `docs/how-mux-was-found.md` | how the stack this is built on was found read |
| `README.md` | what it is, where it stands, and how to build it |
| `TODO.md` | what is not real yet, and what the next thing to write is |

One last thing worth knowing before reading any of them: the writing here is
not decoration. A name said wrong in a comment is a name someone will look
for, and a doc that says what the page does not do is a lie told twice, so
what is written down is read off what was built and off the page it built --
never the other way round.
