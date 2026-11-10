// SPDX-License-Identifier: AGPL-3.0-only
// What a ChatGPT export is, read into this program's own words: said across
// the one line a module unit and a classic translation unit may share, the
// same way the fitting is (src/provider/budget.h) -- the ported importer
// behind it stays in the classic unit, where agora's headers live
// (src/logic/gpt_import.cpp).
#ifndef PCHAT_LOGIC_GPT_IMPORT_H
#define PCHAT_LOGIC_GPT_IMPORT_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pchat::logic {

// One conversation of an export, as this program keeps it: its own id from
// the file, what it is called, and when it was last spoken in (milliseconds
// since the epoch, which is what a moment is here).
struct gpt_conversation {
  std::string id;
  std::string title;
  std::int64_t updated = 0;
};

// One message of one of them: whose it is by whether the reader said it,
// what it says, what it thought before answering where the export kept
// that, and when it was said.
struct gpt_message {
  std::string conversation;
  std::string id;
  std::string text;
  bool from_reader = false;
  std::int64_t when = 0;
  std::int32_t tokens = 0;
  std::string thought;
  std::int64_t thought_ms = 0;
};

// One export read: whole, or not read at all -- and where it was not, what
// the file said of itself instead (a decode failure, a zip that would not
// open), kept as words to say later.
struct gpt_extracted {
  bool ok = false;
  std::vector<gpt_conversation> conversations;
  std::vector<gpt_message> messages;
  std::string failure;
};

// The bytes of a ChatGPT export read as far as this program takes them: the
// zip archive of conversations.json it ships as, or the bare JSON array a
// reader may save by hand (the ported importer knows both --
// extractAndParse).
[[nodiscard]] gpt_extracted gpt_extract(std::string_view bytes);

}  // namespace pchat::logic

#endif  // PCHAT_LOGIC_GPT_IMPORT_H
