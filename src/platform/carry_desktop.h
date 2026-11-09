// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.carry_desktop -- What a window's own machine asks with: the
// curl it already links (the ported tree brings it in for its own transport,
// and this program's build has had it since), reached over one plain line so
// that curl's headers never enter a module unit -- the same rule as the wires'
// (src/provider/anthropic/wire.h) and the counters' (src/tokens_worth.h).
//
// No threads here: the transfer is driven by `desktop_pump`, which the
// carrying calls from the program's own loop and which never waits on a
// socket. A window that asks a question keeps drawing while it is asked.
#ifndef PCHAT_PLATFORM_CARRY_DESKTOP_H
#define PCHAT_PLATFORM_CARRY_DESKTOP_H

#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace pchat::platform {

// One asking, whole, over as long as somebody holds it.
struct desktop_ask;

// Where a pump says the asking stands: still going; answered (what arrived is
// an answer and is taken as one); refused (a status of 400 or worse -- its
// words in `why`, and never taken as an answer); lost (the connection went
// before an answer, which is the one case a carrying may ask again).
enum class desktop_end { working, answered, refused, lost };

// Opened for one carrying, and closed through the pointer it gave.
desktop_ask *desktop_open();
void desktop_close(desktop_ask **one);

// Asked: the place, the headers as they were written (a name and its value,
// one after the other), and the body. False where this machine could not even
// begin asking.
[[nodiscard]] bool desktop_ask_at(desktop_ask *one, std::string_view url,
                                  std::span<const std::string> fields,
                                  std::string_view body);

// Everything arrived since the last pump, whole: appended to `piece`, which
// the caller takes as its own. What the asking stands at comes back, and where
// it is refused, `why` says what the server said of it.
[[nodiscard]] desktop_end desktop_pump(desktop_ask *one, std::string &piece,
                                       std::string &why);

}  // namespace pchat::platform

#endif  // PCHAT_PLATFORM_CARRY_DESKTOP_H
