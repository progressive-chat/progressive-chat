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
| `probe_design.mjs` | Read the design, pressed, and the column of the design read back out |
| `probe_settings.mjs` | the names of the nine groups in the settings, and the row that offers the design reached at the foot of them |
| `probe_generation.mjs` | what the five things an answer is asked with say, and that a press either side of one moves it |
| `probe_switches.mjs` | each of the seven switches drawn, and moved by a press |
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
