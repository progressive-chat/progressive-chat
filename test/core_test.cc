module;
#include "tests.h"
export module pchat.test.core;
// SPDX-License-Identifier: AGPL-3.0-only
// What the model itself says: ids are comparable, and what the screens read is
// what was put in.
import std;
import pchat.core;



using namespace pchat;

SAID(core, ids_compare_by_value) {
  EXPECT_EQ((conversation_id{"general"}), (conversation_id{"general"}));
  EXPECT_NE((conversation_id{"general"}), (conversation_id{"random"}));
  EXPECT_LT((conversation_id{"alpha"}), (conversation_id{"beta"}));
}

SAID(core, root_keeps_what_it_was_given) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"a"}, .title = "Ann", .last_activity = 2});
  root.conversations.push_back(conversation{.id = conversation_id{"b"}, .title = "Bob", .last_activity = 1});
  root.messages.push_back(message{.id = message_id{"1"}, .chat = conversation_id{"a"}, .text = "hi"});
  root.opened = conversation_id{"a"};
  const chats_root same = root;
  EXPECT_TRUE(same == root);
  EXPECT_EQ(root.opened->value, "a");
  EXPECT_EQ(root.messages.size(), 1u);
}

SAID(core, delivery_is_ordered_as_it_is_lived) {
  EXPECT_LT(delivery::sending, delivery::sent);
  EXPECT_LT(delivery::sent, delivery::seen);
}

SAID(core, the_wire_sees_a_compact_capsule_and_what_came_after_it) {
  const conversation_id chat{"kitchen"};
  const auto plain = [&chat](std::string_view id, std::string_view text) {
    return message{.id = message_id{std::string(id)},
                   .chat = chat,
                   .text = std::string(text)};
  };
  const std::vector<message> said{plain("1", "first"), plain("2", "second"),
                                  plain("3", "third")};
  // Nothing compacted: all of it goes, as it did before any capsule stood.
  EXPECT_EQ(wire_view(said).size(), 3u);

  // A capsule where compaction left it: what stood before it was summarised
  // into it, so the capsule and what was said after it are what is carried.
  const message capsule{.id = message_id{"c"},
                        .chat = chat,
                        .author = "Compact",
                        .text = "the sum of it",
                        .state = delivery::sent,
                        .capsule = true};
  const message after = plain("4", "after it");
  const std::vector<message> compacted{said[0], said[1], capsule, after};
  const std::vector<message> view = wire_view(compacted);
  EXPECT_EQ(view.size(), 2u);
  EXPECT_TRUE(view[0].capsule);
  EXPECT_EQ(view[0].text, std::string("the sum of it"));
  EXPECT_EQ(view[1].id, message_id{"4"});

  // Two capsules: the last one carries what even the first one held --
  // nothing of the stretch between them stands on the wire twice.
  const message second_capsule{.id = message_id{"c2"},
                               .chat = chat,
                               .author = "Compact",
                               .text = "the newer sum",
                               .state = delivery::sent,
                               .capsule = true};
  const std::vector<message> chained{said[0], capsule, after, second_capsule,
                                     plain("5", "and after the second")};
  const std::vector<message> chained_view = wire_view(chained);
  EXPECT_EQ(chained_view.size(), 2u);
  EXPECT_EQ(chained_view[0].text, std::string("the newer sum"));
  EXPECT_EQ(chained_view[1].id, message_id{"5"});

  // A capsule that failed to be written says nothing: it stands skipped,
  // and where no capsule was ever written whole, the chat goes as it always
  // did -- a compact that never landed is no compact.
  const message failed_capsule{.id = message_id{"cf"},
                               .chat = chat,
                               .author = "Compact",
                               .text = "half of it",
                               .state = delivery::failed,
                               .capsule = true};
  const std::vector<message> broken{said[0], said[1], failed_capsule};
  const std::vector<message> broken_view = wire_view(broken);
  EXPECT_EQ(broken_view.size(), 2u);
  EXPECT_FALSE(broken_view[0].capsule);
  const std::vector<message> after_failed{said[0], capsule, after,
                                          failed_capsule, plain("6", "later")};
  const std::vector<message> after_failed_view = wire_view(after_failed);
  EXPECT_EQ(after_failed_view.size(), 3u);
  EXPECT_TRUE(after_failed_view[0].capsule);
  EXPECT_EQ(after_failed_view[2].id, message_id{"6"});
}

SAID(core, a_title_is_the_first_line_that_says_anything) {
  // As a model says one: the first line that says anything, its own quotes
  // taken off, and nothing of the lines after it.
  EXPECT_EQ(title_from("The kitchen remodel\nand what it cost"),
            std::string("The kitchen remodel"));
  EXPECT_EQ(title_from("\n\n  Bread, then\n  more bread  \r\nsecond"),
            std::string("Bread, then"));
  EXPECT_EQ(title_from("\"A name in quotes\""), std::string("A name in quotes"));

  // Nothing usable: nothing at all, and an empty answer names nothing.
  EXPECT_EQ(title_from(""), std::string{});
  EXPECT_EQ(title_from("  \n \t "), std::string{});

  // Too wide for a row: cut at a word, and on a whole character -- no half
  // of one left standing at the end.
  const std::string long_name =
      title_from("A very long name for a conversation that keeps going and "
                 "going well past where any row would show it at all");
  EXPECT_TRUE(long_name.size() <= 48u);
  EXPECT_TRUE(long_name.find(' ') != std::string::npos);
  EXPECT_EQ(long_name, std::string("A very long name for a conversation that keeps"));

  // I imya novoj besedy — imya, a ne sluchajnaya stroka.
  EXPECT_EQ(std::string(new_chat_title), std::string("New chat"));
}

SAID(core, an_automatic_compact_begins_where_the_budget_is_almost_spent) {
  // The point Agora's automatic compact starts at: 90% of the window, and
  // no lower -- a chat under it asks as it always asked.
  EXPECT_FALSE(overruns(1'000'000u, 0.0f));
  EXPECT_FALSE(overruns(1'000'000u, 0.899f));
  EXPECT_TRUE(overruns(1'000'000u, 0.9f));
  EXPECT_TRUE(overruns(1'000'000u, 1.0f));
  // A chat nobody gave a window overflows nothing: what fits is fit as it
  // always was, and a zero share of no room at all is not a full one.
  EXPECT_FALSE(overruns(0u, 0.95f));
  EXPECT_FALSE(overruns(0u, 9.0f));
}

int main() { return pchat::tests::run(); }
