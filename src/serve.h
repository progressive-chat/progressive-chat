// SPDX-License-Identifier: AGPL-3.0-only
// The program's own door: the ported HTTP server answering this program's own
// wire (its base '/v1'), so a page and a desktop have something behind the
// address they ask. One canned answer today -- the words a model says when it
// has none to give and says so -- which is enough for the wire to be spoken
// and the round to be whole.
//
// The face is plain: a port number, a start, a stop. The ported server and the
// SSE bytes stay behind serve.cpp, the way the store's schema does.
#ifndef PCHAT_SERVE_H
#define PCHAT_SERVE_H

namespace pchat::serve {

// Brings the door up on the loopback, at the first port it can have (the
// ported server takes 0 for 'any'): says whether it stands now.
[[nodiscard]] bool up();

// Whether it is standing.
[[nodiscard]] bool is_up();

// The port it stands on: zero where it does not.
[[nodiscard]] int port();

// Brings it down, if up.
void down();

}  // namespace pchat::serve

#endif  // PCHAT_SERVE_H
