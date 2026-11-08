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

      Said of the page on 2026-11-08, and no longer true of it: a browser
      with both `webgl` and `webgl2` to be had of it nowhere ("gl to be
      had: {"webgl":false,"webgl2":false}") still paints the whole room,
      the same 1785 characters of it a browser with software GL paints
      (`tools/probe_no_gpu.mjs`). What asks is `pchat.platform.drawn`, and
      what the window is told where there is nothing is to draw in
      software (`src/main.cc`). So the phone is the one left to say it of,
      and off nothing but the phone itself.
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

      Every one of the seven is written up, with what it would take to be
      true to its own words, in `docs/switches.md` -- read that first, for it
      says which of them waits on the back end and which waits on something
      this tree has not got yet.

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
- [ ] **A heading and a list a model writes are said as they were written**:
      nothing reads `##` at the head of a line as a heading, nor `-` as the
      beginning of a list: each is drawn as the words they say, hash and
      dash and all. The marks inside a line are read now -- bold, slanted,
      struck, code, a place said in it, and how deep it stands quoted -- but
      what a line is made *of* is not, and it would be a text split into
      lines that each know what they are, rather than one run of runs as it
      is today (`src/logic/formatting.cc`, and every place a message's body
      is said in `src/ui/message_list.cc`).
- [x] ~~The settings page answers no press that any probe has made.~~
      **It was the probing, not the page.** Asked again by pressing rather
      than by trusting the mirror's squares, the settings page answers every
      press it is given, and the row "Read the design" opens the design --
      *"The design of a chat"* said after one press at x 450, y 480. What
      misled three probes is kept below, in `docs/probes.md`: the box the
      mirror gives for something is a square of layout, not of the glass, and
      for this row it says y 2417 in a page 900 high.

- [ ] **What a model said for later (`||` either side of it) is not said at
      all.** It can be read into a run and drawn covered behind a plate of
      dots -- which was done, and asked of the page: the run is drawn with
      its words in nothing and a plate behind them -- but nothing can open
      it. skiff shows a spoiler when the text itself answers a press; what it
      showed is put back the next frame, because this room says every node's
      styles again every frame and keeps nothing that was pressed, and
      nothing in `scene::hostWork()` says a spoiler was pressed, so there is
      nothing for `src/app/` to keep and answer with. A message covered for
      good is worse than one said plainly, so no line says one today.
- [ ] Where nothing measures a line there are surely more than one node whose
      size came of nothing: six chips and the fold above an answer were the
      two found, both now said their size, and nothing else pressable has
      been probed the way they were.
- [ ] **The editor's row is drawn as far as its three dots, and no further**:
      they are drawn after the ring now, and open thinking with its effort
      and Advanced into the large settings; what the design asks of them
      besides -- web search, and a context compacted -- is not drawn, for a
      press that asks nothing is a stub, and neither what a page on the web
      would be searched nor where a context would be written short is
      written yet (`src/ui/composer.cc`; the design says which under *The
      editor's row*). The plus on their left that attaches (camera, gallery,
      videos, files) is not drawn either, and for the same reason: nothing
      reads a file given it in this program today.

      Read off the page on 2026-11-07 by `tools/probe_presses.mjs`, which
      pressed every square of a control's size in the settled room: the
      clip, the face and the round button beside the field each took a press
      and answered nothing at all. The round button sends now, and empties
      the field as Enter in it does (`tools/probe_send.mjs` says both), and
      nothing was emptied either way before it. The face answers too, since
      2026-11-08: pressed, it opens twenty-eight faces over the messages,
      and a face pressed writes itself into the field, which nothing asked
      for could have done -- no request reaches where the pen stands, and
      so the card keeps one string of its own and reads it empty again
      before the next frame (`tools/probe_faces.mjs` says the three of it:
      opened, written, and put away by the room). The clip alone of the
      card still asks for what nothing answers.

      Said again on 2026-11-08, this time read out of the source and not off
      the page alone: the clip is laid out (`fClip.apply`,
      `src/ui/composer.cc:452`), coloured (`:493`) and painted (`:599`), and
      unlike `fFace` (`:495`) and `fDots` (`:505`) beside it it is never
      given a request to send -- there is no `fClip.again(asked, ...)` in the
      tree. `tools/probe_presses.mjs`, which now names the squares it finds
      instead of only counting them, called it `1166,864` against the room as
      it stands when it is opened.
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
      looked at why -- looked at now, on 2026-11-08, and it is not the
      narrow page at all. It starts at 390x844 and says 973 characters,
      worker or no; what stops it is the worker being blocked, and that at
      either width, for it is the worker that isolates the page and so the
      reason there are threads to draw on. Where one refuses to be taken
      the page said nothing but "starting" for as long as it was left
      open, which says neither what went wrong nor that anything had: it
      says what is missing now (`web/bootstrap.js`), and
      `tools/probe_worker_blocked.mjs` reads that back at both widths.
