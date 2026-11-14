# The seven switches

The settings page offers a row to each of seven things, and each row says
whether that thing is offered at all. This is what they are, what is kept when
one is moved, and -- said plainly, because it is easy to read the page and
think otherwise -- **what each one would still have to be given before it does
what its row says it does**. As of 2026-11-14 one of the seven is read beyond
the row that draws it; the other six are read by the row alone.

## What one of them is

| where | what it is |
| --- | --- |
| `src/config.cc`, `enum class switch_choice` | the seven, named once, so a row and what the program keeps say the same thing |
| `src/config.cc`, `asking::switches` | whether each of them is offered, seven `bool`s in one struct |
| `src/ui/settings.cc`, the names beside `switch_choice` | the words each row says, kept beside the name it belongs to |
| `src/ui/settings.cc`, `settings_rows` | each row drawn with `offered_of(how, named.which)` deciding which way its pill stands |
| `src/app/requests.cc`, `struct set_switch` | what a press on a row asks for: which one, and on or off |
| `src/app/screens.cc`, the `set_switch` visitor | the only answer to that ask today: `how_ = set_of(how_, asking.which, asking.on)` |

So the state is real: it is kept, it survives every other press, and it is
read back to draw the row. What it is not is *read by anything that would then
do something*. The three tools this program owns are read that way, and the
difference is worth holding next to this:

- a **tool** is offered on the wire. `tools_of(how)` (`src/tools.cc`) walks the
  three of them, leaves out whichever was switched off, and the answer the
  program writes names only the ones left. Switch one off on the settings page
  and the next request this program writes is different because of it.
- one of the **seven** is nothing on the wire and nothing anywhere else. It is
  a `bool` that a row reads to draw itself.

## The seven, and what each still needs

| row | default | what it would take to be true to its words |
| --- | --- | --- |
| Read a picture into words | on | a way in for a picture beside the field -- the clip already stands there -- and something that reads one into words: either the model asked through the wire with the picture carried along, or a separate reader. Nothing that asks for a picture exists yet, and a probe of what the page says cannot see this row change anything. |
| Make a picture out of words | on | the other half of the same thing, and somewhere to put what comes back. Nothing in this tree asks for a picture to be made. |
| Speak through another program | **off** | an address to ask through, and the asking itself going through it instead of straight to the provider. What carries a request today is the browser's own fetch to the server that served the page (`docs/PROTOCOL.md`); a proxy is not part of that yet. |
| Be served as a page of its own | **off** | something serving it, which is the back end being written apart from this tree. While nothing serves it, nothing can be switched to ask for it. |
| Keep what has been said here | on | **wired 2026-11-14.** On, every change is written to the store at the moment it happens (`screens.cc`, `remembered()`), a run that ends keeps what it had, and at the next start the conversations come from the store rather than from the sample in `src/app/sample.cc` -- an empty store still leaves the sample standing, which is what a first run and a fresh page have. Off, nothing more is written and the store is let go; what a run already kept stands. The row says what stands there beside its own name ("Kept here: N conversations, M messages" while on, "kept off" while not), which is what `tools/probe_switches.mjs` now reads back after its press. Its own state still starts at the default each run -- no setting outlives a run today, the switch among them. |
| Keep what it has been taught | on | somewhere for what it has been taught to stand, and something that reads it and says it to the wire. Nothing holds such a thing today. |
| Keep what has been given to it | on | the same for anything handed to it -- a file, a picture, a name -- and somewhere that anything is put. |

Two of the seven are **off** when nothing has been pressed, and that is said
in `asking::switches` with the reason kept beside it: *"but the two that put
this program where other programs can reach it, which are asked for before
they are given."* A row offered is not a row promised.

## What is proved about them, and what is not

Proved on the page in a browser (`tools/probe_switches.mjs`, read again
2026-11-08): every one of the seven is **drawn**, every one **moves** when its
row is pressed, and the pill it moves is read back out of the row differently
afterwards. The probe finds each row by its words, wheels it into the room,
and presses it there -- it does not trust the square the mirror gives it, which
for rows this far down is a square of layout and not of the glass
(`docs/probes.md`).

Not proved from this page: that the six still unwired change anything when
they are on -- there is nothing yet for them to change. A row says that
something is offered; **what offering it changes is the rest of it**, and until
that is written for each, the honest reading of the page is six rows held ready
and one (keeping) wired, not "seven things that work".

## How one of them gets wired, when its time comes

Put beside the three tools, which are the same shape and already read:

1. What the row asks is answered -- already true for the seven, in
   `src/app/screens.cc`.
2. What the switch is offered for exists as something the program can do -- a
   picture read, an address asked through, somewhere kept.
3. What it changes is written where the thing happens. For a tool that place
   is `tools_of(how)` in `src/tools.cc`, which turns what is kept into what a
   wire is told.
4. What it changes is **said on the page**, so a probe can ask it. This is
   the step that is easy to skip and expensive to skip: three probes of this
   tree spent a night reporting things as broken that merely had no visible
   effect to read. Nothing is done, in this repository, until a probe can be
   answered by it.
