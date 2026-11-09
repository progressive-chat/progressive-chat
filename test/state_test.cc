module;
#include "tests.h"
#include <stdlib.h>
#include "../src/platform/state.h"
export module pchat.test.state;
// SPDX-License-Identifier: AGPL-3.0-only
// What a machine keeps of this program between one run and the next: written,
// read back whole, and read back as nothing once it is gone. What a platform
// that keeps nothing says is also this file's business: a page and a phone
// keep nothing today, and where that is so, there is nothing to write and
// nothing to read -- said by returning, and not by failing.
//
// The file itself goes where the program itself puts it: XDG state home, said
// here too, so that the test writes into a directory of its own instead of
// somebody's real one.
import std;

namespace {

// One directory for this test's own state, named by the moment it was made.
[[nodiscard]] std::string a_state_home() {
  const auto when = std::chrono::steady_clock::now().time_since_epoch().count();
  return (std::filesystem::temp_directory_path() /
          ("pchat-state-" + std::to_string(when)))
      .string();
}

}  // namespace

SAID(state, what_it_keeps_it_reads_back_whole) {
  if (!pchat::platform::keeps_state())
    return; // where nothing is kept there is nothing to say
  const std::string home = a_state_home();
  ::setenv("XDG_STATE_HOME", home.c_str(), 1);
  EXPECT_TRUE(pchat::platform::state_write("one archive, whole"));
  EXPECT_EQ(pchat::platform::state_read(), std::string("one archive, whole"));
  EXPECT_TRUE(pchat::platform::state_write("and another, whole"));
  EXPECT_EQ(pchat::platform::state_read(), std::string("and another, whole"));

  // What it kept, taken away: what is read back is nothing at all, and not a
  // piece of what stood there before it.
  std::error_code why;
  std::filesystem::remove_all(
      std::filesystem::path(home) / "progressive-chat" / "state.prgrssv", why);
  EXPECT_TRUE(!why);
  EXPECT_EQ(pchat::platform::state_read(), std::string{});

  // Nothing of this test is left in the machine it ran on.
  ::unsetenv("XDG_STATE_HOME");
  std::filesystem::remove_all(home, why);
}

int main() { return pchat::tests::run(); }
