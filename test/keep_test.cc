// SPDX-License-Identifier: AGPL-3.0-only
// What the ported store keeps, read back: a conversation kept, a message kept
// under it, and both standing in what comes out. The store is the ported
// tree's own (ports/agora-core chat_dao); this asks it the way this program
// would, through the one plain header, with no network and no file beside the
// test.
module;
#include "tests.h"
#include "../src/keep.h"

export module pchat.test.keep;
import std;

namespace {

using pchat::keep::conversation_row;
using pchat::keep::message_row;

}  // namespace

SAID(keep, a_conversation_and_its_message_are_kept_and_read_back) {
  pchat::keep::close();  // one store per process: start this case from empty
  EXPECT_TRUE(pchat::keep::open());
  if (!pchat::keep::is_open())
    return;

  pchat::keep::keep_conversation(conversation_row{.id = "chat-1",
                                                 .title = "Kitchen",
                                                 .last_updated = 1700000000000});
  pchat::keep::keep_message(message_row{.id = "m-1",
                                       .conversation_id = "chat-1",
                                       .text = "There is bread.",
                                       .token_count = 4,
                                       .timestamp = 1700000001000});

  const std::vector<conversation_row> conversations = pchat::keep::conversations();
  EXPECT_TRUE(conversations.size() == 1);
  if (conversations.size() != 1)
    return;
  EXPECT_EQ(conversations[0].id, std::string("chat-1"));
  EXPECT_EQ(conversations[0].title, std::string("Kitchen"));

  const std::vector<message_row> messages = pchat::keep::messages_of("chat-1");
  EXPECT_TRUE(messages.size() == 1);
  if (messages.size() != 1)
    return;
  EXPECT_EQ(messages[0].id, std::string("m-1"));
  EXPECT_EQ(messages[0].text, std::string("There is bread."));
  EXPECT_EQ(messages[0].token_count, 4);

  // One message held out whole, by the id it was kept under -- the read an
  // answer's line makes when the program starts again. Nothing where the id
  // was not kept.
  const pchat::keep::kept_of_row whole = pchat::keep::kept_of("m-1");
  EXPECT_TRUE(whole.found);
  EXPECT_EQ(whole.conversation_id, std::string("chat-1"));
  EXPECT_EQ(whole.text, std::string("There is bread."));
  const pchat::keep::kept_of_row absent = pchat::keep::kept_of("no-such");
  EXPECT_FALSE(absent.found);

  pchat::keep::close();
}

// What the page's settings are kept as: one text written, read back whole --
// the pair the program asks of at the next run (its own table beside the
// ported schema's).
SAID(keep, settings_said_are_read_back_whole) {
  pchat::keep::close();
  EXPECT_TRUE(pchat::keep::open());
  EXPECT_EQ(pchat::keep::settings(), std::string(""));
  pchat::keep::keep_settings(R"({"asking":{"thinking":false}})");
  EXPECT_EQ(pchat::keep::settings(), std::string(R"({"asking":{"thinking":false}})"));
  pchat::keep::keep_settings(R"({"asking":{"thinking":true}})");
  EXPECT_EQ(pchat::keep::settings(), std::string(R"({"asking":{"thinking":true}})"));
  pchat::keep::close();
}

// A store at a file, as the program holds it: what was kept in one run is
// read back in the next. The file is under the session's own temp -- no path
// beside the test is assumed, and nothing is left standing after it.
SAID(keep, a_store_at_a_file_holds_what_a_last_run_kept) {
  const std::string where = "/tmp/opencode/pchat-keep-test.sqlite";
  std::filesystem::remove(where);

  pchat::keep::close();
  EXPECT_TRUE(pchat::keep::open_at(where));
  pchat::keep::keep_conversation(conversation_row{.id = "chat-2",
                                                 .title = "Cellar",
                                                 .last_updated = 1700000002000});
  pchat::keep::keep_message(message_row{.id = "m-2",
                                       .conversation_id = "chat-2",
                                       .text = "Jars are on the shelf.",
                                       .token_count = 5,
                                       .timestamp = 1700000003000});
  pchat::keep::close();

  // The next run: the same file, no writing.
  EXPECT_TRUE(pchat::keep::open_at(where));
  const std::vector<conversation_row> conversations = pchat::keep::conversations();
  EXPECT_TRUE(conversations.size() == 1);
  if (conversations.size() == 1) {
    EXPECT_EQ(conversations[0].id, std::string("chat-2"));
    EXPECT_EQ(conversations[0].title, std::string("Cellar"));
  }
  const pchat::keep::kept_of_row whole = pchat::keep::kept_of("m-2");
  EXPECT_TRUE(whole.found);
  if (whole.found)
    EXPECT_EQ(whole.text, std::string("Jars are on the shelf."));
  EXPECT_EQ(pchat::keep::settings(), std::string(""));
  pchat::keep::keep_settings(R"({"looks":{"amoled":true}})");
  pchat::keep::close();

  // The settings of the earlier run, at the next open: asked of the same file.
  EXPECT_TRUE(pchat::keep::open_at(where));
  EXPECT_EQ(pchat::keep::settings(), std::string(R"({"looks":{"amoled":true}})"));
  pchat::keep::close();
  std::filesystem::remove(where);
}

int main() { return pchat::tests::run(); }
