# The half that is not drawn

`docs/ui.md` says what is painted and by what measure. This says the rest:
what the program keeps, what a press asks it for, what it answers between
two frames, and where nothing is real yet.

## The layers

```
what a press asks for  -->  app::request         (src/app/requests.cc)
what answers it        -->  app::screens         (src/app/screens.cc)
what it keeps          -->  chats_root, config   (src/core.cc, src/config.cc)
what it draws          -->  pchat.ui             (src/ui/, see docs/ui.md)
what it says to a wire -->  pchat.provider       (src/provider*.cc)
what the machine gives -->  pchat.platform       (src/platform/)
```

Nothing reaches across two lines that are not joined here. The drawing never
keeps anything of its own to say later -- it is told everything again every
frame -- and the program never asks the drawing about itself. A press, a
request, the answer to it: for what that is step by step, `docs/ui.md`.

## What is kept

`chats_root` (`src/core.cc`) is the model: the accounts, the conversations,
the messages, and which one is open. Everything in it is typed -- ids are
strings compared and never read into parts, a moment is milliseconds since
the epoch, delivery is one of five, a conversation is direct, group or
channel. Nothing past the place a protocol comes in looks at text again.

`pchat.config` is what is kept between runs, or would be: `looks` (the
preset, the style, light or dark or the machine's own, how far type is
scaled up, whose faces are drawn in) and `asking` -- everything an answer is
asked with: whether it thinks, how hard, how far it may wander, the ceiling
on how long it may be, the three tools it may stop to ask for, and the seven
switches nothing has answered yet. `settings_root` keeps the two together,
and a list beside them: the emoji pressed most lately, kept for no rows to
show them yet.

## What a press asks for

One request per thing asked (`src/app/requests.cc`), and one place that
answers them all (`app::screens::before_frame`, `src/app/screens.cc`): what
is open, what is said, what is searched for, what model answers, what every
answer is asked with, what is copied out, and a conversation forked out of
the messages up to one answer. The whole table of press to request to answer
is in `docs/ui.md`.

Two things are said of that answer worth saying twice: it happens between
frames and not during one, so a press never changes what is already being
drawn; and what is open is said to the window as one thing (`what_is_open`,
`src/ui/window.cc`), six rights and wrongs in a row rather than six words
passed one beside the other.

## What is said where nothing is asked

`src/logic/` answers questions the drawing has of the model, and nothing
else: which conversations a search names (`search`), what one row of them is
called (`lists`), what a message's text is as the runs it is drawn in
(`formatting`), what the design's own words are (`doc`), what the field holds
and what sending it makes (`composer`), and what was being written to each
conversation when its reader left it (`drafts`).

Which of them are asked: every line the room draws is read once by
`formatting` -- its marks taken out of it and said as the runs they meant,
every place named in it or written in it as writing writes one said as a link
that opens where it goes -- and `lists` is asked what the field in the drawer
names. The rest is as it was written.

`src/tokens.cc` says how much room what was said takes in a model's window,
and how much it is worth; `src/tools.cc` says which three tools are this
program's own to answer and what they are called on a wire, so that a row in
the settings and a body written for that wire name the same one.

## What is said to a wire

`pchat.provider` writes what would go out and reads what would come back,
and opens nothing at all: `turn`s gathered out of a conversation, a `turn`
for what every model is told, one body written in one of three shapes (the
chat one everyone answers, the responses one some do, Anthropic's own --
`wire_of` says which by the base's ending and by who answers), the tools
offered on the way, and the place it is sent.

Around that: `run` takes one answer from asked to over, `stream` reads an
answer arriving a frame at a time into text, thought, tool calls, what it
cost and why it stopped, and `loop` asks again as many times as a model
stops to ask for a tool until it stops asking, something fails, or it has
been asked as many times as it may. Each of them is said by a test with no
network under it (`test/provider_*_test.cc`), which is the only way a
protocol is worth writing at all.

What carries the bytes is the machine's: `pchat.platform.carry`. Only a page
carries anything today, through the browser's own fetch, and everywhere else
`carrying::any()` says so and nothing is asked. The place it is asked at is
written into the program whole today (`src/app/screens.cc`, `opened_`): the
server that served the page, at `/v1`, with no key in it -- whose a key is,
that server's own to know.

## The machine underneath

- `notified` puts one notification where the machine puts notifications: on
  Android, Android's own; everywhere else nothing, and it says which.
- `kept` keeps from one run to the next the GPU programs Skia compiles, which
  a phone compiles slowly enough to pay twenty seconds for before the first
  frame; where nothing can be written, the page, it is empty.
- `carry` is above: one place, its headers and a body in, bytes out.
- `drawn` says whether this machine can be drawn through its GPU at
  all, asked once before the window is made, of a window nobody will see.

## What is not real yet

Nothing counts anything for real: no answer has come from a wire in any of
the three builds, so tokens, tool calls and how long an answer took are all
said by `src/app/sample.cc`, which answers for the back end that is not
written. What a page would ask of one is written (`docs/PROTOCOL.md`), and
what would be asked back is read (`pchat.provider.stream`); who carries it
outside a page is the only part left out.
