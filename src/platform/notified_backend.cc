// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.notified.backend -- Where nothing puts notifications: this
// program asks the machine for none and is told that none was shown. What a
// window and a page say until they are given one of their own.
export module pchat.platform.notified.backend;

import std;

export namespace pchat::platform::notified::backend {

bool say(std::string_view, std::string_view) { return false; }

}  // namespace pchat::platform::notified::backend
