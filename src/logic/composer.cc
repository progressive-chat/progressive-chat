// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.composer -- What the field at the bottom holds, and what
// sending it makes: one message in the conversation, and the field empty
// again.
export module pchat.logic.composer;

import std;
import pchat.core;
import pchat.tokens;

export namespace pchat::logic {

// One file attached to what is being written: what it is called where it
// stands, and what this program reads of it -- the whole of a text file as
// far as a message should carry, which is all an attachment is in this tree
// today (pictures, video and PDFs are not read into anything yet, and are
// not attached rather than attached wrong; `attachment_text_of` below says
// which is which).
struct attachment {
  std::string name;
  std::string text;
  friend bool operator==(const attachment&, const attachment&) = default;
};

// What a file is called where it stands: everything after the last slash of
// its path, so that what is named in a message and on the card is the file's
// own name and not where the machine happened to keep it.
[[nodiscard]] inline std::string file_name_of(std::string_view path) {
  const std::size_t slash = path.find_last_of("/\\\\");
  return std::string(slash == std::string_view::npos ? path
                                                     : path.substr(slash + 1u));
}

// How much of a file is read into one message, and how much is left where it
// stands: 64 KiB, cut on a whole character -- a byte continuing one is never
// the last one kept.
inline constexpr std::size_t attachment_read = 64u * 1024u;

// What of a file attached is text to read: its bytes as they are, as far as
// a message should carry -- and nothing at all where the file is not text
// (a NUL byte in the first page of it says a picture, an archive or a
// program, and none of those is words this program can hand to a wire).
[[nodiscard]] inline std::optional<std::string> attachment_text_of(
    std::string_view bytes) {
  const std::size_t page = std::min(bytes.size(), std::size_t{8192});
  if (bytes.substr(0u, page).find('\0') != std::string_view::npos)
    return std::nullopt;
  std::size_t cut = std::min(bytes.size(), attachment_read);
  if (cut < bytes.size())
    while (cut > 0u && (static_cast<unsigned char>(bytes[cut]) & 0xc0u) == 0x80u)
      --cut;
  return std::string(bytes.substr(0u, cut));
}

// The field: what is written in it, what is attached to it, what it answers,
// and whether it has been opened out to the whole window.
struct composer_state {
  std::string text;
  std::vector<attachment> attachments;
  std::optional<message_id> replies_to;
  bool expanded = false;
  friend bool operator==(const composer_state&, const composer_state&) = default;
};

// Whether there is anything to send: a message with nothing in it and nothing
// on it says nothing, and the button that sends it stays dark.
[[nodiscard]] inline bool can_send(const composer_state& field) {
  return !field.attachments.empty()
      || !std::ranges::all_of(field.text, [](char unit) {
           return unit == ' ' || unit == '\n' || unit == '\r' || unit == '\t';
         });
}

// What sending asks for, once its conversation is known.
struct sending_request {
  conversation_id chat;
  std::string text;
  std::vector<attachment> attachments;
  std::optional<message_id> replies_to;
};

// The id a new message gets: its own number in nanoseconds, which is what
// nothing else could have handed out for it yet.
[[nodiscard]] inline std::string new_message_id(moment when) { return std::to_string(when); }

// Sent: the message it becomes, still going out, and the field as it was
// before anything was written in it.
struct sent_message {
  message said;
  composer_state cleared;
};

[[nodiscard]] inline sent_message send(sending_request asking, std::string author, moment when) {
  // What is attached, said where it is sent: whole, under the name the file
  // is called by -- a text file asked about is asked about by what it says,
  // and a message with nothing written on it but its attachments says the
  // same thing as one with writing and attachments together. One way it is
  // written, for every wire this program speaks reads a message as text
  // (content parts for what a picture says come with the pictures).
  std::string body = std::move(asking.text);
  for (const attachment &one : asking.attachments) {
    if (!body.empty())
      body += "\n\n";
    body += "[" + one.name + "]";
    if (!one.text.empty())
      body += "\n" + one.text;
  }
  message said{.id = message_id{new_message_id(when)},
               .chat = std::move(asking.chat),
               .author = std::move(author),
               .outgoing = true,
               .text = std::move(body),
               .sent = when,
               .state = delivery::sending,
               .replies_to = std::move(asking.replies_to)};
  // What the reader asked for, worth what Agora's counter says it is worth,
  // counted where it is written rather than at some later projection -- and
  // what is attached is written now too, so it is counted now too.
  said.tokens = static_cast<std::uint32_t>(pchat::tokens::of(said.text));
  return {.said = std::move(said), .cleared = composer_state{}};
}

}  // namespace pchat::logic
