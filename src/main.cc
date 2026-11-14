// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat: the program. The window on the main thread, and -- once
// there is one -- the client on another; what the client says crosses to the
// window as changes to the model, and what the window asks for is asked of
// the client.
//
//   progressive-chat   the conversations of the sample, which is all the
//                      model has until the back end's own half fills it
module;
#include "serve.h"

import std;
import skiff.platform.window;
import skiff.platform.events;
import pchat.platform.kept;
import pchat.platform.drawn;
import pchat.app.program;

using namespace pchat::app;

// The window: what the host draws it through. Declared here, in a unit that
// is no module's, so either host -- window_host.cc for SDL3,
// browser_host.cc for the page -- can define it.
int pchat_run(program &one, const skiff::platform::window::options &how,
              const skiff::platform::events::kinds &kinds);

// Shown in skiff's own window (window_host.cc), in the page
// (browser_host.cc), or as an APK (main_android.cc).
int pchat_main() {
  const skiff::platform::events::kinds kinds;
  // The tree is one object the size of a windowful of parts: in static
  // storage, as Android's native activity has less of a stack than a window
  // needs.
  static std::optional<program> held;
  program &one = held.emplace();
  // The program's own door, up before anything asks through it: on a host the
  // ported server stands on, it answers this program's own wire (its base
  // '/v1') while the window is being made; where no such host is here -- a
  // page, a phone -- nothing stands and nothing changes.
  (void)pchat::serve::up();
  // Where the programs the GPU compiled are kept between runs: compiled
  // again at every run without it, and a phone is slow at it.
  // Software drawing when no GPU can be drawn through: without this it went
  // before its first frame wherever GL was not to be had.
  const skiff::platform::window::options how{
      .title = "progressive-chat",
      .width = 1100,
      .height = 720,
      .software = !pchat::platform::gpu_can_be_had(),
      .programs = pchat::platform::where_programs_are_kept()};
  return pchat_run(one, how, kinds);
}
