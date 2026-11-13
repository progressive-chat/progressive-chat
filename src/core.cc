// SPDX-License-Identifier: AGPL-3.0-only
// pchat.core -- The model every screen reads, and every protocol fills:
// the accounts, the conversations, the messages, and what has changed.
//
// Everything here is typed. What comes in from a protocol -- or from a file,
// later -- is read once, where it comes in, into these structs and enums, and
// nothing past that point looks at text again.
export module pchat.core;

import std;

export namespace pchat {

// Ids are opaque strings: compared and sorted, never parsed.
struct account_id {
  std::string value;
  friend bool operator==(const account_id &, const account_id &) = default;
  friend auto operator<=>(const account_id &, const account_id &) = default;
};
struct conversation_id {
  std::string value;
  friend bool operator==(const conversation_id &,
                         const conversation_id &) = default;
  friend auto operator<=>(const conversation_id &,
                          const conversation_id &) = default;
};
struct message_id {
  std::string value;
  friend bool operator==(const message_id &, const message_id &) = default;
  friend auto operator<=>(const message_id &, const message_id &) = default;
};

// Milliseconds since the epoch: what a protocol sends and nothing converts
// again.
using moment = std::int64_t;

enum class delivery { writing, sending, sent, failed, seen };
enum class conversation_kind { direct, group, channel };
enum class presence { offline, online, away, busy };
enum class membership { none, invited, joined, left };

// An account: what it is called, where it is kept, and how far it is in.
struct account {
  account_id id;
  std::string name;
  std::string server;
  membership state = membership::none;
  presence shown = presence::offline;
  friend bool operator==(const account &, const account &) = default;
};

// A conversation: one row of the drawer.
// The system prompt used when the conversation sets none: brief, plain, and
// named for the program.
inline constexpr std::string_view default_system_prompt =
    "You are answering in Progressive Chat, briefly and plainly.";

struct conversation {
  conversation_id id;
  account_id account;
  std::string title;
  conversation_kind kind = conversation_kind::direct;
  moment last_activity = 0;
  std::uint32_t unread = 0;
  bool pinned = false;
  bool muted = false;
  // Models that answer here. The first is the one the foot writes to.
  // tokens = what this chat has cost so far; window = room left in the
  // answerer's window.
  std::vector<std::string> models;
  // The system prompt this conversation sets; empty means the default.
  // What the model is told about itself before it is asked anything: the
  // conversation's system prompt.
  std::string system_prompt;
  std::uint32_t tokens = 0;
  std::uint32_t window = 0;
  float context_used = 0.0f;
  // The model that writes here by default: the first one, or none.
  [[nodiscard]] std::string leading_model() const {
    return models.empty() ? std::string{} : models.front();
  }
  // The system prompt actually sent: this conversation's, else the default.
  [[nodiscard]] std::string system_prompt_for() const {
    return system_prompt.empty() ? std::string(default_system_prompt) : system_prompt;
  }
  friend bool operator==(const conversation &, const conversation &) = default;
};

// What came before an answer, where one thought before it spoke: how long
// it was thinking, how many tool calls it made on the way, and what it
// wrote to itself while it was (its greentext).
// One tool a model stopped to ask for on its way to an answer: which one,
// what it asked it for, and what the tool said back. Kept against the thought
// that asked for it, so the fold above an answer can list each one and not
// only how many there were.
struct call_made {
  std::string tool;
  std::string asked;
  std::string answered;
  friend bool operator==(const call_made &, const call_made &) = default;
};

struct thought {
  std::int64_t took_ms = 0;
  std::uint32_t tool_calls = 0;
  std::string said;
  // Those calls, and what came of each of them: whole where they were asked
  // here and answered here, and empty where they were only counted.
  std::vector<call_made> calls{};
  friend bool operator==(const thought &, const thought &) = default;
};

// A message: one thing said in one conversation, by somebody or by its
// reader. Which of the two decides where it is drawn.
struct message {
  message_id id;
  conversation_id chat;
  std::string author;
  bool outgoing = false;
  std::string text;
  moment sent = 0;
  delivery state = delivery::sent;
  std::optional<message_id> replies_to;
  // Before an answer: what thought it, in how long and by how many calls.
  // Nothing of it where nobody thought -- where this was asked and answered.
  std::optional<thought> before;
  // What this one is worth, in tokens: read out of a protocol where it says,
  // counted out of its words where it does not.
  std::uint32_t tokens = 0;
  // Why it is a failure, in the words that said it: empty where nothing was
  // answered, and for every state that is not failed. Last of the fields, so
  // the braces a sample or a test writes whole keep their meaning.
  std::optional<std::string> why_failed;
  friend bool operator==(const message &, const message &) = default;
};

// All of it, as the model keeps it: three lists the screens read as ranges,
// and the conversation the window has open.
struct chats_root {
  std::vector<account> accounts;
  std::vector<conversation> conversations;
  std::vector<message> messages;
  std::optional<conversation_id> opened;
  friend bool operator==(const chats_root &, const chats_root &) = default;
};

// What changed: enough to know what to draw again, and no more.
struct chats_change {
  bool accounts = false;
  bool conversations = false;
  std::vector<conversation_id> conversations_changed;
  std::vector<message_id> messages_added;
  std::vector<message_id> messages_changed;
};

} // namespace pchat
