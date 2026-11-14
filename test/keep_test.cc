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

  pchat::keep::close();
}

int main() { return pchat::tests::run(); }
