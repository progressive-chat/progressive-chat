module;
#include "tests.h"
#include "prgrssv_read.h"
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
const std::string *body_of(const std::vector<pchat::test::zipped_entry> &in,
                           std::string_view name) {
  for (const pchat::test::zipped_entry &one : in)
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
  room.system_prompt = "Answer briefly.";
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
  const std::vector<pchat::test::zipped_entry> in =
      pchat::test::read_entries(bytes);

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
    EXPECT_EQ(number_at(*room_json, "messages"), 2.0);
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

// Every name said_of writes reads back to the enumerator it was said from --
// the pair the settings restore walks: a word from an earlier run means what
// it meant then, and a word never said keeps the default.
SAID(prgrssv, every_name_said_reads_back_as_it_was) {
  EXPECT_TRUE(prgrssv::theme_mode_of(prgrssv::said_of(theme_mode::light)) ==
              theme_mode::light);
  EXPECT_TRUE(prgrssv::theme_mode_of(prgrssv::said_of(theme_mode::system)) ==
              theme_mode::system);
  EXPECT_TRUE(prgrssv::effort_of(prgrssv::said_of(effort::least)) ==
              effort::least);
  EXPECT_TRUE(prgrssv::effort_of(prgrssv::said_of(effort::all)) ==
              effort::all);
  EXPECT_TRUE(prgrssv::font_choice_of(prgrssv::said_of(font_choice::custom)) ==
              font_choice::custom);
  EXPECT_TRUE(prgrssv::preset_of(prgrssv::said_of(scheme_preset::forest)) ==
              scheme_preset::forest);
  EXPECT_TRUE(prgrssv::preset_of("never-said") == default_preset);
  EXPECT_TRUE(prgrssv::style_of(prgrssv::said_of(scheme_style::expressive)) ==
              scheme_style::expressive);
  EXPECT_TRUE(prgrssv::style_of("") == default_style);
}

// What settings_of writes, read back the way the program reads it: the same
// walk restore_settings makes -- each name in the text meaning what it meant
// when the rows wrote it.
SAID(prgrssv, settings_said_come_back_as_they_were) {
  looks shown;
  shown.preset = scheme_preset::ocean;
  shown.mode = theme_mode::light;
  shown.font_scale = 1.5;
  shown.avatars = true;
  asking how;
  how.how_hard = effort::most;
  how.temperature = 1.25;
  how.max_tokens = 4096;
  how.tools[1] = false;
  how.switches[static_cast<std::size_t>(switch_choice::remember)] = false;

  const std::string text = prgrssv::settings_of(shown, how);
  const std::expected<json::value, json::read_error> read = json::read(text);
  EXPECT_TRUE(read.has_value());
  if (!read)
    return;
  EXPECT_EQ(text_at(*read, "looks/preset"), std::string_view("ocean"));
  EXPECT_EQ(text_at(*read, "looks/mode"), std::string_view("light"));
  EXPECT_EQ(number_at(*read, "looks/font_scale"), 1.5);
  EXPECT_TRUE(truth_at(*read, "looks/avatars").value_or(false));
  EXPECT_EQ(text_at(*read, "asking/how_hard"), std::string_view("most"));
  EXPECT_EQ(number_at(*read, "asking/temperature"), 1.25);
  EXPECT_EQ(number_at(*read, "asking/max_tokens"), 4096.0);
  EXPECT_TRUE(truth_at(*read, "asking/tools/how_much").value_or(false));
  EXPECT_TRUE(truth_at(*read, "asking/tools/conversation_search")
                  .value_or(true) == false);
  EXPECT_TRUE(
      truth_at(*read, "asking/switches/remember").value_or(true) == false);
}

SAID(prgrssv, an_empty_program_still_writes_a_whole_archive) {
  const chats_root none;
  const std::string bytes =
      prgrssv::archive_of(none, default_looks, default_asking);
  const std::vector<pchat::test::zipped_entry> in =
      pchat::test::read_entries(bytes);

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

int main() { return pchat::tests::run(); }
