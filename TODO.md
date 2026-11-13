# To do

Not a full plan. What is wrong or missing today, kept between builds.

## Wrong today

- [x] **Icon wrong** -- done 2026-11-04. Every density and both shapes now cut
      from progressive-android's own 512x512 (`android/icon-source.png`) by
      `tools/make_icons.py`. Still not applied to the browser tab
      (`web/icon.svg`) or a desktop window; nobody has looked.
- [x] **Narrow-window middle empty** -- done 2026-11-05. Drawer was requested
      from the first frame: a wide window put it beside the messages, a narrow
      one over them (no room beside) and left the conversation nowhere. Now
      beside where both fit, over only where the pill asks
      (`src/app/screens.cc`, `drawer_`). Read off the page at nine widths:
      below 960 the conversation, footer and composer still draw; widening
      restores the drawer.

## Known not to be done

- [ ] **Program exits at startup -- on Android.** In a browser it no longer
      does: the page paints 1532 nodes and its read-out text is the whole
      design. What had failed was the probe's own browser with `--disable-gpu`
      (no WebGL): Skia asking GL gets nothing, the same exit a desktop with no
      window takes (`.github/workflows/crash.yml`, address + undefined
      sanitizers, `GrGLExtensions::init`). So with no GL -- in any of the three
      -- the program exits before its first frame. Whether the phone is one of
      those places is unsaid (`adb logcat -b crash` would say it in one line),
      but the APK writes its own exit -- signal, code, location -- to the log
      before going, read with `adb logcat -d -s progressive-chat`.
      Said of the page on 2026-11-08, no longer true of it: a browser with
      both `webgl` and `webgl2` unavailable
      (`"gl to be had: {"webgl":false,"webgl2":false}"`) still paints the
      whole room, the same 1785 characters a software-GL browser paints
      (`tools/probe_no_gpu.mjs`). What asks is `pchat.platform.drawn`, and
      where there is nothing the window is told to draw in software
      (`src/main.cc`). So only the phone is left to say it of.
- [x] **Settings read at its end** -- done 2026-11-06. Scrolling kept the end
      it had followed while the column was still being built, so Settings was
      at -2163 and every group before the middle of a column twice the room's
      height was unpressed. Opened, it now reads from its first line
      (`src/ui/settings.cc`, `scrollToStart`). Worth keeping: the mirror gives
      where each line stands as well as what it says; one probe read those
      places out.
- [ ] **No back end.** `src/app/sample.cc` answers for now, so nothing counted
      -- tokens, tool calls, answer duration -- is counted for real.
- [x] **Copy to clipboard** -- read back 2026-11-07: a page in a browser,
      asked for its clipboard by permission, keeps the whole answer the Copy
      chip was pressed on and nothing that was in it before
      (`tools/probe_copied.mjs`).
- [x] **Three of eight settings groups named and empty** -- Multimodal,
      Network, Memory & Data. Drawn today under the others: Appearance (theme,
      type, dark), what answers in the open conversation (Services, Models),
      Generation (Responses), the three tools the program owns (Tools), and
      the build (About). Each now has what it is named for: the three that
      stood empty -- a picture asked about, a proxy asked through, a memory
      kept between conversations -- draw one switch each for the seven things
      a switch is offered for, kept with the three tools, one row each under
      its item (`src/config.cc`, `switch_choice`; read off the page
      2026-11-07, where every row of the seven is pressed and every pill
      moves). A row says a thing is offered; what any of the seven does when
      on is still unwritten -- that is what is left here.
- [ ] **No answer behind the seven switches.** Each is kept and moves; none is
      read by anything that would then do what it says -- what reads a picture
      into words, what a proxy is asked through, where what is kept is kept.
      The row says a thing is offered; what offering changes is the rest.
      All seven are written up, with what it takes to be true to their words,
      in `docs/switches.md` -- read that first; it says which wait on the back
      end and which on something this tree has not got.
- [x] **Model's folded self-text** -- what a model wrote to itself, above its
      answer, now opens and closes: `history_fold` keeps it
      (`src/ui/message_list.cc`); a browser probe says a press anywhere in the
      line opens then shuts it. Nothing opened it before because nothing there
      measured the words; that cost, and what is said instead, is under *What
      nothing measures reads as nothing wide* in `docs/skiff-notes.md`.
- [x] **Bar over the messages: only the three dots answered** -- what every
      model is told, the conversation taken into its own, and given away, were
      each drawn and wired, but a press across the bar asked nothing of them --
      read off the page 2026-11-07, by a probe that pressed every ten points
      and watched what the press went to and whether any click happened. Why is
      in `docs/skiff-notes.md` under *What the page says was pressed is what it
      painted last*: each of the four held the whole breadth, so the last sat
      over the other three and what they press was never asked. Done the same
      day, read again off the page: four plates in the bar, each answering
      (`tools/probe_top_bar.mjs`, which presses them in the settled layout and
      says what each did after).
- [x] ~~**Heading and list in a model's reply drawn as written**~~ -- `runs_of`
      reads `##` at line head as a heading (`h1..h6` bits) and `-`, `*`, `+`
      or `N.` as a list (`list` bit), strips the marks, draws a bullet where a
      list mark stood (as `src/ui/doc.cc` does), and `styles_in` maps heading
      level to `TextStyled.scale` (2.0 .. 0.95); a line's bits are said of that
      line only, not over a newline. Tests in `test/logic_test.cc`.
- [x] ~~Settings page answers no press any probe made.~~ **It was the probing,
      not the page.** Asked again by pressing rather than trusting the mirror's
      squares, the settings page answers every press; the row "Read the design"
      opens the design -- *"The design of a chat"* said after one press at
      x 450, y 480. What misled three probes is kept below, in
      `docs/probes.md`: the box the mirror gives for something is a square of
      layout, not of the glass, and for this row it says y 2417 in a page 900
      high.
- [ ] **Model's `|| ... ||` for later is not drawn.** It can be read into a run
      and drawn covered behind a plate of dots -- done and asked of the page:
      the run draws with its words blank and a plate behind them -- but nothing
      opens it. skiff shows a spoiler when the text itself answers a press;
      what it showed is put back next frame, because the room redraws every
      node's styles each frame and keeps nothing pressed, and nothing in
      `scene::hostWork()` says a spoiler was pressed, so `src/app/` has nothing
      to keep and answer with. A message covered for good is worse than one
      said plainly, so no line draws one today.
- [ ] **Unmeasured line sizes.** Where nothing measures a line, more than one
      node surely sized from nothing: six chips and the fold above an answer
      were the two found, both now sized; nothing else pressable has been
      probed the way they were.
- [ ] **Editor row stops at its three dots.** They draw after the ring now and
      open thinking with its effort and Advanced into the large settings; what
      the design asks of them besides -- web search, a context compacted --
      is not drawn, because a press that asks nothing is a stub, and neither
      what page would be searched nor where a context would be shortened is
      written yet (`src/ui/composer.cc`; the design says which under *The
      editor's row*). The plus on their left that attaches (camera, gallery,
      videos, files) is not drawn either, same reason: nothing reads a file
      given it in this program today.
      Read off the page 2026-11-07 by `tools/probe_presses.mjs`, which pressed
      every square of a control's size in the settled room: the clip, the face
      and the round button beside the field each took a press and answered
      nothing. The round button sends now and empties the field as Enter does
      (`tools/probe_send.mjs` says both); neither emptied anything before. The
      face answers too, since 2026-11-08: pressed it opens twenty-eight faces
      over the messages, and a face pressed writes itself into the field, which
      nothing asked for could do -- no request reaches where the pen stands, so
      the card keeps one string of its own and reads it empty again before the
      next frame (`tools/probe_faces.mjs` says the three of it: opened,
      written, put away by the room). The clip alone still asks what nothing
      answers.
      Said again 2026-11-08, read out of the source and not off the page alone:
      the clip is laid out (`fClip.apply`, `src/ui/composer.cc:452`), coloured
      (`:493`), painted (`:599`), and unlike `fFace` (`:495`) and `fDots`
      (`:505`) beside it is never given a request to send -- no
      `fClip.again(asked, ...)` in the tree. `tools/probe_presses.mjs`, which
      now names the squares it finds instead of only counting them, called it
      `1166,864` against the room as it stands when open.
- [x] **Fold's tool calls** -- the thinking a fold opens did not say the tool
      calls the line above it counted; done 2026-11-07. Each call an answer made
      is kept as asked and answered (`call_made` in `src/core.cc`, recorded by
      every round of `screens.cc`), and open, the fold says them under what the
      model wrote to itself, one line each (`src/ui/message_list.cc`,
      `calls_said`). `tools/probe_fold_calls.mjs` says them open and not shut;
      the sample's third sieve answer says two calls now, for two is what each
      of its lines says.
- [x] **Search did nothing** -- done 2026-11-06: what is typed in the drawer's
      field is asked for at every edit, not only on Enter, and what it keeps is
      every conversation named after it or saying it in anything said in it
      (`src/logic/lists.cc`, `message_matches`). A drawer with nothing kept of
      the search says so in a line where its rows would be; a row kept because
      a message in it says what was searched for shows that message and not the
      last one.
      Written 2026-11-05 as done and not so: none of it asked anything of the
      program until a press a node keeps was delivered by the program, once a
      frame, which nothing here had done (`src/app/program.cc`,
      `skiff::bind::press`). Everything a field asks for went the same way --
      including what the system prompt's card was typed into.
- [x] **Generation stood named and empty** -- done 2026-11-06: the five things
      an answer is asked with draw under their row, each said as it stands and
      moved one step either way by a press (`src/ui/settings.cc`, `knob_row`;
      `src/config.cc`, `asking`). Where each stops, and how far one press moves
      it, is said once and asked for by both the row and the program that keeps
      it. From there they go into every body written for a wire
      (`src/provider.cc`, `wants_of`), which is what the tests read back out of
      the json.
- [x] **Could not install a browser to drive the page** -- done 2026-11-05: a
      playwright is put out before the core it is said on top of, so asking for
      the newest asked for a `playwright-core` that was not there yet and
      nothing was read of the page at all (`.github/workflows/probe.yml`). One
      version, said by name, moved on by hand when the next one settles.
- [x] **The probe** -- done 2026-11-04: it asks for SwiftShader now and so has a
      WebGL to draw through, and came back with 1532 nodes and the page's whole
      text. What it still comes back without is the narrow page: 390x844 with
      the worker blocked does not start, and nobody looked at why -- looked at
      now, 2026-11-08, and it is not the narrow page. It starts at 390x844 and
      says 973 characters, worker or no; what stops it is the worker being
      blocked, at either width, for the worker isolates the page and is the
      reason there are threads to draw on. Where one refuses to be taken, the
      page said nothing but "starting" for as long as it was left open -- which
      says neither what went wrong nor that anything had. It says what is
      missing now (`web/bootstrap.js`), and
      `tools/probe_worker_blocked.mjs` reads that back at both widths.

## Cross-stack benchmark idea

- [ ] **Build the same simple app on different stacks, measure them.** A first
      Android start was long and unsuccessful, which suggested the experiment:
      write one small identical app -- same screen, same draw, same ask -- on
      this stack and on others, then compare what each costs to start, draw and
      hold (start time, frames, memory, binary size). The point is the numbers,
      not the app: a stack chosen by looking is a guess, one chosen by timing
      on the same machine is not. Where the stacks differ (C++26+skiff here,
      others elsewhere) the same screen must be honest about it -- one room,
      one text, one press, a stopwatch on each.
      A ready-made bench candidate: `j4niwzis/calculator`.
