# Every probe in the tree, and what each one asks

A probe is a question asked of the painted page, because nothing inside says
what was drawn. How to make one, and what a page can be asked, is in
[verifying.md](verifying.md); this is what each of the ones kept in `tools/`
asks, so that a question already asked once is not asked again by accident,
and so that a question nobody has asked yet can be seen missing.

Every one of them is driven either at the deployed page or at a build that
has only been put out into a workflow's artifact -- the second is what one is
asked while its change is still in a branch:

```sh
node tools/probe_faces.mjs                        # the deployed page
node tools/probe_faces.mjs http://127.0.0.1:8126/index.html   # an artifact
```

`probe.yml` runs `tools/probe_painted.mjs` on what it has put out, or a
`said_by_hand.mjs` written into the run when something else is wanted.

## What a probe's press has to do to be answered

Five things a press has to be for the room to answer it, all learned the hard
way by probes that asked nothing for a while (`probe_copied.mjs` first):

- **The mirror's box is a square of layout, not of the glass.** It says
  where a thing is put, not where it is painted or whether a press reaches
  it: for the row "Read the design" it gives y **2417** in a page 900 high,
  and every press aimed there landed in nothing -- three probes that way,
  each ending in "nothing answers". What finds it is to press: scroll a step,
  press across the column, and read whether what was wanted was said. A press
  at x 450, y 480 opens the design, which no reading of boxes would have
  found. So a probe that cannot press its way to a thing must not say the
  thing answers nothing -- it has only said it did not find it.
- **Read whether it is out before saying it was put away.** A plate that
  turns a card out and in by turns has to be asked which way it stands now:
  `probe_more.mjs` pressed its three dots a third time to put the card away,
  and the card had already gone, so it read `false -> false` and meant
  nothing by either of them.
- **The pointer stands on it first.** A press put down where nothing had been
  pointed at is not answered: move onto what is pressed before the press, and
  press where it stands rather than in the padding beside its words.
- **The room has finished saying itself.** Six seconds is long enough; two is
  not. Read too soon, the box a chip was read with is one the chip has
  already left, and what the press lands on is not the chip.
- **It is said in the page's own words.** A field is said with a label
  (`[aria-label="Search conversations"]`) and not with a tag: the mirror is
  free to say one with an `input` or with a `textarea`, and a probe that said
  `textarea` waited thirty seconds for something that was never drawn.

Neither is about this program in particular: what they say is that a press a
probe makes is a press and nothing else, so a probe that presses sooner, or
somewhere it never pointed, is asking nothing at all.

## A probe goes stale, and says nothing while it does

A probe keeps asking what it was written to ask even after the page has moved
from under it -- and what it answers then is either a string of `false` or a
crash, neither of which is the page saying no. Thirteen of them were found that way
in one night (2026-11-08) and mended; each is here with what went stale, so
that the next one found in that state is recognised for it.

| probe | what had gone stale | what mends it |
| --- | --- | --- |
| `probe_copied.mjs` | pressed without pointing first, and read the room after 2.5 s | move onto the chip, press it, wait 6 s |
| `probe_search.mjs`, `probe_search2.mjs`, `probe_search3.mjs` | waited for `textarea[aria-label="Search conversations"]`, and the field is not said with a textarea | find it by its label alone |
| `probe_settings.mjs` | pressed the row at the square the mirror gave it, y 2417 | presses its way down the column instead, and reaches the design |
| `probe_design.mjs` | opened settings at (330,30) and pressed the row at (640,720) | opens settings by the word, presses until the design is said |
| `probe_generation.mjs`, `probe_now.mjs` | opened settings at (330,30) | open it by the word "Settings", read where it stands |
| `probe_switches.mjs` | hunted a gear drawn in a 36x36 square near the top left, and fell over when there was none | open settings by the word; all seven switches are drawn and move again |
| `probe_bar.mjs` | asked the room for `sonnet-4`, a model this page never says, and read `false` for every press | ask for the answerer the room names, `Claude Haiku 4`; `named: true` on every plate |
| `probe_generation.mjs` | scanned a fixed band of the card for something that moved, and never reached a row, for the rows stand below the fold | read each row by its own name, wheel it into the room, press either side; all five move -- `0.70 -> 0.65`, `1.00 -> 0.95`, `2048 -> 1792`, `0.00 -> -0.10` twice |
| `probe_typed.mjs` | opened the card with the first plate of the top row, where the system words are behind the second, and hunted the field by pressing down the card | open the second plate at 1206,26 and read the field by its label; it stands at 640,450 and keeps what was written, `kept: true` |
| `probe_editor_menu.mjs` | pressed each square of the card's top row without pointing at it first, and found no dots at all | point first; the three dots stand at 1245,818 and open five words: thinking, effort, advanced |
| `probe_fold.mjs` | pressed the fold at 2.5 s, before the room had finished being made, and without pointing at it | wait 6 s and point first; the green summary comes and goes: false, true, false |
| `probe_fold_calls.mjs` | pressed the fold without pointing at it first, and read 0 of 3 calls even with the fold open | move onto the fold, press it; it says `3 of 3` with the page at 2359 chars, and `0 of 3` shut |

One of them is worth saying twice, for it looked like the page refusing:
`probe_generation.mjs` could move four of the five rows and never Top P, and
Top P stands at **1.00**, which is its own ceiling. A row standing at the top
of what it may be cannot be raised, and raising was the only thing the probe
tried; eighty back from the row's far end lowers it, and then Top P answers
like the rest of them, `1.00 -> 0.95`.

Two probes of this tree also fell over for a reason that was not the page and
not the probe: `probe_nothing_to_send.mjs` and `probe_faces_lately.mjs` both
died with the browser's execution context destroyed while five probes were
being run at once against the one deployed page, and both said what they were
written to say when they were run one after another. **Run a few at a time.**

A seventh was mended the same evening: `probe_more.mjs` asked whether the
bubble over the messages says `~4.1k / 1M tokens`, and what the page says now
is `4.1k tokens`, so its first reading said `false` from the start and meant
nothing. It asks for what is said, and it asks which way the three dots stand
before it says the menu was put away -- pressed a third time the card had
already gone, and that check read `false -> false`. It reads `true -> false`
now: out before, gone after.

## What the whole page says

| probe | what it asks |
| --- | --- |
| `probe_now.mjs` | what the build standing on the net says of itself |
| `probe_painted.mjs` | the page shot and said: every part gone over, with a wire standing in for the back end |
| `probe_zero_size.mjs` | every node whose size came of nothing, which is drawn nowhere and answers no press |
| `probe_worker_blocked.mjs` | what the page says where its worker is refused, at 1280x900 and at 390x844 |
| `probe_typed.mjs` | every field's label, which is what it says empty, and what it holds |
| `probe_rect.mjs` | where the fields stand in the page, said as rectangles |
| `probe_rect2.mjs` | the same for one field, with the four boxes that hold it |
| `probe_scan.mjs` | asked by hand: pressed across the bar over the messages until something answered |
| `probe_bar.mjs` | asked by hand: the plates over the messages, pressed once each, left to right |
| `probe_top.mjs` | asked by hand: the plate that opens what every model is told |
| `probe_search2.mjs` | asked by hand: where four conversations stand once the drawer's field was typed into |

The five asked by hand are the oldest ones here and carry no header of their
own; what they ask is what they press, and nothing more has been written of
them.

## What a press answers

| probe | what it asks |
| --- | --- |
| `probe_presses.mjs` | every square small enough to be a control, pressed once on a page of its own: which answer something, and which are stubs |
| `probe_send.mjs` | the two ways out of the field -- Enter in it, and the button beside it -- held to the same two answers each |
| `probe_nothing_to_send.mjs` | that neither way out sends what is only spaces, which is one rule asked in two places |
| `probe_field.mjs` | that writing works in the field at all |
| `probe_search.mjs` | the drawer's field, pressed where it stands |
| `probe_search3.mjs` | what that field is asked for when it is pressed and not typed into |
| `probe_fold.mjs` | that a press on the fold above an answer reads what the model thought, and folds it away again |
| `probe_fold_calls.mjs` | that the calls a fold counted are said only while it is open |
| `probe_top_bar.mjs` | every plate over the messages: that something is read after each of them was pressed |
| `probe_more.mjs` | three dots, where the room is too narrow for four presses |

## What each room has in it

| probe | what it asks |
| --- | --- |
| `probe_design.mjs` | the settings opened where it stands, Read the design pressed by pressing until the design is said, and the column read back whole |
| `probe_settings.mjs` | the names of the nine groups in the settings, and the row that offers the design reached at the foot of them |
| `probe_generation.mjs` | what the five things an answer is asked with say, and that a press either side of one moves it |
| `probe_switches.mjs` | each of the seven switches drawn, and moved by a press -- read by its words, wheeled into the room, pressed there -- and the keeping row's own words read before and after its press (what its switch does is on the page) | (see `docs/switches.md`: keeping is wired, the other six are rows)
| `probe_editor_menu.mjs` | the three dots beside the card's ring: thinking, how hard, and the way into the settings |
| `probe_faces.mjs` | the face beside the clip: a card of faces, one written into the field, and the card put away |
| `probe_faces_lately.mjs` | that the faces pressed most lately stand in a row above the rest, newest first |
| `probe_block.mjs` | the block of code the sample's answer says at the end of it, read back in the monospace face, with no fence of backticks left in the page |
| `probe_marks.mjs` | the marks in a line: said in the style they said, drawn as none of them, and what is said in code standing as it was |
| `probe_links_opened.mjs` | a place said in a message: its own link in what the mirror says of the page, and the tab a press on it opens |
| `probe_quoted.mjs` | a line said quoted: read back with none of its stops left, what it says said whole, and the marks inside it still drawn |
| `probe_drafts.mjs` | that what is written belongs to one conversation and comes back to it |
| `probe_copied.mjs` | that what a Copy chip copies reached the machine's own clipboard |

## Two things every one of them learned the hard way

A press is put where a node is *read*, and not where a node was once. The
column beside the messages does not keep its first breadth, a card grows
upwards without moving anything below it, and a probe that presses a point
written down from another run is how an afternoon was spent proving a working
press was broken.

The mirror says one node to a line. A row of faces is not one string in it
until the breaks are taken out of it -- which is what `probe_faces_lately.mjs`
does with its face before it asks whether the row is there.
