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
// stands, and what this program reads of it -- either the whole of a text
// file as far as a message should carry, or a picture whole as it is (its
// own bytes, base64'd for every wire that reads a picture that way, and the
// media type its own first bytes say). Video, PDFs and archives are still
// not attached rather than attached wrong: nothing here reads those yet.
struct attachment {
  std::string name;
  std::string text;
  // A picture, where the file was one: `image/png` and friends from the
  // magic bytes, and the bytes whole, encoded once here so that no wire and
  // no file in an archive encodes them again.
  std::string media;
  std::string image;
  friend bool operator==(const attachment&, const attachment&) = default;
};

// How much of a picture is attached at all: eight mebibytes of the file as
// it stands, before anything is encoded -- a whole photograph beside a
// message this program may carry is one thing, and the largest answer a
// provider will read in one request is another (the encoding of eight
// mebibytes is nearly eleven, which every wire here has room for).
inline constexpr std::size_t attachment_image = 8u * 1024u * 1024u;

// What one picture counts against a window, said before any wire is asked
// and kept with the message: the same reservation Agora's cost model makes
// for a picture whose dimensions it does not know -- see `send`.
inline constexpr std::uint32_t image_window_cost = 1024u;

// What a file is a picture of, said by its own first bytes: the four
// formats this program attaches, and nothing at all where the file is
// another thing -- a picture is known by what it starts with, which is how
// every reader of one has always known.
[[nodiscard]] inline std::optional<std::string> image_media_of(
    std::string_view bytes) {
  const auto starts_with = [&bytes](std::initializer_list<unsigned char> sig) {
    if (bytes.size() < sig.size())
      return false;
    std::size_t at = 0;
    for (const unsigned char want : sig)
      if (static_cast<unsigned char>(bytes[at++]) != want)
        return false;
    return true;
  };
  if (starts_with({0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a}))
    return std::string("image/png");
  if (starts_with({0xff, 0xd8, 0xff}))
    return std::string("image/jpeg");
  if (starts_with({'G', 'I', 'F', '8'}))
    return std::string("image/gif");
  if (bytes.size() >= 12u && starts_with({'R', 'I', 'F', 'F'}) &&
      bytes.substr(8u, 4u) == "WEBP")
    return std::string("image/webp");
  return std::nullopt;
}

// The bytes of anything, said the way every wire that carries a picture says
// them: base64, padded, four characters to every three bytes.
[[nodiscard]] inline std::string base64_of(std::string_view bytes) {
  constexpr std::string_view alphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((bytes.size() + 2u) / 3u * 4u);
  for (std::size_t at = 0u; at < bytes.size(); at += 3u) {
    const std::size_t left = bytes.size() - at;
    std::uint32_t bits = static_cast<unsigned char>(bytes[at]) << 16;
    if (left > 1u)
      bits |= static_cast<unsigned char>(bytes[at + 1u]) << 8;
    if (left > 2u)
      bits |= static_cast<unsigned char>(bytes[at + 2u]);
    out.push_back(alphabet[(bits >> 18) & 63u]);
    out.push_back(alphabet[(bits >> 12) & 63u]);
    out.push_back(left > 1u ? alphabet[(bits >> 6) & 63u] : '=');
    out.push_back(left > 2u ? alphabet[bits & 63u] : '=');
  }
  return out;
}

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

// One file decided: a picture first -- by the bytes it starts with, so that
// a picture without a NUL in it anywhere is still a picture and not text --
// then what it says as text, and nothing at all where it is neither (a
// video, a PDF, an archive, or a picture over the room one message should
// carry). The name it is attached under is its own, wherever it stood.
[[nodiscard]] inline std::optional<attachment> attachment_of(
    std::string_view path, std::string_view bytes) {
  const std::string name = file_name_of(path);
  if (bytes.size() <= attachment_image) {
    if (const auto media = image_media_of(bytes))
      return attachment{.name = name, .media = *media,
                        .image = base64_of(bytes)};
  }
  if (const auto text = attachment_text_of(bytes); text && !text->empty())
    return attachment{.name = name, .text = *text};
  return std::nullopt;
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
  // What pictures came with it, kept as they are: media said by the file's
  // own bytes, bytes base64'd once already (attachment_of did it).
  for (const attachment &one : asking.attachments)
    if (!one.image.empty())
      said.images.push_back(image_part{one.media, one.image});
  // What the reader asked for, worth what Agora's counter says it is worth,
  // counted where it is written rather than at some later projection -- and
  // what is attached is written now too, so it is counted now too. A picture
  // is counted at the reservation Agora's own cost model puts on one whose
  // dimensions it does not know (message_preparation_tokens.cpp:
  // kMissingDimensionsReservation), so the circle in the foot says what the
  // window has really spent and not only what the words came to.
  said.tokens = static_cast<std::uint32_t>(pchat::tokens::of(said.text)) +
                static_cast<std::uint32_t>(said.images.size()) *
                    image_window_cost;
  return {.said = std::move(said), .cleared = composer_state{}};
}

}  // namespace pchat::logic
