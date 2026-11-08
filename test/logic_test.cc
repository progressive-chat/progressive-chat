module;
#include "tests.h"
export module pchat.test.logic;
// SPDX-License-Identifier: AGPL-3.0-only
// What the screens read out of the model: the drawer's order, the search over
// it, the badge, the field, and the marks in a line.
import std;
import pchat.core;
import pchat.logic.lists;
import pchat.logic.search;
import pchat.logic.composer;
import pchat.logic.formatting;
import pchat.logic.doc;



using namespace pchat;

namespace {
[[nodiscard]] std::vector<std::string> titles(std::span<const conversation* const> rows) {
  return rows | std::views::transform([](const conversation* row) { return row->title; })
       | std::ranges::to<std::vector>();
}
}  // namespace

SAID(logic, pinned_first_then_the_last_spoken_in) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"1"}, .title = "Quiet", .last_activity = 1});
  root.conversations.push_back(conversation{.id = conversation_id{"2"}, .title = "Busy", .last_activity = 9});
  root.conversations.push_back(conversation{.id = conversation_id{"3"}, .title = "Pinned", .last_activity = 2, .pinned = true});
  EXPECT_EQ(titles(logic::ordered(root)), (std::vector<std::string>{"Pinned", "Busy", "Quiet"}));
}

SAID(logic, a_search_matches_a_title_in_any_case) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"1"}, .title = "General"});
  root.conversations.push_back(conversation{.id = conversation_id{"2"}, .title = "Random"});
  EXPECT_EQ(logic::matching(root, "gen").size(), 1u);
  EXPECT_EQ(logic::matching(root, "GENERAL").size(), 1u);
  EXPECT_EQ(logic::matching(root, "").size(), 2u);
  EXPECT_EQ(logic::folded("MiXeD"), "mixed");
}

SAID(logic, the_badge_counts_conversations_and_messages) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"1"}, .title = "One", .unread = 2});
  root.conversations.push_back(conversation{.id = conversation_id{"2"}, .title = "Two", .unread = 3});
  root.conversations.push_back(conversation{.id = conversation_id{"3"}, .title = "Read"});
  const logic::unread_tally tally = logic::unread_of(root);
  EXPECT_EQ(tally.conversations, 2u);
  EXPECT_EQ(tally.messages, 5u);
}

SAID(logic, sending_makes_one_message_and_an_empty_field) {
  const logic::sent_message said =
      logic::send(logic::sending_request{conversation_id{"general"}, "hello", {}, std::nullopt}, "Reader", 7);
  EXPECT_EQ(said.said.text, "hello");
  EXPECT_EQ(said.said.chat.value, "general");
  EXPECT_TRUE(said.said.outgoing);
  EXPECT_TRUE(said.cleared.text.empty());
  EXPECT_FALSE(logic::can_send(logic::composer_state{.text = "  \n"}));
}

SAID(logic, marks_make_runs_of_style) {
  const std::string said = "**bold** and `code`";
  const std::vector<logic::run> runs = logic::runs_of(said);
  EXPECT_FALSE(runs.empty());
  const bool bold = std::ranges::any_of(runs, [](const logic::run& one) { return (one.style & logic::bold) != 0; });
  EXPECT_TRUE(bold);
}

SAID(logic, the_links_in_a_line_and_where_each_goes) {
  EXPECT_TRUE(logic::links_of("nothing of the sort here").empty());
  // A place said whole: where it stands in the line is where its own units
  // are, and nothing either side of it is taken into it.
  const std::string said = "read https://example.com/a?b=1 now";
  const std::vector<logic::place> one = logic::links_of(said);
  EXPECT_EQ(one.size(), 1u);
  EXPECT_EQ(one[0].first, 5u);
  EXPECT_EQ(std::string(said.substr(one[0].first, one[0].last - one[0].first)),
            one[0].target);
  EXPECT_EQ(one[0].target, std::string("https://example.com/a?b=1"));
  // The stop a sentence puts after a place is the sentence's, not its.
  EXPECT_EQ(logic::links_of("see https://example.com.")[0].target,
            std::string("https://example.com"));
  // What wraps one in writing is not part of it either.
  EXPECT_EQ(logic::links_of("(https://example.com)")[0].target,
            std::string("https://example.com"));
  // https:// read as itself, and not as http:// with an s left over.
  EXPECT_EQ(logic::links_of("https://example.com")[0].target,
            std::string("https://example.com"));
  // A place begun www. is https:// to the machine, said without either.
  EXPECT_EQ(logic::links_of("www.example.com works")[0].target,
            std::string("https://www.example.com"));
  // Two in one line, each in its own place in it.
  const std::vector<logic::place> two =
      logic::links_of("http://a.example and https://b.example/x");
  EXPECT_EQ(two.size(), 2u);
  EXPECT_EQ(two[0].target, std::string("http://a.example"));
  EXPECT_EQ(two[1].target, std::string("https://b.example/x"));
  // Nothing is said of a place begun and left without a name: "https://"
  // alone is two marks in a sentence, not somewhere to be taken.
  EXPECT_TRUE(logic::links_of("see https:// there").empty());
}

SAID(logic, what_the_marks_said_and_where_each_run_stands) {
  const logic::drawn drawn = logic::drawn_of("**bold** and `code`");
  // Said without a star or a backtick in it: the screen draws what they said
  // and none of them.
  EXPECT_EQ(drawn.said, std::string("bold and code"));
  EXPECT_EQ(drawn.runs.size(), 2u);
  // Each run stands where its own words stand in what is drawn -- which is
  // what the screen is told, and how it knows how much of the line to draw
  // in that style.
  EXPECT_EQ(std::string(drawn.said.substr(drawn.runs[0].first,
                                         drawn.runs[0].last - drawn.runs[0].first)),
            std::string("bold"));
  EXPECT_TRUE((drawn.runs[0].style & logic::bold) != 0);
  EXPECT_EQ(std::string(drawn.said.substr(drawn.runs[1].first,
                                         drawn.runs[1].last - drawn.runs[1].first)),
            std::string("code"));
  EXPECT_TRUE((drawn.runs[1].style & logic::code) != 0);
  // Inside code nothing is a mark but the backtick that ends it: what is
  // said in code is said as it stands, star and all.
  const logic::drawn in_code = logic::drawn_of("`p * p`");
  EXPECT_EQ(in_code.said, std::string("p * p"));
  EXPECT_EQ(in_code.runs.size(), 1u);
  EXPECT_TRUE((in_code.runs[0].style & logic::code) != 0);
  // A line with nothing said of it is itself, said in no run at all.
  const logic::drawn no_marks = logic::drawn_of("nothing of the sort");
  EXPECT_EQ(no_marks.said, std::string("nothing of the sort"));
  EXPECT_TRUE(no_marks.runs.empty());
}

SAID(logic, a_link_written_the_way_writing_writes_one) {
  const logic::drawn drawn =
      logic::drawn_of("read [this very thing](https://example.com/one) now");
  // Neither pair of stops is drawn, as no mark is: what is left is the line
  // as a reader reads it.
  EXPECT_EQ(drawn.said, std::string("read this very thing now"));
  EXPECT_EQ(drawn.places.size(), 1u);
  EXPECT_EQ(drawn.places[0].target, std::string("https://example.com/one"));
  // And where it stands is where the words it was written on stand now.
  EXPECT_EQ(std::string(drawn.said.substr(drawn.places[0].first,
                                         drawn.places[0].last - drawn.places[0].first)),
            std::string("this very thing"));
  // A stop with neither of its halves after it was no link at all.
  const logic::drawn no_link = logic::drawn_of("read [this very thing, said");
  EXPECT_TRUE(no_link.places.empty());
  EXPECT_EQ(no_link.said, std::string("read [this very thing, said"));
}

SAID(logic, a_line_said_in_quotes_and_how_deep_it_stands) {
  // What a model writes quoted is drawn quoted, and no stop of quote that
  // said so is drawn.
  const logic::drawn said = logic::drawn_of("> quoted once\n> > and again\nsaid plain\n");
  EXPECT_EQ(said.said, std::string("quoted once\nand again\nsaid plain\n"));
  EXPECT_EQ(said.runs.size(), std::size_t{2});
  if (said.runs.size() >= 2) {
    EXPECT_EQ(said.runs[0].first, std::size_t{0});
    EXPECT_EQ(said.runs[0].last, std::size_t{12});
    EXPECT_EQ(said.runs[0].quoted, 1);
    EXPECT_EQ(said.runs[1].first, std::size_t{12});
    EXPECT_EQ(said.runs[1].last, std::size_t{22});
    EXPECT_EQ(said.runs[1].quoted, 2);
  }
  // A stop of quote said in the middle of a line is said, and not read as
  // how deep anything stands.
  const logic::drawn kept = logic::drawn_of("2 > 1 said");
  EXPECT_EQ(kept.said, std::string("2 > 1 said"));
  EXPECT_TRUE(kept.runs.empty());
  // Marks inside a quoted line are still the marks they are.
  const logic::drawn both = logic::drawn_of("> **in quotes** here\n");
  EXPECT_EQ(both.said, std::string("in quotes here\n"));
  EXPECT_EQ(both.runs.size(), std::size_t{2});
  if (both.runs.size() >= 2) {
    EXPECT_TRUE((both.runs[0].style & logic::bold) != 0);
    EXPECT_EQ(both.runs[0].quoted, 1);
    EXPECT_EQ(both.runs[1].quoted, 1);
  }
}

SAID(logic, a_stretch_struck_out_says_what_it_said) {
  const logic::drawn said = logic::drawn_of("~~gone~~ said");
  EXPECT_EQ(said.said, std::string("gone said"));
  EXPECT_EQ(said.runs.size(), std::size_t{1});
  if (!said.runs.empty()) {
    EXPECT_EQ(said.runs[0].first, std::size_t{0});
    EXPECT_EQ(said.runs[0].last, std::size_t{4});
    EXPECT_TRUE((said.runs[0].style & logic::struck) != 0);
  }
  // One stop of strike alone is said, as one star alone is.
  const logic::drawn one = logic::drawn_of("a ~ b");
  EXPECT_EQ(one.said, std::string("a ~ b"));
  EXPECT_TRUE(one.runs.empty());
}

SAID(logic, a_stretch_said_underlined_and_one_said_slanted) {
  // Two stops of underscore are a line under it, and one is slanted: what
  // the longer of the two opens is read first, as `**` is before `*`.
  const logic::drawn said = logic::drawn_of("__under__ and _over_ said");
  EXPECT_EQ(said.said, std::string("under and over said"));
  EXPECT_EQ(said.runs.size(), std::size_t{2});
  if (said.runs.size() == 2) {
    EXPECT_TRUE((said.runs[0].style & logic::underline) != 0);
    EXPECT_TRUE((said.runs[0].style & logic::italic) == 0);
    EXPECT_TRUE((said.runs[1].style & logic::italic) != 0);
    EXPECT_TRUE((said.runs[1].style & logic::underline) == 0);
  }
}

SAID(logic, three_backticks_say_a_block_and_nothing_of_themselves) {
  // What stands between two fences of them is said as it stands, and what a
  // mark in it said is not read -- a star in code is a star. Neither fence,
  // nor the language written on the opening one, is said.
  const logic::drawn said =
      logic::drawn_of("before\n```cpp\nint a = 1 * 2;\n```\nafter");
  EXPECT_EQ(said.said, std::string("before\nint a = 1 * 2;\nafter"));
  const auto block = std::ranges::find_if(said.runs, [](const logic::run_place &one) {
    return (one.style & logic::block) != 0;
  });
  EXPECT_TRUE(block != said.runs.end());
  if (block != said.runs.end()) {
    EXPECT_TRUE((block->style & logic::code) != 0);
    EXPECT_TRUE((block->style & logic::italic) == 0);
    EXPECT_TRUE((block->style & logic::bold) == 0);
  }
  EXPECT_EQ(said.said.find('`'), std::string::npos);
}

int main() { return pchat::tests::run(); }


SAID(doc, hashes_counted_and_a_list_told_from_words) {
  const std::vector<logic::doc_line> read = logic::read_doc(
      "# The design of a chat\n"
      "\n"
      "How the screen looks.\n"
      "## The room\n"
      "One conversation fills it.\n"
      "### What is not drawn yet\n"
      "* a pill\n"
      "- another\n"
      "#### As small as three\n");
  EXPECT_EQ(read.size(), 10u);
  EXPECT_TRUE(read[0].kind == logic::doc_kind::title);
  EXPECT_EQ(read[0].said, std::string("The design of a chat"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::gap);
  EXPECT_TRUE(read[2].kind == logic::doc_kind::words);
  EXPECT_EQ(read[2].said, std::string("How the screen looks."));
  EXPECT_TRUE(read[3].kind == logic::doc_kind::heading);
  EXPECT_EQ(read[3].said, std::string("The room"));
  EXPECT_TRUE(read[5].kind == logic::doc_kind::subheading);
  EXPECT_EQ(read[5].said, std::string("What is not drawn yet"));
  EXPECT_TRUE(read[6].kind == logic::doc_kind::bullet);
  EXPECT_EQ(read[6].said, std::string("a pill"));
  EXPECT_TRUE(read[7].kind == logic::doc_kind::bullet);
  EXPECT_EQ(read[7].said, std::string("another"));
  // More hashes than three are said as small as three was.
  EXPECT_TRUE(read[9].kind == logic::doc_kind::subheading);
}

SAID(doc, a_last_line_with_no_break_after_it) {
  const std::vector<logic::doc_line> read = logic::read_doc("one\ntwo");
  EXPECT_EQ(read.size(), 2u);
  EXPECT_EQ(read[1].said, std::string("two"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::words);
}

SAID(doc, a_mark_alone_in_front_is_words) {
  const std::vector<logic::doc_line> read = logic::read_doc("*\n-# not a heading");
  EXPECT_TRUE(read[0].kind == logic::doc_kind::words);
  EXPECT_EQ(read[0].said, std::string("*"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::words);
  EXPECT_EQ(read[1].said, std::string("-# not a heading"));
}

SAID(doc, the_design_this_build_was_made_with_is_read_whole) {
  const std::string_view design = logic::the_design();
  EXPECT_LT(1000u, design.size());
  EXPECT_TRUE(design.find("The design of a chat") != std::string_view::npos);
  const std::vector<logic::doc_line> read = logic::read_doc(design);
  EXPECT_LT(100u, read.size());
  bool titled = false;
  bool listed = false;
  for (const logic::doc_line &one : read) {
    titled = titled || one.kind == logic::doc_kind::title;
    listed = listed || one.kind == logic::doc_kind::bullet;
  }
  EXPECT_TRUE(titled);
  EXPECT_TRUE(listed);
}
