// SPDX-License-Identifier: AGPL-3.0-only
// What this program keeps of a conversation, through the ported tree's own
// store (ports/agora-core chat_dao): the rows a conversation is made of, and
// one message kept and read back whole.
//
// The face is plain -- ids, titles, text, counts -- because a header shared
// with a module unit may not name the ported headers or this tree's own
// modules. The ported ChatEntity/MessageEntity stay behind keep.cpp, where
// agora's headers are read, the way the counters and the wire are read there.
#ifndef PCHAT_KEEP_H
#define PCHAT_KEEP_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pchat::keep {

// One conversation as the store knows it: what its row says.
struct conversation_row {
  std::string id;
  std::string title;
  std::int64_t last_updated = 0;
  bool pinned = false;
  std::string model_id;
};

// One message as the store knows it: what its row says.
struct message_row {
  std::string id;
  std::string conversation_id;
  std::string text;
  int token_count = 0;
  std::int64_t timestamp = 0;
  bool outgoing = false;
  std::string why_failed;
  // What thought it, before it said: the thought's own text, empty where
  // nothing was thought. Its times and calls stay in the program's tree --
  // the store keeps the words.
  std::string thought;
  // The model that answered (empty: the reader wrote this one).
  std::string model;
};

// Opens the store's own database, empty, with the ported schema on it: a
// store of one process. No path says a store that is here and nowhere after
// this run -- the rows live in memory and go with it. Says whether it opened
// at all.
[[nodiscard]] bool open();

// The same, at a file: the rows are read back from what a last run kept and
// written to it from here. Says whether it opened -- a page has no files beside
// it, and where the file cannot stand, no store does.
[[nodiscard]] bool open_at(const std::string &path);

// Whether the store is open and answering.
[[nodiscard]] bool is_open();

// Closes it, if open.
void close();

// Keeps a conversation, so a message kept under its id has somewhere to stand.
void keep_conversation(conversation_row one);

// Keeps one message in the conversation whose id it names. The conversation
// must be kept first: a message answers to a row that is there.
void keep_message(message_row one);

// Every conversation the store holds, as its row says them.
[[nodiscard]] std::vector<conversation_row> conversations();

// The messages of one conversation, in the order they were kept.
[[nodiscard]] std::vector<message_row> messages_of(const std::string &conversation_id);

// One message held out of the store by its id: what it said, under the
// conversation it said it in. Nothing where the id is not held -- a line an
// answer was cut off mid-write, or a message the store never saw, is asked
// of again, not read back half.
struct kept_of_row {
  bool found = false;
  std::string conversation_id;
  std::string text;
};

// The message with this id, whole, from the store.
[[nodiscard]] kept_of_row kept_of(const std::string &message_id);

}  // namespace pchat::keep

#endif  // PCHAT_KEEP_H
