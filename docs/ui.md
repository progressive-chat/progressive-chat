<!-- SPDX-License-Identifier: AGPL-3.0-only -->

# What is drawn, and how

Everything there is to know about the drawing half of this program, in one
place: what the parts are, what each is for, what measures every one of them
is laid out by, how a press becomes something the program does, and what is
written down where something is drawn for the first time. Nothing here is
guessed; where a thing was learned by getting it wrong, the wrongness is said
too, and the place it is said at length is named.

The whole room is one tree of `scene::Node`s, made once when the program is
made and never rebuilt: what changes is only what each node says and where it
stands, and that is said again to every node, every frame, by one call to
`again` that walks down from the window's root. So nothing here is a widget
that keeps its own state and tells the world about it later: every part is
filled in from the model between frames, and every press is answered between
frames too (docs/verifying.md says how that looks from outside, through a
browser).

## The parts

| Part                        | File                 | What it draws                                                     |
| --------------------------- | -------------------- | ----------------------------------------------------------------- |
| the room                    | `src/ui/window.cc`   | every other part, put beside each other for this size of window   |
| the conversation list       | `src/ui/drawer.cc`   | the search, the conversations, Tasks and Remote, the settings row  |
| the bar over the messages   | `src/ui/top_bar.cc`  | the name, the four plates, what the chat has been worth           |
| the messages                | `src/ui/message_list.cc` | bubbles, the footer under each answer, a fold above each one  |
| the card at the foot        | `src/ui/composer.cc` | the field, the plus, the face that opens the faces, the name of what answers, the ring, three dots, and what was being written to each conversation |
| the settings                | `src/ui/settings.cc` | eight groups, in the order the design names them                  |
| the design, read in the room | `src/ui/doc.cc`     | `design-doc.md`, in lines that wrap to the breadth of the room    |
| what every model is told    | `src/ui/prompt.cc`   | one field, and nothing else                                       |
| the seeds offered           | `src/ui/appearance.cc` | one row per seed, its own colour beside its name                |
| what more than one part uses | `src/ui/controls.cc` | a circle for a name, a pill, a field, a badge, a dividing line   |
| every measure and two shapes | `src/ui/base.cc`    | the numbers everything is laid out by; `ask_box`, `press_box`     |
| the colours                 | `src/ui/theme.cc`    | `paints`: one field per use, read out of Material's scheme        |
| the icons                   | `src/ui/icons.cc`    | `shape`: every mark drawn here, as one path each                  |

Three parts are pages rather than panels: the settings, the design and what
every model is told. Each is drawn over the whole room when it is open and
put away whole -- `setVisible(false)` -- when it is not, and each is asked
for by one request and put away by another (`open_settings`/`close_settings`,
`open_design`/`close_design`, `open_system_prompt`/`close_system_prompt`).

## The measures

Every one of them said once in `src/ui/base.cc`, and no number of points
written anywhere else in the drawing half but these:

| Measure             | Points | What it is                                                  |
| ------------------- | ------ | ----------------------------------------------------------- |
| `drawer_width`      | 360    | how wide the conversation list is                           |
| `drawer_open_room`  | 48     | how much of the messages stays in sight beside it, come over |
| `drawer_margin`     | 20     | what the panel keeps between itself and its own parts       |
| `drawer_heading_size` | 26   | what a name is set in at the head of the panel              |
| `drawer_row_height` | 44     | one row of the panel's own                                  |
| `drawer_row_size`   | 15     | the type of a row's words                                   |
| `drawer_row_radius` | 14     | how far a row is rounded                                    |
| `drawer_row_corner` | 4      | where two rows meet, so that they read as one row cut in two |
| `drawer_row_gap`    | 4      | what two rows keep between them                             |
| `drawer_right_margin` | 3    | how much wider a row's right margin is than its left        |
| `drawer_mark`       | 18     | how big the mark before a name is                           |
| `side_by_side_from` | 960    | from this breadth on, the drawer stands beside the messages |
| `top_bar_height`    | 52     | how tall the bar over the messages is                       |
| `top_scrim`         | 180    | how far the bar's own ground reaches down behind the list   |
| `pill_radius`       | 22     | a pill's corners                                            |
| `row_height`        | 76     | one row of the list: a conversation, and its circle         |
| `avatar`            | 44     | the circle standing for a conversation, or for who wrote    |
| `bubble_max`        | 720    | no bubble wider than this, however wide the room is         |
| `bubble_radius`     | 20     | a bubble's corners                                          |
| `bubble_corner`     | 4      | the corner a bubble pulls in against the one above it       |
| `card_radius`       | 20     | the card written in, and a card in the settings             |
| `send_size`         | 46     | the round button that sends                                 |
| `composer_height`   | 92     | what the card written in stands high                        |
| `gap`               | 12     | what two things kept apart keep between them                |
| `edge`              | 12     | what the room keeps clear of itself; a card's own padding   |
| `body_size`         | 15     | the body's type, before the reader asks for it larger       |

Two more belong to the card's three dots alone, and stand with the menu they
open in `src/ui/composer.cc`: `editor_menu_width` (208), and
`editor_row_height` (40) for one row of it. Three more belong to the card of
faces, opened beside the clip: `face_cell` (36), which is what one of them
is given and what four rows of seven of them are laid out by; `face_size`
(22), the type each is set in; and `face_radius` (10), how far one of them is
rounded. The drawer's rows and the menu's rows are said there as well, for
what each is a row *of* is said where that part is drawn and not in the
measures of everything.

## The colours

Everything drawn is given one field of `paints` (`src/ui/theme.cc`), read out
of a Material colour scheme grown from one seed. A part never says a number
of its own: it says what the colour is *for* -- `paper`, `drawer`, `field`,
`card`, `raised`, `text`, `dim`, `faint`, `primary`, `on_primary`, `sent`,
`on_sent`, `outline`, `badge`, `badge_text`, `avatar`, `on_avatar`,
`thought`, `scrim_top`, `search_bar`, `scrim`, `away` -- and which colour
that is today is Material's own to say, seed by seed and light or dark.

Three of them are see-through: `scrim_top`, whose four high bits are nothing
at all so that the messages read through it; `search_bar`, whose own alpha is
`0xd9`; and `away`, nothing painted and everything pressed, which is what
puts away a card that three dots opened (`docs/skiff-notes.md` says what a
plate with no colour still takes).

## A press, from the pointer to the model

1. **What was pressed is what was painted last there.** The tree is walked
   from what was painted last in the place backwards, and the first node that
   takes input answers. So a plate whose mark is painted in its middle
   answers through the mark, and the mark's own walk climbs to the plate.
   This is what `press_box` in `src/ui/base.cc` exists for, and it is why
   every press with a mark in it here is made of one and not of a plate and
   an icon put over it. (`docs/skiff-notes.md`, *What the page says was
   pressed is what it painted last*.)
2. **The node asks.** It pushes one `app::request` into the `app::requests`
   it was given -- a variant of every kind of thing the screens can ask for
   (`src/app/requests.cc`). It does not touch the model and it does not draw:
   a handler runs while it is handling, and everything else happens between
   frames.
3. **The program answers.** `screens::take` (`src/app/screens.cc`) reads each
   asked request, oldest first, and does what it says -- one callable per
   kind of request, so that a new one cannot be forgotten.
4. **The tree is filled in again.** What every part draws is said to it once
   more, from the model, with the size of the room the last frame left.

### What is painted over the room

Two plates in `app_window` take presses for the room rather than for
themselves. The `fShade` dims everything beside the drawer while it is over
the messages and puts it away when pressed. `fAway` is painted nothing at all
and stands while a card opened in the composer is out -- either card of three
dots, or the faces -- and everything the room has below the card hands its
presses to it, and the card puts itself away. Both are
taken by what was painted over them, which is why each stands where it does
in the child order, and `docs/skiff-notes.md` (*What the page says was
pressed is what it painted last*) says what that order is and what it costs
to get it wrong.

`ask_box` is a plate that only asks; `press_box` is the same with a mark
painted in it. Both are given their request, their colour and the list they
push into every frame, which is why nothing about a press has to be kept
between frames and nothing about it can go stale.

### Every request, and what the program does with it

| Asked                    | For                          | Done                                                              |
| ------------------------ | ---------------------------- | ----------------------------------------------------------------- |
| `open_conversation`      | a row of the list            | this conversation open; the drawer put away where it came over the messages |
| `send_message`           | Enter, the round button      | sent: kept, counted, and asked of a model                          |
| `new_chat`               | the row at the head          | a conversation of nothing, opened, with the window its first model has |
| `copy_answer`            | the Copy chip                | the answer, whole, given to the machine's clipboard                |
| `regenerate_answer`      | the Again chip               | the answer away with every one after it, and asked again           |
| `fork_answer`            | the Fork chip                | everything up to this answer, into a chat of its own               |
| `share_answer`           | the Share chip               | the answer given away as it reads                                  |
| `delete_answer`          | the Delete chip              | the answer gone, and what was asked above it left                  |
| `copy_text`              | a line of the settings       | what it says, whole, onto the clipboard                            |
| `set_query`              | the list's search field      | what to search for, asked at every edit of it                      |
| `toggle_drawer`          | the pill, the room beside it | the list come over the messages, or put away                       |
| `open_settings`          | the row; the menu's Advanced | the settings opened, and what opened them put away                 |
| `close_settings`         | the settings' own close      | the settings put away                                              |
| `open_tasks`, `open_remote` | the two rows beside the search | nothing yet: what they name is not written                      |
| `fork_conversation`      | the bar                      | this conversation, whole, into one of its own                      |
| `share_conversation`     | the bar                      | everything said in it, in order, given away                        |
| `open_system_prompt`     | the bar's three dots         | what every model here is told, opened to be read and written       |
| `close_system_prompt`    | that card's own close        | put away                                                           |
| `set_system_prompt`      | typing in that card          | kept by the conversation as it is typed                            |
| `toggle_more`            | the bar's three dots         | what they hold, out or away                                        |
| `toggle_editor_menu`     | the card's three dots        | what they hold, out or away                                        |
| `toggle_faces`           | the face beside the clip     | the card of faces, out or away                                     |
| `set_thinking`           | its Thinking row             | whether an answer is thought about before it is written            |
| `set_effort`             | its Effort row               | how hard: the next of the four, whichever stands now               |
| `open_design`, `close_design` | the drawer's row        | the design read in the room, and put away whole                    |
| `pick_seed`              | a row of Appearance          | every colour grown again out of this seed                          |
| `set_amoled`             | its switch                   | a ground of nothing at all, where it is asked for                  |
| `set_avatars`, `set_row_times` | their switches         | whether a circle stands for a conversation; whether a row says how long ago |
| `send_test_notification` | the Notifications row        | one notification of this program's own, to see whether it is let show any |
| `set_font_scale`         | the Appearance row           | every size in a message asked for again with this multiply in it   |
| `pick_theme`             | the Appearance row           | light or dark, whatever the seed is grown in                       |
| `set_generation`         | a knob of Generation         | one of the five things a wire is told about choosing words         |
| `set_tool`               | a switch of Tools            | one of the three tools this program owns, offered or not           |
| `set_switch`             | a switch among the seven     | one of the seven a switch is offered for offered or not            |
| `set_answerer`           | a row of Models              | this model answers in the conversation open, or it does not        |

Two of them are answered by nothing at all today, and are said so in
`TODO.md` rather than answered by a press that does nothing: Tasks and
Remote.

## Adding a control

In the order it is done, which is the order the tree is walked:

1. **The request.** One struct in `src/app/requests.cc`, named for what is
   wanted and not for the thing pressed, and added to the `request` variant
   there: every kind of it is handled where it is read, and one left out of
   the variant cannot be asked.
2. **What it is answered by.** One callable in `screens::take`, doing it to
   the model and nothing to the drawing.
3. **The shape.** A node in the part that holds it, made with the part and
   never made again; where a press is taken, an `ask_box` or a `press_box`.
4. **Where it stands and what it says.** The same node's spec and its words,
   said to it in the part's own `again` from the model and from the size of
   the room -- every frame, whether anything changed or not.
5. **Put away, where there is something it belongs to.** A card opened over
   the room takes its own request to close, and takes whatever plates stand
   over the room with it.
6. **Said, and proved.** One line in `TODO.md` for what is still missing, and
   a probe in `tools/` that presses it in a browser of its own and says what
   the press did (`docs/verifying.md`, *Every square, pressed once*). A
   control nothing can be seen to answer was not finished: either something
   answers it or it is not drawn.

## Where a size comes from

Nothing here is told how tall it is. A bubble is as wide as the words in it
wrap to at the breadth it was given, and as tall as the lines they come to;
the messages are as tall as their bubbles put together, which is what the
scroll view measures. What that costs, and every wrong way round it that was
tried first, is written down in `docs/skiff-notes.md` -- read *What nothing
measures reads as nothing wide* before putting a number where a measure
would do.

## What the room reads as

Where the window is narrower than `side_by_side_from` the list stands beside
the messages no longer: it comes over them when the pill asks for it, leaving
`drawer_open_room` of them in sight to be pressed to put it away. The same
narrowing keeps the conversation, the footer under an answer and the card
written in where they are. Everything else is said out of the breadth it was
given and nothing out of the machine it runs on: a phone and a wide window
differ by the numbers above and by nothing else.

## What a place said in a message is

A line written in the field, or answered by a model, may name somewhere:
begun `https://`, `http://` or `www.`, and running until what no place runs
past -- a space, or one of the marks that wraps one in writing. That much of
the line is not said as words only: the text draws it in the colour
everything on the screen that answers a press is drawn in (`paints::link`,
the scheme's primary) and underlines it, and a press that lands on it and
does not move goes where it says. A press that moves is a selection first,
as it is anywhere in a text here, so no link is opened by a reader reaching
for the words around it.

Where each stands is read once, in `pchat.logic.formatting`, out of the line
and nothing else: what no place runs past, what a sentence says after a
place rather than in it, and how a place begun `www.` is `https://` to the
machine though neither of them was written. The screen is given the places
and nothing of how they were found: it draws them where it is told and never
looks at a colon again -- which is what the tests of that module say of it,
too, including that a place begun and left without a name is not one. Where
a press takes one is asked of the machine itself (`pchat.platform.opened`):
a browser of a desk's or a phone's own, another tab where this runs as a
page, and nothing asked of anywhere else yet.

Asked of the page by `tools/probe_links_opened.mjs`: a message written with
a place in it, read back as its own `<a href>` in what the mirror says of
the glass, then pressed, and the tab it opened seen from outside the page.

## What the card keeps of what was written

What is in the field belongs to the conversation it was written for, and
nothing written for one is read in another: leaving a conversation keeps
what was in the field to it (`pchat.logic.drafts`), opening it again gives
it back, and opening another gives that one whatever belongs to it -- which
is nothing at all the first time. Sent, nothing of it is kept anywhere, and
coming back later finds the field the way sending left it. That is kept
beside the field itself and said nowhere else, because whose field it is and
what is written in it have never been anything but that card's own: the
program between the frames never hears of it, and nothing of it outlives the
window.

Asked of the page by `tools/probe_drafts.mjs`: written in one conversation,
gone in the next, back again in the first, and gone for good once it has
been sent.

## What the card writes into the field

A face, opened by the face beside the clip, is written into the field rather
than asked for anywhere: nothing of what a request carries reaches where the
pen stands, and whose field it is belongs to that card alone (above). So the
card keeps one string, a pressed face writes itself into it, and the card
reads it empty again before the next frame, putting what was in it at the
end of what has been written. Twenty-eight stand in four rows of seven, over
the messages and above the face they came out of; sending what was written
puts them away, and so does opening another conversation, for a face belongs
to the field it was opened over.

Above them, once one of them has been pressed, is a row of the ones pressed
most lately, newest first and no longer than a row's breadth: a press says
`face_written` as well as writing itself, the screen keeps them in the order
they were pressed and not the order of how often, and the card is one row
taller while it shows them. Nothing below them moves: the twenty-eight stand
where they stood, for the card grew upwards and its row went into the room it
was made by. `recent_emoji` (`pchat.config`) is what is to keep them between
runs; what keeps them today is the screen, until there are runs to keep them
between.

Asked of the page by `tools/probe_faces.mjs`: the face opens twenty-eight,
a press on two of them writes two into the field, and a press in the middle
of the messages puts the card away again -- and by
`tools/probe_faces_lately.mjs`, that opening the card again says the same two
in the row above, newest first, in the order the field does not keep them in.
