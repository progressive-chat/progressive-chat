// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.state, where the machine keeps it: one file under the
// machine's own state home -- `$XDG_STATE_HOME/progressive-chat/state.prgrssv`,
// and `$HOME/.local/state/...` where XDG has not been said -- made whole
// before it is written, so that a first run and a run after it ask the same
// thing and get the same file. Where neither is said (no home), this
// platform keeps nothing either, the same as a page.
#include "state.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

namespace pchat::platform {
namespace {

// Where this machine keeps it, or nothing where the machine says nothing of a
// home to keep it beside.
[[nodiscard]] std::filesystem::path state_place() {
  const char *stated = std::getenv("XDG_STATE_HOME");
  std::filesystem::path base;
  if (stated != nullptr && stated[0] != '\0') {
    base = std::filesystem::path(stated);
  } else {
    const char *home = std::getenv("HOME");
    if (home == nullptr || home[0] == '\0')
      return std::filesystem::path{};
    base = std::filesystem::path(home) / ".local" / "state";
  }
  return base / "progressive-chat" / "state.prgrssv";
}

}  // namespace

bool keeps_state() { return !state_place().empty(); }

std::string state_read() {
  const std::filesystem::path place = state_place();
  if (place.empty())
    return std::string{};
  std::ifstream file(place, std::ios::binary);
  if (!file)
    return std::string{};
  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>{});
}

bool state_write(std::string_view bytes) {
  const std::filesystem::path place = state_place();
  if (place.empty())
    return false;
  std::error_code why;
  std::filesystem::create_directories(place.parent_path(), why);
  if (why)
    return false;
  std::ofstream file(place, std::ios::binary | std::ios::trunc);
  if (!file)
    return false;
  file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  return file.good();
}

}  // namespace pchat::platform
