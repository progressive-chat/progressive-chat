// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.state, where nothing is kept: a page keeps its bytes where
// the page's own host says to (and will, when it says so), a phone keeps them
// in its own files when it carries anything at all -- and until either does,
// this is the file that stands in for them, so that everything above asks the
// same question and hears the same honest no.
#include "state.h"

namespace pchat::platform {

bool keeps_state() { return false; }

std::string state_read() { return std::string{}; }

bool state_write(std::string_view) { return false; }

}  // namespace pchat::platform
