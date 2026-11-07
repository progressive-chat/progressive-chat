// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.notified -- One notification, put where the machine this
// program runs on puts notifications, for a screen to ask for: the title and
// the body go in, and whether anything was shown comes out.
//
// Behind it is what the platform gives (src/platform/notified_backend*.cc):
// a notification of Android's own on a phone, and nothing at all everywhere
// else, which this says in what it answers with. Nothing above this knows
// which of them it got.
export module pchat.platform.notified;

import std;
import pchat.platform.notified.backend;

export namespace pchat::platform::notified {

bool say(std::string_view title, std::string_view text) {
  return backend::say(title, text);
}

}  // namespace pchat::platform::notified
