# The design of progressive-chat

This document is the design of progressive-chat: how it should be, focused on
what can be seen — the repo's own look (how the tree is laid out, the README,
and the docs that say where everything is), and the app's screen: the room, the
drawer, the messages, the answer, and the settings. It is written once so that
what exists can be compared against a reference. It covers this program's own
design — not the chat protocol's, and not another program's.

Everything here is drawn by the same layout tree for all three target platforms,
and no measurement in this document belongs to a single one of them: the room has
a width, and the content inside it takes a share of that width.

## The room

One conversation fills the room. Three parts are always present: the top bar,
the message area, and the footer.

- **Top bar** — floats over the scrolling content below. It contains: the button
  that opens the conversation drawer (where a drawer can be opened at all), the
  name of the current conversation, and on the other side two items — a **plus**
  that starts a new chat, and next to it **the conversation's token count** (e.g.
  `12.4k tokens`), which is the total of what the model was asked and what it
  answered. Nothing else is in the top bar. It has no search of its own; the
  drawer provides search.
- **Message area** — between the top bar and the footer, where messages scroll.
- **Footer** — the editor, where text is written. It always shows **which model
  will receive the message**, even before anything is written, and next to that a
  **circle showing how full the context is** — filled by the share of the model's
  window this conversation has consumed, empty where there is room left. Both are
  always present: writing to a model without knowing which one it is, or how close
  to the limit the conversation has come, is writing blind.

### Top-bar items beyond the plus and the token count

- **Model bubbles for the answering models** sit on the top edge of the main
  panel, one per bubble. The name in a bubble is **the model's name**, and
  pressing a bubble selects which model answers and shows which one has been
  used. The portion of the model's window this conversation has consumed is
  **not** repeated in every bubble — it is shown once, next to the editor
  (**The editor's row**), because otherwise every answering model would repeat
  the same count three times.
- **A button for the system prompt**: the instructions given to every model
  before anything else, e.g. `You are answering in Progressive Chat, briefly and
  plainly.` — read and edited here. This sits next to the plus; the top bar is
  not just a plus.
- **Fork conversation** and **share** also live in the top bar: the first makes a
  new conversation branched from this one, the second shares it.
- **When the screen is narrow**, four of these move behind **three dots** next to
  the plus: **search**, **system prompt**, **fork conversation**, and **share**.
  On a wide screen they stand as themselves; on a narrow one they are one press
  deep.

## The drawer

The conversation list overlays the messages when the room is too narrow to keep
it beside them, and stands beside them otherwise. It is never full-width: a strip
of the messages stays visible beside it, and pressing that strip closes the
drawer.

Contents, in order from top to bottom:

- **Conversations** — the panel heading, large and bold, set further in from the
  panel's edge than the items below it.
- **Search conversations...** — the only search. Its own rounded ground (rounded
  all the way around), a magnifier icon before the placeholder, and the
  placeholder in grey.
- **Tasks** and **Remote** below it, only a few pixels apart: both share one
  ground, rounded between them so the two read as a single row split in two —
  the upper one rounded above and barely below, the lower one barely above and
  rounded below.
- **+ New Chat** — its own ground, fully rounded, like the search field.
- The conversation rows, one per conversation.
- **Settings** — the panel's last row: a gear icon and its label on the same
  ground as Tasks and Remote, rounded like the search field. This is the only
  entry into settings, because nothing else in the room has space for it.

A conversation's row shows **its name**, **the last thing said**, and an **unread
badge**. It does not show how long ago that was: a timestamp is something a row
can be asked for, but it is not shown by default. The right side of a row reserves
three times the space of the left, so a name read to its end does not run into
the badge.

By default, a conversation has no avatar — no letter, no picture. Where one is
enabled, the row's circle is placed before its name, and everything after it
shifts right by the circle's width.

## A message you wrote

Your messages are **bubbles**, placed on **the right** of the room. A bubble can
be at most **three-quarters of the room's width**; anything shorter is only as
wide as its text, not stretched to fill.

Two things indicate a bubble is yours — either alone is sufficient, and together
there is no way to misread it:

- it is on the right, against the room's edge, while everything else is read from
  the left;
- **its bottom-right corner is squarer than the other three**, which are all
  rounded alike. This corner is the tail: the bubble points toward whoever wrote
  it. A model's answers, having no bubble, cannot have one.

Below your text is the timestamp, small, in the bubble's own color, aligned
right. No name is shown above your bubble: the two indicators above already say
whose it is.

## A model's answer

A model's answer has **no bubble**. It spans the full width of the room, text on
the background, no wider than the room. No nickname above it, and no model name
or id either: which model answered is shown in the editor, once, for everything
that follows. Nothing of an answer is placed on the right.

### Folded above it

Folded above an answer: **how long it was thinking and how often it called a
tool** — `Thought for 4.2 s · 3 tool calls`. The whole line is **one press**, and
pressing it **opens the fold**: what the model wrote to itself while thinking is
shown below in greentext, and beneath that **every tool call it made along the
way**, one line each — what it requested and what it received — since those three
are what the count refers to. Pressing again folds it back.

### The editor's row

The bottom of the main panel is a single row, read left to right, and nothing
else in the panel is a plus:

- **A plus at the panel's top right starts a new chat.** That is the only plus in
  that part of the screen. Nothing beside it adds an answerer: which models
  answer this conversation is configured under **Services → Models** in settings,
  and nowhere else.
- **A plus at the editor's left, before the model name**, attaches something to
  the message. Pressed, it offers four things: **photo or media from the
  camera**, **photos from the gallery**, **videos**, and **files**.
- **The model name** comes next, shown as **the model's own name** followed, in
  brackets, by **the provider it is asked through**: `DeepSeek V4.1 Flash (ds)`.
  The wire id given to the provider — `deepseek-v4-1-flash` — is not shown here.
- **A circle next to the name** shows **how much of this model's window the
  conversation has consumed**, filled clockwise from twelve o'clock. Nothing
  beside it repeats the same count in words: a conversation's share of a window
  is shown once on screen, not once per answering model.
- **Three dots after the circle** open one menu containing: **thinking**, on/off,
  with **the thinking effort** shown beside it (`max`); **web search**, on/off;
  **compact context**, which is a disabled grey control while the chat is empty;
  and **Advanced**.
- **Advanced** opens the **large settings**: **context window size**,
  **temperature**, **maximum output tokens**, **top p**, **frequency penalty**,
  and **presence penalty** — the same five that settings keeps under
  **Responses → Generation**, reached from where they are used.

### What the fold shows

Above the answer, and only where there is something to fold, is what the model did
before answering: **how long it took**, **how many tool calls it made**, and —
folded inside that line — **its thinking, as greentext**: what it wrote while not
yet answering, in its own green, which is the color it keeps when the fold is
open. Folded, it is one line with nothing read under it; unfolded, the greentext
appears below it, and the answer beneath moves down accordingly.

### Under it

An answer has a **footer** — a row of small controls on its left:

- **copy** — the answer's full text to the clipboard;
- **regenerate** — the same answer requested again, replacing this one;
- **fork** — this answer taken as its own conversation, the two diverging from
  here;
- **share** — the answer shared as a link or standalone text;
- **info** — what answered it, how long it took, its token cost, and when: the
  numbers the design otherwise hides;
- **delete** — the answer removed, leaving the one asked above it in place.

These are always visible when an answer exists, whether or not the pointer is
over them: what an answer can be asked for should be visible before it is
looked for.

## The settings

One panel with **eight groups**, in this order. Each group contains what this
program must be able to agree or refuse, and to what degree.

**Services**

- **Providers** — API keys, the base URL each is asked at, and which providers
  this program talks to at all.
- **Models** — which models are enabled, which are disabled, and how each is
  configured. Under this row: every offered answerer (`pchat.config`), a switch
  for each, and the currently open conversation is the one a press puts a model
  into or takes it out of: the first is what answers when nothing else has been
  said, and the last is never removed, because no conversation can be said to
  have no answerer.

**Responses**

- **System prompts** — the instructions given to every answer before it is
  asked, created and stored here.
- **Generation** — the parameters an answer is asked with, and their current
  values: **temperature**, **top p**, **maximum tokens**, **presence penalty**,
  and **frequency penalty**, each adjusted one step at a time by a control
  beside it and kept within its range (`0…2`, `0…1`, `256…32768`, `-2…2`,
  `-2…2`).
- **Context** — what is kept from a conversation when the window is smaller than
  the conversation.
- **Title generation** — naming a conversation automatically.

**Multimodal**

- **Image transcription** — a vision model asked to describe a picture, so what
  cannot see can still be told. Two switches stand under this row: **read a
  picture into words** and **make a picture out of words**. When one is off it is
  not offered at all.
- **Image generation** — pictures produced from written descriptions.

**Tools**

- **Web search** — one search tool, configured once, shared across all providers.
- **Conversation search** — the conversation's history and how it is searched.
  Three switches stand under this row: **look up what was said here**, **say how
  much has been said**, and **say what time it is**. When one is off it is not
  offered to a model at all.
- **Shell** — commands run on a remote machine.
- **Tor** — requests routed through the Tor network. *Where this belongs is
  undecided:* it is a tool here, but it could equally be under Network or under
  Services next to what routes requests.
- **MCP** — external tool servers this program connects to.
- **Automation** — tasks, loops, and their scheduling precision.
- **Interaction** — questions the assistant asks back.

**Network**

- **Proxy** — all requests routed through an HTTP or SOCKS proxy. Whether a
  proxy is used at all is a switch under this row: **speak through another
  program**. It is off by default because a proxy nothing routes through is
  pointless.
- **WebUI** — this program driven from a browser on another machine.

**Memory & Data**

- **Memory** — what is remembered while running, and what is stored in files.
- **Skills** — instruction files that can be given to the model.
- **Data control** — everything of yours exported as a file or taken back. What
  is retained at all is a switch under this row: **keep what has been said
  here**, **keep what it has been taught**, and **keep what has been given to
  it**.

**Notifications**

- **Test** — one notification of this program's own, shown by the host machine:
  a press requests it, and what is requested is a test, not a message. On
  machines where an app may only show notifications after being allowed (Android
  13+), the first press requests permission and shows nothing, and the next
  press shows one. Nothing behind this row stores anything: there are no
  notifications yet.

**Appearance & Language**

- **Appearance** — the theme, its color scheme, whether colors follow the
  system's, and the drawer's row options: whether a circle appears beside names,
  and whether a row shows how long ago its content was said. Both are off by
  default.
- **Language** — which language the program itself uses on screen.

**About**

- **About** — what this build is: its number and source commit, the license it
  is distributed under, and where its source is kept. Every line is copied
  whole to the system clipboard by pressing it.

## What this design does not do

- It does not say "user" or "model" anywhere on screen. The side indicates it,
  the bubble indicates it, and the editor shows which model.
- It does not give a model's answer its own background color, nor does it put the
  answer's text inside a border. Answers are read as plain text; only yours are
  held in a shape.
- It does not place an answer's timestamp next to every line: only your messages
  keep a timestamp, and it is kept inside them.
- It does not expand what it folds. The line above an answer — how long it took,
  how many calls it made — is written as one line and stays one: the thinking
  beneath it and the calls it counts are not read anywhere yet.
- It does not yet count anything for real. Until the program's own back end
  exists, what fills the room is sample data: the numbers above are written into
  it like any other words, and nothing measures them.
