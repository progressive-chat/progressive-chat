# The words this tree says things in

This program is written in a small vocabulary of its own, and most of it is
ordinary words said in an ordinary way rather than terms made up for a
program. Nothing here is decoration: each of these is said the same way
everywhere in the tree, and each is *only* that thing. Where a word here is
also a word the code uses, they are one word.

Read this beside [first-hour.md](first-hour.md), which says where everything
stands before this says what each thing is called.

## What things are

- **said** -- what something says when it is asked, and nothing about how it
  came to say it. `drawn_of`'s `said` member is a message's text with its
  marks taken out of it; a probe prints what the page said back; a commit
  message says what the change does. Nothing "returns", "yields" or
  "produces" in this tree where it can be said instead.
- **asked** -- what a thing is asked, whether it is a model asked for an
  answer, a node asked for its measure, or a field asked what it holds. Also
  **`request`**: a request the model keeps and the screens read, which is the
  only thing that carries anything between them.
- **what is kept** -- the whole of what the program holds between frames (the
  conversations, their messages, the looks, the account), as opposed to what
  one node says of itself for one frame.
- **the room** -- the whole window: the drawer down one side, the messages
  beside it under the floating bar, and the card to write in at the foot.
  Three things are *pages* rather than panels because they are drawn over the
  whole of it: the settings, the design, and what every model is told.
- **the card** -- what stands at the foot of the room: the field, and beside
  it everything said about the answer that is coming.
- **the glass** -- what a probe reads. Named for what stands between what
  the program drew and whoever asks about it, which is also why nothing about
  the drawing is ever asked of the program itself.

## The four shapes of an answer

- **a model** -- one that answers, known by a name a wire knows it by.
- **a wire** -- one of the three shapes a conversation can be spoken over
  (`chat/completions`, `responses`, `messages`), known by how its base ends
  and by nothing else.
- **an answerer** -- a model on a wire, put into one conversation (`config`),
  so that one conversation may be answered by one and another by another.
- **what the model keeps** -- everything said to it and by it so far, which
  is what a wire is handed each time, and what `src/provider.cc` writes into
  a body.

Carrying is what goes out and comes back: nothing in this tree says
"request"/"response" of an exchange with a wire where it can say what was
**carried** (`pchat.platform.carry`), so that asking and answering are one
word and not two, and nothing about either is written twice.

## The marks of a message

- **a mark** -- two stars, one star, `_`, a backtick, two tildes, two bars,
  or one link written the way writing writes one. Marks are what a model
  writes; nothing in the drawing ever looks at one.
- **a run** -- a stretch of a line that one set of marks meant: what it says,
  the bits of style it carries, where it stands in what is drawn, and how
  deep the line it stands in is quoted.
- **said whole** -- a mark taken out and what it meant said in its place. A
  line is never drawn with its stars and backticks in it.
- **quoted** -- a line said behind stops of quote at its head: drawn in the
  colour a quote is drawn in, and none of the stops drawn anywhere.
- **a place said in a line** -- either a bare target a model wrote or one
  written in square stops with where it goes beside it in round ones: both
  are drawn as a link that opens where it says, and neither is drawn as it
  was written.

## The drawing

- **paints** -- every colour a part asks for, named for what it draws:
  `paper`, `text`, `dim`, `card`, `field`, `thought`, `link`, `badge`. Only
  `src/ui/theme.cc` says one, and no colour is written anywhere else in the
  tree.
- **the looks** -- an eight-way choice of seed, and whether that seed is
  said light or dark, and how large the type is, and whether the black is
  blacked out for an AMOLED screen: all of `src/theme*.cc` and `config`.
- **a chip** -- one of the six things under an answer, said as a pill with a
  word in it: copy, ask again, fork, give away, about, rub out. Each is a
  press that answers something.
- **the fold** -- what stands above an answer that was thought about first:
  its thinking behind one line, and every call it stopped to ask for under
  that, said only while the fold is pressed open.
- **faces** -- what stands above the card behind its own face: twenty-eight
  of them in four rows of seven, and the seven pressed lately kept above the
  rest of them.
- **the gauge** -- the ring over the card that says how full the model's
  window is of this conversation.
- **a plate** -- the rounded surface something stands on, which is also what
  a press walks to reach what answers it.
- **the mirror** -- what skiff writes into the browser's page for every node
  drawn, which is what a probe reads and not quite what was drawn
  ([skiff-notes.md](skiff-notes.md)).

## What is asked of it

- **a press** -- one mouse press somewhere in the room, which reaches what
  was painted last at that square, and walks up from it to what answers for
  it. Nothing is pressed that answers nothing.
- **a stub** -- a press that asks nothing, a field nothing holds, a colour
  written outside `theme.cc`: anything named as though it were there that
  answers nobody. `TODO.md` keeps them until they ask something.
- **never** -- what is said instead of a false generality, because something
  happening every time this tree looks is what can be said of it. "Nothing
  is drawn twice" is a claim that either holds or does not; "usually" is not
  one.
- **nowhere** -- the same said of a place: no colour is written in
  `src/ui/` but in one file, and no test opens a wire, and no commit says
  "we".

## Three names said of anything new

Anything added here is said in three places at once, and if one of them is
missing it is not in: **what it says** (its own file header, in one or two
lines, said as what it is for), **what is asked of it** (a test in `test/`,
or a probe in `tools/` where it is only visible on the glass), and **where
it is read about** (one of these docs, in one sentence that says the same
thing the header says).
