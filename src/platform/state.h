// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.state -- What this machine keeps of the program between one
// run and the next: the bytes of an archive in, the same bytes out, and no
// bytes at all where this platform keeps nothing (a page and a phone keep
// nothing yet -- src/platform/state.cpp says so, and says why there).
//
// Said across the one line a module unit and a classic translation unit may
// share, with the file itself on the classic side where the standard's own
// headers live (src/tokens_worth.h does it the same way).
#ifndef PCHAT_PLATFORM_STATE_H
#define PCHAT_PLATFORM_STATE_H

#include <string>
#include <string_view>

namespace pchat::platform {

// Whether this platform keeps anything at all: where it does not, reading is
// empty and writing is refused, and nothing above has to be two programs.
[[nodiscard]] bool keeps_state();

// What it kept from the last run, whole, and nothing where it kept nothing --
// where it kept something unreadable, that is the same as nothing.
[[nodiscard]] std::string state_read();

// What to keep for the next run: false where it could not be written, which
// a caller may say aloud but must not stop for -- a run whose state could not
// be written is still the run it was.
[[nodiscard]] bool state_write(std::string_view bytes);

}  // namespace pchat::platform

#endif  // PCHAT_PLATFORM_STATE_H
