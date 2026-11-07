# What is still to do

Not a plan of the program's whole, only what is known to be wrong or
missing today, written down so that none of it is lost between one
build and the next.

## Wrong today

- [x] **The icon is wrong** -- done 2026-11-04: what every density and both
      shapes of it are made of now is progressive-android's own 512x512
      picture (`android/icon-source.png`), cut by `tools/make_icons.py`.
      The same picture has still not been given to the browser's tab
      (`web/icon.svg`) or to a desktop window, which nobody has looked at.

## Wrong today

- [x] **The middle of a narrow window is empty** -- done 2026-11-05: the
      drawer was asked for from the first frame onwards, which a wide
      window answered by putting it beside the messages and a narrower
      one -- where there is no beside -- by putting it over them and the
      conversation nowhere. It is beside them wherever there is room for
      both now, and over them only where the pill asked for it
      (`src/app/screens.cc`, `drawer_`). Read off the page, driven at nine
      widths: below 960 the conversation, the footer and the composer are
      all still said, and widening gives the drawer back.

## Known not to be done

- [ ] **The program goes as soon as it starts -- on an android.** In a
      browser it does not any more: the page paints, 1532 nodes of it, and
      what it says to be read out of it is the whole of the design. What
      had gone was the probe's own browser, told `--disable-gpu`, which is
      told to have no WebGL at all -- and Skia asking GL what it knows is
      given nothing, which is the same going a desktop with no window has
      (`.github/workflows/crash.yml`, address and undefined sanitizers,
      `GrGLExtensions::init`). So: where there is no GL, in any of the
      three, this program goes before its first frame. Whether the phone
      is one of those places has not been said yet -- `adb logcat -b crash`
      off it would say it in one line -- or off nothing but the phone itself
      now, for the APK says its own going: signal, code and where, written
      to the log before it goes, and read with `adb logcat -d -s
      progressive-chat`.

- [x] **The settings page was read at its end** -- done 2026-11-06: what
      scrolls kept the end it had followed while the column was still being
      made, so Settings was said at -2163 and every group before the middle
      of a column twice the room's height was pressed by nothing. Opened, it
      is read from its first line now (`src/ui/settings.cc`, `scrollToStart`).
      How that was seen is worth keeping: the mirror says where every line
      stands as well as what it says, and one probe read those places out.
- [ ] The back end: `src/app/sample.cc` answers for now, so nothing that
      is counted -- tokens, tool calls, how long an answer took -- is
      counted for real.
- [x] Copy to the clipboard carries it to the machine's own clipboard --
      read back on 2026-11-07: a page kept in a browser, asked for its
      clipboard by permission, keeps the whole answer the Copy chip was
      pressed on and nothing that was in it before (`tools/probe_copied.mjs`).
- [x] Three of the eight settings groups still stand named and empty:
      Multimodal, Network, and Memory & Data. What is drawn today under the
      others is Appearance (the theme, its type and its dark), what answers
      in the conversation that is open (Services, Models), Generation
      (Responses), the three tools this program owns (Tools), and what this
      build is (About). Every one of them has what it is named for now: the
      three that stood empty -- a picture asked about, a proxy asked
      through, a memory kept between conversations -- say a switch to each
      of the seven things a switch is offered for, kept where the three
      tools are kept and said in a row each under the item each belongs
      under (`src/config.cc`, `switch_choice`; read off the page on
      2026-11-07, where every row of the seven is pressed and every pill it
      moves). A row says that something is offered; what any of the seven
      does when it is on is still nothing written, and that is what is left
      of this item.
- [ ] Nothing answers any of the seven switches: each of them is kept and
      each of them moves, and none of them is read by anything that would
      then do what it says it would -- what a picture is read into words
      with, what a proxy is asked through, and where what is kept is kept.
      The row says a thing is offered; what offering it changes is the rest
      of it.
- [x] What a model wrote to itself, folded above its answer, is pressed open
      and read: `history_fold` keeps it (`src/ui/message_list.cc`), and a probe
      of the page in a browser says a press anywhere in the line opens it and
      shuts it again. Nothing opened it before because nothing there measured
      the words: what that costs, and what is said instead, is written under
      *What nothing measures reads as nothing wide* in `docs/skiff-notes.md`.
- [x] **Nothing in the bar over the messages answers a press, outside of the
      three dots**: what every model is told, this conversation taken into
      one of its own, and given away, are each said and each wired, and a
      press all the way across the bar asked nothing of any of them -- read
      off the page on 2026-11-07, by a probe that pressed every ten points
      of the bar and watched what the press was said to and whether any
      click was made at all. Why is written in `docs/skiff-notes.md` under
      *What the page says was pressed is what it painted last*: each of the
      four held the whole breadth of the bar, so the last of them was over
      the other three, and what they press was never asked. Done the same
      day, and read again off the page: four plates in the bar, and every
      one of them answered (`tools/probe_top_bar.mjs`, which presses them
      in the settled layout and says what each did afterwards).
- [ ] Where nothing measures a line there are surely more than one node whose
      size came of nothing: six chips and the fold above an answer were the
      two found, both now said their size, and nothing else pressable has
      been probed the way they were.
- [ ] **The editor's row is drawn as far as its name and its circle, and no
      further**: the design asks for a plus on its left that attaches (camera,
      gallery, videos, files) and three dots on its right that open thinking
      with its effort, web search, compact context, and Advanced into the
      large settings. Neither is drawn: a press that asks nothing is a stub,
      and what is to answer them -- where a file is read and where a context
      is compacted -- is not written yet (`src/ui/composer.cc`; the design
      says which under *The editor's row*).

      Read off the page on 2026-11-07 by `tools/probe_presses.mjs`, which
      pressed every square of a control's size in the settled room: the
      clip, the face and the round button beside the field each took a press
      and answered nothing at all. The round button sends now, and empties
      the field as Enter in it does (`tools/probe_send.mjs` says both), and
      nothing was emptied either way before it. The clip and the face still
      ask for what nothing answers, and are the two left of this item
      pressed for.
- [x] The thinking a fold opens did not say the tool calls the line above it
      counted -- done 2026-11-07: each call an answer made is kept as it was
      asked and answered (`call_made` in `src/core.cc`, recorded by every
      round of `screens.cc`), and opened, the fold says them under what the
      model wrote to itself, one line each (`src/ui/message_list.cc`,
      `calls_said`). `tools/probe_fold_calls.mjs` says them when it is open
      and not when it is shut; the sample's third sieve answer says two calls
      now, for two is what each of its lines says.
- [x] **Search does nothing yet** -- done 2026-11-06: what is typed in the
      drawer's field is asked for at every edit of it and not only when Enter
      is pressed, and what it keeps is every conversation named after it or
      saying it in anything said in it (`src/logic/lists.cc`,
      `message_matches`). A drawer with nothing kept of the search says so in
      a line where its rows would be; a row kept because a message in it says
      what was searched for says that message and not the last one.

      Written on 2026-11-05 as done and not so: none of it asked anything of
      the program until a press a node keeps was delivered by the program,
      once a frame, which nothing here had done (`src/app/program.cc`,
      `skiff::bind::press`). Everything a field asks for went the same way --
      including what the system prompt's card was typed into.
- [x] **Generation stood named and empty like the rest of the settings** --
      done 2026-11-06: the five things an answer is asked with are drawn
      under their row now, each said as it stands and moved one step either
      way by a press (`src/ui/settings.cc`, `knob_row`; `src/config.cc`,
      `asking`). Where each of them stops, and by how much one press moves
      it, is said once and asked for by both the row and the program that
      keeps it. From there they go on into every body written for a wire
      (`src/provider.cc`, `wants_of`), which is what the tests read back out
      of the json.

- [x] **Nothing could install a browser to drive the page with** -- done
      2026-11-05: a playwright is put out before the core it is said on
      top of, so asking for the newest asked for a `playwright-core` that
      was not there yet and nothing was read of the page at all
      (`.github/workflows/probe.yml`). One version, said by name, and
      moved on by hand when the next one has settled.

- [x] **The probe** -- done 2026-11-04: it asks for SwiftShader now and so
      has a WebGL to draw through, and it came back with 1532 nodes and the
      page's whole text. What it still comes back without is the narrow
      page: 390x844 with the worker blocked does not start, and nobody has
      looked at why.
