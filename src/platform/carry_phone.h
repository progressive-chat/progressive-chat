// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.carry_phone -- What a phone asks with: the HTTP stack its
// own machine carries (java.net.HttpURLConnection, asked of through JNI),
// which brings that machine's TLS, its certificates and its proxies with it
// and asks this tree to bring none of them. Reached over one plain line, with
// no JNI type in a signature, so that the module that carries never sees a
// Java header -- the same rule as the wires' (src/provider/anthropic/wire.h)
// and the counters' (src/tokens_worth.h).
//
// A window's own asking is pumped from the program's own loop because curl
// answers without being waited on. A phone's read waits on its socket, so
// the asking runs on a thread of its own and `phone_pump` takes from it the
// way everything else that carries takes: what arrived since the last pump,
// and where the asking stands. Nothing above knows the difference.
//
// What a phone may be asked over at all is its own policy's business:
// Android refuses cleartext `http://` by itself since 9, and nothing here
// overrides that -- providers are asked over https as they stand, and a
// model at home over http is the machine's own network security to allow.
#ifndef PCHAT_PLATFORM_CARRY_PHONE_H
#define PCHAT_PLATFORM_CARRY_PHONE_H

#include <span>
#include <string>
#include <string_view>

namespace pchat::platform {

// One asking, whole, over as long as somebody holds it.
struct phone_ask;

// Where a pump says the asking stands: still going; answered (what arrived
// is an answer and is taken as one); refused (a status of 400 or worse --
// its words in `why`, and never taken as an answer); lost (the connection
// went before an answer, which is the one case a carrying may ask again).
// The same four a window's own pump says, in the same words
// (src/platform/carry_desktop.h).
enum class phone_end { working, answered, refused, lost };

// Opened for one carrying, and closed through the pointer it gave.
phone_ask *phone_open();
void phone_close(phone_ask **one);

// Asked: the place, the headers as they were written (a name and its value,
// one after the other), and the body. False where the phone could not even
// begin asking.
[[nodiscard]] bool phone_ask_at(phone_ask *one, std::string_view url,
                                std::span<const std::string> fields,
                                std::string_view body);

// Everything arrived since the last pump, whole: appended to `piece`, which
// the caller takes as its own. What the asking stands at comes back, and
// where it is refused, `why` says what the server said of it.
[[nodiscard]] phone_end phone_pump(phone_ask *one, std::string &piece,
                                   std::string &why);

}  // namespace pchat::platform

#endif  // PCHAT_PLATFORM_CARRY_PHONE_H
