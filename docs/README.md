# Docs

Start with [README.md](../README.md): what this is, where it stands, and how
to build it. This page says where everything else is, and the few things
worth knowing before opening it.

## Where to read

| ask it of | because |
| --- | --- |
| [ui.md](ui.md) | what is drawn, by what measure, in what colour, and every press |
| [app.md](app.md) | what the program keeps, what it answers, what is not real yet |
| [PROTOCOL.md](PROTOCOL.md) | what a server would be asked, and what it says back |
| [verifying.md](verifying.md) | how a question is asked of a built page |
| [probes.md](probes.md) | what every probe in `tools/` already asks |
| [tests.md](tests.md) | what every test asks, without a window and without a wire |
| [skiff-notes.md](skiff-notes.md) | what skiff does, and the traps in it |
| [switches.md](switches.md) | the settings rows, and what each still needs |
| [TODO.md](../TODO.md) | what is not real yet |
| [how-mux-was-found.md](how-mux-was-found.md) | the conversation the stack was found in |

## How the tree is built

Two rules hold everywhere:

- **No source says `#if` about a platform.** A module asks the machine once;
  a machine that differs writes an overlay beside it (`opened.cc`,
  `opened_web.cc`), and CMake picks one.
- **One module per file, named for what it is** (`pchat.ui:composer`). The
  file's header says which module it is; only what it exports may be used
  elsewhere.

## A frame, a press

Nothing drawn is kept: the window is told everything again every frame, and
nothing in `src/ui/` holds a thing to say later that it was not told in this
one. Between two frames, `app::screens::before_frame` reads everything asked
since the last frame and says what is true now. A press asks for one thing,
and the answer is drawn in the next frame; nothing changes while a frame is
drawn. Nothing is drawn that answers nothing: a press either asks for
something, or it is not drawn.

## A change, from nothing to done

1. What is true of it is a function, where it can be tested (`src/logic/`).
2. It is said by a test (`test/`, `SAID(group, what)`).
3. It is drawn, and wired to the request it answers.
4. Its colours come from `paints` (`src/ui/theme.cc`) and nowhere else.
5. The machine part is written once, plus an overlay per platform.
6. What can only be seen is read off the page by a probe (`tools/`).
7. It is written down: the paragraph in the right doc, the row in
   `probes.md`.

Nothing is said in a doc the page has not been asked; nothing is pushed
before a check has built it.

## The words

- **the room** -- the whole window: the drawer, the messages, the card.
- **the card** -- the field at the foot, and what is said about the answer.
- **the glass** -- what a probe reads: the page's mirror of the scene.
- **a plate** -- the rounded surface a thing stands on; a press walks up to
  what answers.
- **a stub** -- a press that asks nothing, a field nothing holds, a colour
  written outside `theme.cc`: anything named as though it were there that
  answers nobody.
- **a wire** -- one of the three shapes a conversation is spoken over
  (`chat/completions`, `responses`, `messages`), known by how its base ends
  and by nothing else.
- **an answerer** -- a model on a wire, put into one conversation.
