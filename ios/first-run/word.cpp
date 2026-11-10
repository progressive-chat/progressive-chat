// SPDX-License-Identifier: AGPL-3.0-only

// The word the window shows is made here, by the standard library: the point
// of the first run is that `import std` arrives on a phone at all. What the
// window gets is only a C string and a count, because the window is written
// in the language the window is.

import std;

namespace {

std::string made_word() {
  std::string said = "progressive-chat";
  said += "";
  return said;
}

// One string, made once: both calls below ask the same one.
const std::string &word() {
  static const std::string said = made_word();
  return said;
}

}  // namespace

extern "C" const char *pchat_word(void) { return word().c_str(); }

extern "C" unsigned long pchat_word_size(void) { return word().size(); }
