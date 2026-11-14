module;
#include <emscripten.h>
// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.opened (web) -- A place opened by a page: another tab of the
// browser this page runs in, asked for by its own window.open, which is what
// a page has where a desk and a phone have a browser of their own.
//
// Opened in a new tab and not in this one: this tab is the program, and a
// press on a link in a message is not a reason to lose it.
export module pchat.platform.opened;

import std;

export namespace pchat::platform::opened {

bool open(std::string_view target) {
  if (target.empty())
    return false;
  // A "_blank" whose name is forgotten, so every link opens a tab by itself
  // and two links do not share one; and "noopener", without which the page
  // opened is given the window this one was opened from -- this program's
  // whole tree -- and could reach into it.
  const int asked = EM_ASM_INT(
      {
        const place = UTF8ToString($0);
        const tab = window.open(place, '_blank', 'noopener');
        return tab ? 1 : 0;
      },
      std::string{target}.c_str());
  return asked != 0;
}

}  // namespace pchat::platform::opened
