// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.opened -- A place opened where this machine opens places: a
// link said in a message or an answer, given to whatever the machine has for
// one -- a browser of its own on a desk and a phone, another of this page's
// tabs in a browser. What was opened is not this program's to know about
// afterwards; whether the machine was asked is all it is told.
//
// Nothing above this knows which way it happened.
module;
#include <SDL3/SDL.h>
// SPDX-License-Identifier: AGPL-3.0-only
export module pchat.platform.opened;

import std;

export namespace pchat::platform::opened {

// Given to the machine, and asked for nothing back: SDL asks the desk, the
// phone and the window host each in the way they have. Where it refused (no
// browser to hand it to), this says so.
bool open(std::string_view target) {
  if (target.empty())
    return false;
  return SDL_OpenURL(std::string{target}.c_str());
}

}  // namespace pchat::platform::opened
