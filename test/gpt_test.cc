module;
#include "tests.h"
#include "../src/logic/gpt_import.h"
export module pchat.test.gpt;
// SPDX-License-Identifier: AGPL-3.0-only
// What a ChatGPT export is once read: the ported importer does the reading,
// and these say that what comes out of it stands in the words the bridge
// above promises -- a conversation whole, its messages in the order they
// were said, and a file that is not an export failing instead of pretending.
import std;

using namespace pchat::logic;

namespace {

// One export of the bare-array kind, shaped as the port's own golden
// scenario has it (tools/golden/gpt-chat-importer, 01-text-simple): a root
// node with no message, and the two that follow it in a line.
constexpr std::string_view one_export =
    R"([{"conversation_id": "c1", "title": "Weekend plans",
        "create_time": 1700000000.0, "update_time": 1700000300.5,
        "mapping": {
          "n0": {"id": "n0", "parent": "client-created-root"},
          "n1": {"id": "n1",
                 "message": {"id": "m1", "author": {"role": "user"},
                             "content": {"content_type": "text",
                                         "parts": ["is there bread"]},
                             "create_time": 1700000000.0},
                 "parent": "n0"},
          "n2": {"id": "n2",
                 "message": {"id": "m2", "author": {"role": "assistant"},
                             "content": {"content_type": "text",
                                         "parts": ["there is bread"]},
                             "create_time": 1700000120.0,
                             "metadata": {"model_slug": "gpt-4o"}},
                 "parent": "n1"}}}])";

}  // namespace

SAID(gpt, an_export_read_is_a_conversation_and_its_messages_whole) {
  const gpt_extracted read = gpt_extract(one_export);
  EXPECT_TRUE(read.ok);
  EXPECT_TRUE(read.failure.empty());
  EXPECT_EQ(read.conversations.size(), 1u);
  EXPECT_EQ(read.conversations[0].id, std::string("c1"));
  EXPECT_EQ(read.conversations[0].title, std::string("Weekend plans"));
  // Seconds in the file, milliseconds here: a moment is what this program
  // counts a moment in, and the importer already counted it that way.
  EXPECT_EQ(read.conversations[0].updated,
            std::int64_t{1700000300} * 1000 + 500);
  EXPECT_EQ(read.messages.size(), 2u);

  const gpt_message &asked_about = read.messages[0];
  EXPECT_EQ(asked_about.conversation, std::string("c1"));
  EXPECT_EQ(asked_about.id, std::string("m1"));
  EXPECT_EQ(asked_about.text, std::string("is there bread"));
  EXPECT_TRUE(asked_about.from_reader);
  EXPECT_EQ(asked_about.when, std::int64_t{1700000000} * 1000);

  const gpt_message &answered = read.messages[1];
  EXPECT_EQ(answered.id, std::string("m2"));
  EXPECT_EQ(answered.text, std::string("there is bread"));
  EXPECT_FALSE(answered.from_reader);
  EXPECT_EQ(answered.when, std::int64_t{1700000120} * 1000);
}

SAID(gpt, a_file_that_is_not_an_export_says_so_instead_of_pretending) {
  const gpt_extracted read = gpt_extract("this is not json at all");
  EXPECT_FALSE(read.ok);
  EXPECT_TRUE(read.conversations.empty());
  EXPECT_TRUE(read.messages.empty());
  EXPECT_FALSE(read.failure.empty());
}

int main() { return pchat::tests::run(); }
