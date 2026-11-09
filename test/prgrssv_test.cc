module;
#include "tests.h"
#include "prgrssv_zip.h"
export module pchat.test.prgrssv;
// SPDX-License-Identifier: AGPL-3.0-only
// The archive the program writes: everything in it, and back out of it whole --
// a manifest that says what it is, a conversation whose messages carry what
// thought them, and the settings the page had set.
import std;
import pchat.core;
import pchat.config;
import pchat.theme;
import pchat.json;
import pchat.prgrssv;

using namespace pchat;

namespace {

// What one file in an archive says, where it stands by that name: nothing
// where the archive holds no such file.
const std::string *body_of(const std::vector<pchat::prgrssv::stored> &in,
                           std::string_view name) {
  for (const pchat::prgrssv::stored &one : in)
    if (one.name == name)
      return &one.body;
  return nullptr;
}

// One file read whole: nothing where the file is missing or says something
// that is not JSON, so that every check under it asks only what it holds.
std::optional<json::value> read_body(const std::string *file) {
  if (file == nullptr)
    return std::nullopt;
  auto whole = json::read(*file);
  if (!whole)
    return std::nullopt;
  return std::move(*whole);
}

// What stands down a path inside a value: member names and item numbers one
// after another ("messages/1/thought/said"), and nothing where any step of it
// is missing.
const json::value *along(const json::value &one, std::string_view path) {
  const json::value *at = &one;
  while (!path.empty() && at != nullptr) {
    const std::size_t cut = path.find('/');
    const std::string_view head = path.substr(0, cut);
    path = cut == std::string_view::npos ? std::string_view{}
                                         : path.substr(cut + 1);
    if (head.empty())
      return nullptr;
    bool counted = true;
    for (const char figure : head)
      if (figure < '0' || figure > '9') {
        counted = false;
        break;
      }
    if (counted) {
      std::size_t item = 0u;
      for (const char figure : head)
        item = item * 10u + static_cast<std::size_t>(figure - '0');
      at = json::item_of(*at, item);
    } else {
      at = json::said(*at, head);
    }
  }
  return at;
}

// What a path says as text: nothing where the path does not hold or holds no
// text -- one line saying what was written and what came back of it.
std::string_view text_at(const json::value &one, std::string_view path) {
  const json::value *at = along(one, path);
  return at == nullptr ? std::string_view{}
                       : json::text_of(*at).value_or(std::string_view{});
}

// What a path says as a number: nothing where it holds none.
double number_at(const json::value &one, std::string_view path) {
  const json::value *at = along(one, path);
  return at == nullptr ? -1.0 : json::number_of(*at).value_or(-1.0);
}

// What a path says of whether something is on: unset where the path does not
// hold, which is not the same as off.
std::optional<bool> truth_at(const json::value &one, std::string_view path) {
  const json::value *at = along(one, path);
  return at == nullptr ? std::nullopt : json::truth_of(*at);
}

}  // namespace

SAID(prgrssv, everything_held_comes_back_as_it_was) {
  chats_root root;
  root.accounts.push_back(account{.id = account_id{"home"},
                                  .name = "Home",
                                  .server = "https://chat.example"});
  conversation room{.id = conversation_id{"kitchen"},
                    .account = account_id{"home"},
                    .title = "What is in the kitchen",
                    .kind = conversation_kind::direct,
                    .last_activity = 12};
  room.models.push_back("haiku-4");
  room.told = "Answer briefly.";
  room.tokens = 9;
  room.window = 1000000;
  room.context_used = 0.25f;
  root.conversations.push_back(std::move(room));
  root.messages.push_back(message{.id = message_id{"kitchen-1"},
                                  .chat = conversation_id{"kitchen"},
                                  .author = "reader",
                                  .outgoing = true,
                                  .text = "two \"quotes\"\nand a dash",
                                  .sent = 11,
                                  .state = delivery::sent,
                                  .tokens = 4});
  root.messages.push_back(
      message{.id = message_id{"kitchen-2"},
              .chat = conversation_id{"kitchen"},
              .author = "haiku-4",
              .text = "The pantry has bread.",
              .sent = 12,
              .state = delivery::seen,
              .before =
                  thought{.took_ms = 420,
                          .tool_calls = 1,
                          .said = "thinking \"hard\"",
                          .calls = {call_made{
                              .tool = "conversation_search",
                              .asked = "in the kitchen?",
                              .answered = "bread in the pantry"}}},
              .tokens = 7});

  const std::string bytes =
      prgrssv::archive_of(root, default_looks, default_asking);
  std::vector<pchat::prgrssv::stored> in;
  EXPECT_TRUE(pchat::prgrssv::unzipped(bytes, in));

  // Three files: the index, the one conversation, and the settings.
  EXPECT_EQ(in.size(), 3u);

  // The manifest says what the archive is and what it holds.
  const std::optional<json::value> manifest =
      read_body(body_of(in, "manifest.json"));
  EXPECT_TRUE(manifest.has_value());
  if (manifest) {
    EXPECT_EQ(text_at(*manifest, "format"), "prgrssv");
    EXPECT_EQ(number_at(*manifest, "version"), 1.0);
    EXPECT_TRUE(text_at(*manifest, "program")
                    .starts_with("progressive-chat "));
    EXPECT_EQ(text_at(*manifest, "accounts/0/name"), "Home");
    EXPECT_EQ(number_at(*manifest, "conversations"), 1.0);
    EXPECT_EQ(number_at(*manifest, "messages"), 2.0);
  }

  // The conversation: what it is called, which model answers in it, and both
  // messages whole -- the escaping done to the first undone by reading it.
  const std::optional<json::value> room_json =
      read_body(body_of(in, "conversations/kitchen.json"));
  EXPECT_TRUE(room_json.has_value());
  if (room_json) {
    EXPECT_EQ(text_at(*room_json, "title"), "What is in the kitchen");
    EXPECT_EQ(text_at(*room_json, "kind"), "direct");
    EXPECT_EQ(text_at(*room_json, "models/0"), "haiku-4");
    // The messages stand as the list they were said in, and not as a count:
    // two of them -- the count of them is the manifest's own, said above.
    const json::value *messages = json::said(&*room_json, "messages");
    EXPECT_TRUE(messages != nullptr);
    if (messages) {
      EXPECT_TRUE(json::item_of(*messages, 1) != nullptr);
      EXPECT_TRUE(json::item_of(*messages, 2) == nullptr);
    }
    EXPECT_EQ(text_at(*room_json, "messages/0/text"),
              "two \"quotes\"\nand a dash");
    EXPECT_TRUE(truth_at(*room_json, "messages/0/outgoing") == true);
    EXPECT_EQ(text_at(*room_json, "messages/0/state"), "sent");
    EXPECT_EQ(number_at(*room_json, "messages/0/sent"), 11.0);
    EXPECT_EQ(text_at(*room_json, "messages/1/author"), "haiku-4");
    EXPECT_EQ(number_at(*room_json, "messages/1/thought/took_ms"), 420.0);
    EXPECT_EQ(text_at(*room_json, "messages/1/thought/said"),
              "thinking \"hard\"");
    EXPECT_EQ(number_at(*room_json, "messages/1/thought/tool_calls"), 1.0);
    EXPECT_EQ(text_at(*room_json, "messages/1/thought/calls/0/tool"),
              "conversation_search");
    EXPECT_EQ(text_at(*room_json, "messages/1/thought/calls/0/asked"),
              "in the kitchen?");
    EXPECT_EQ(text_at(*room_json, "messages/1/thought/calls/0/answered"),
              "bread in the pantry");
    EXPECT_EQ(text_at(*room_json, "messages/1/state"), "seen");
  }

  // The settings: how the page was set when the archive was written.
  const std::optional<json::value> settings =
      read_body(body_of(in, "settings.json"));
  EXPECT_TRUE(settings.has_value());
  if (settings) {
    EXPECT_EQ(text_at(*settings, "looks/preset"),
              "forest");  // the default the page starts on
    EXPECT_TRUE(truth_at(*settings, "looks/amoled") == false);
    EXPECT_EQ(number_at(*settings, "looks/font_scale"), 1.0);
    EXPECT_TRUE(truth_at(*settings, "asking/thinking") == true);
    EXPECT_EQ(number_at(*settings, "asking/temperature"), 0.7);
    EXPECT_EQ(number_at(*settings, "asking/max_tokens"), 2048.0);
    EXPECT_TRUE(truth_at(*settings, "asking/tools/conversation_search") ==
                true);
    EXPECT_TRUE(truth_at(*settings, "asking/switches/remember") == true);
  }
}

SAID(prgrssv, an_empty_program_still_writes_a_whole_archive) {
  const chats_root none;
  const std::string bytes =
      prgrssv::archive_of(none, default_looks, default_asking);
  std::vector<pchat::prgrssv::stored> in;
  EXPECT_TRUE(pchat::prgrssv::unzipped(bytes, in));

  // The manifest and the settings hold even where there is nothing to count.
  EXPECT_EQ(in.size(), 2u);
  const std::optional<json::value> manifest =
      read_body(body_of(in, "manifest.json"));
  EXPECT_TRUE(manifest.has_value());
  if (manifest) {
    EXPECT_EQ(text_at(*manifest, "format"), "prgrssv");
    EXPECT_EQ(number_at(*manifest, "conversations"), 0.0);
    EXPECT_EQ(number_at(*manifest, "messages"), 0.0);
  }
}

SAID(prgrssv, everything_it_held_reads_back_where_it_was_held) {
  chats_root root;
  root.accounts.push_back(account{.id = account_id{"home"},
                                  .name = "Home",
                                  .server = "https://chat.example"});
  conversation room{.id = conversation_id{"kitchen"},
                    .account = account_id{"home"},
                    .title = "What is in the kitchen",
                    .kind = conversation_kind::direct,
                    .last_activity = 12};
  room.models.push_back("haiku-4");
  room.told = "Answer briefly.";
  room.tokens = 9;
  room.window = 1000000;
  room.context_used = 0.25f;
  root.conversations.push_back(std::move(room));
  root.opened = conversation_id{"kitchen"};
  root.messages.push_back(message{.id = message_id{"kitchen-1"},
                                  .chat = conversation_id{"kitchen"},
                                  .author = "reader",
                                  .outgoing = true,
                                  .text = "two \"quotes\"\nand a dash",
                                  .sent = 11,
                                  .state = delivery::sent,
                                  .tokens = 4});
  root.messages.push_back(
      message{.id = message_id{"kitchen-2"},
              .chat = conversation_id{"kitchen"},
              .author = "haiku-4",
              .text = "The pantry has bread.",
              .sent = 12,
              .state = delivery::seen,
              .replies_to = message_id{"kitchen-1"},
              .before = thought{.took_ms = 420,
                                .tool_calls = 1,
                                .said = "thinking \"hard\"",
                                .calls = {call_made{
                                    .tool = "conversation_search",
                                    .asked = "in the kitchen?",
                                    .answered = "bread in the pantry"}}},
              .tokens = 7});
  looks shown = default_looks;
  shown.preset = scheme_preset::rose;
  shown.mode = theme_mode::light;
  shown.font_scale = 1.25f;
  asking how = default_asking;
  how.thinking = false;
  how.how_hard = effort::most;
  how.temperature = 0.25;
  how.max_tokens = 4096;
  how.tools[1] = false;
  how.switches[2] = true;

  // What one run held, said into the start of the next: every file read back
  // into the places it was held in, and nothing of the bytes left over.
  const std::string bytes = prgrssv::archive_of(root, shown, how);
  const prgrssv::restored back = prgrssv::restore_of(bytes);
  EXPECT_TRUE(back.ok());
  if (!back.ok())
    return;
  EXPECT_EQ(back.root, root);
  EXPECT_EQ(back.shown, shown);
  EXPECT_EQ(back.how, how);
}

SAID(prgrssv, bytes_that_were_never_an_archive_say_so) {
  const prgrssv::restored back =
      prgrssv::restore_of("these bytes are not a zip, not by any reading of them");
  EXPECT_FALSE(back.ok());
  EXPECT_FALSE(back.trouble.empty());
  // And an archive of nothing at all is not one either: what the manifest says
  // is what the rest of the file is read as.
  const prgrssv::restored hollow = prgrssv::restore_of(prgrssv::zipped({}));
  EXPECT_FALSE(hollow.ok());
}

SAID(prgrssv, the_door_a_run_asked_through_comes_back_with_the_next_one) {
  chats_root root;
  const pchat::door where{
      .provider = "anthropic",
      .keys = {pchat::door_key{.provider = "anthropic",
                               .key = "sk-kept-whole"}}};
  const std::string bytes =
      prgrssv::archive_of(root, default_looks, default_asking, where);

  // The door stands in the settings whole: which of them is chosen, and what
  // opens it -- both said the way every other setting of this file is said.
  std::vector<pchat::prgrssv::stored> in;
  EXPECT_TRUE(pchat::prgrssv::unzipped(bytes, in));
  const std::optional<json::value> settings =
      read_body(body_of(in, "settings.json"));
  EXPECT_TRUE(settings.has_value());
  if (settings) {
    EXPECT_EQ(text_at(*settings, "door/provider"), std::string("anthropic"));
    EXPECT_EQ(text_at(*settings, "door/keys/0/provider"),
              std::string("anthropic"));
    EXPECT_EQ(text_at(*settings, "door/keys/0/key"),
              std::string("sk-kept-whole"));
  }

  // And back whole for the run that reads this file at its start: the door
  // as it was stood, and what opens it as it was written.
  const pchat::prgrssv::restored back = pchat::prgrssv::restore_of(bytes);
  EXPECT_TRUE(back.ok());
  EXPECT_EQ(back.where.provider, std::string("anthropic"));
  EXPECT_EQ(pchat::key_of(back.where, "anthropic"),
            std::string("sk-kept-whole"));

  // A save asked to be handed around is given no key at all: which door was
  // chosen is still said -- it is a setting like any other -- but what opens
  // it stands only where it was typed, and never in a file given away.
  const std::string given_away =
      prgrssv::archive_of(root, default_looks, default_asking);
  const pchat::prgrssv::restored elsewhere =
      pchat::prgrssv::restore_of(given_away);
  EXPECT_TRUE(elsewhere.ok());
  EXPECT_EQ(elsewhere.where.provider, std::string("page"));
  EXPECT_TRUE(elsewhere.where.keys.empty());
  EXPECT_EQ(pchat::key_of(elsewhere.where, "anthropic"), std::string{});
}

int main() { return pchat::tests::run(); }
