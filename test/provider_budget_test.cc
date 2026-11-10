module;
#include "tests.h"
export module pchat.test.provider_budget;
// SPDX-License-Identifier: AGPL-3.0-only
// What of a conversation is asked about: the window an answerer has decides
// it, with what the model is told and the tools it may ask for taken off that
// window first. A conversation that fits is asked about whole; one that does
// not starts at the oldest message that still fits, and begins on the
// reader's own word, which is where every wire is happy to be started; the
// newest message is asked about even where it alone is wider than the window,
// because a question is asked with something in it.
//
// Nothing of any of it reaches the conversation: the messages stay where they
// are, whole, and are drawn again as they were.
import std;
import pchat.core;
import pchat.provider;
import pchat.provider.budget;
import pchat.tokens;

using namespace pchat;

namespace {

const conversation_id the_chat{"budget"};

// What one message of the conversation says: a line a reader or a model said,
// in the order the messages are given -- the reader first, as a chat starts.
[[nodiscard]] message line_of(const std::size_t at, const bool outgoing,
                              std::string words) {
  return message{.id = message_id{the_chat.value + "-" + std::to_string(at)},
                 .chat = the_chat,
                 .author = outgoing ? "reader" : "model",
                 .outgoing = outgoing,
                 .text = std::move(words)};
}

// Enough words to be worth a count: what a message is measured against when
// the window is small, and what every message of a long chat is made of here.
[[nodiscard]] std::string many_words(const std::size_t how_many,
                                     const std::size_t which) {
  std::string out;
  for (std::size_t one = 0; one < how_many; ++one) {
    out += "the streets are full of people and the weather is warm, number ";
    out += std::to_string(which);
    out += " piece ";
    out += std::to_string(one);
    out += "; ";
  }
  return out;
}

// A conversation of `how_many` messages, each said by the reader where it
// comes at an even place and by the model where it comes at an odd one.
[[nodiscard]] std::vector<message> conversation_of(const std::size_t how_many,
                                                   const std::size_t words) {
  std::vector<message> out;
  out.reserve(how_many);
  for (std::size_t at = 0; at < how_many; ++at)
    out.push_back(line_of(at, at % 2 == 0, many_words(words, at)));
  return out;
}

[[nodiscard]] provider::endpoint an_endpoint(std::string told = "You are brief.") {
  return provider::endpoint{
      .base = "https://api.openai.com/v1", .model = "sonnet-4",
      .told = std::move(told)};
}

// Where an id is in a conversation, or one past the end of it where it is in
// nothing at all.
[[nodiscard]] std::size_t place_of(const std::vector<message> &said,
                                   const message_id &one) {
  for (std::size_t at = 0; at < said.size(); ++at)
    if (said[at].id == one)
      return at;
  return said.size();
}

}  // namespace

SAID(provider_budget, a_conversation_that_fits_is_asked_about_whole) {
  tokens::vocabulary(tokens::o200k());
  const std::vector<message> said = conversation_of(12, 8);
  const message_id from =
      provider::fit_from(said, an_endpoint(), 1'048'576, provider::wanted{});
  EXPECT_EQ(from, said.front().id);
}

SAID(provider_budget, one_that_does_not_fit_starts_where_it_still_fits) {
  tokens::vocabulary(tokens::o200k());
  const std::vector<message> said = conversation_of(60, 40);
  // The smallest window there is, and a conversation far wider than it: what
  // is asked about is a run of the newest messages, and it starts on the
  // reader's own word -- never on an answer, which is what a wire reads as a
  // question with nothing asked in it.
  const message_id from = provider::fit_from(said, an_endpoint(), 4'096,
                                             provider::wanted{});
  EXPECT_NE(from, said.front().id);
  const std::size_t at = place_of(said, from);
  EXPECT_LT(at, said.size());
  EXPECT_TRUE(said[at].outgoing);
}

SAID(provider_budget, what_the_model_is_told_comes_off_the_window_first) {
  tokens::vocabulary(tokens::o200k());
  const std::vector<message> said = conversation_of(60, 40);
  const std::size_t brief =
      place_of(said, provider::fit_from(said, an_endpoint("You are brief."),
                                        4'096, provider::wanted{}));
  const std::size_t told =
      place_of(said, provider::fit_from(
                         said, an_endpoint(many_words(1'500, 0)), 4'096,
                         provider::wanted{}));
  // Said once and said at length, with the same conversation and the same
  // window: what the model is told stands ahead of the messages, so the more
  // of it there is, the less of the conversation fits behind it.
  EXPECT_LT(brief, told);
}

SAID(provider_budget, the_tools_it_may_ask_for_come_off_it_too) {
  tokens::vocabulary(tokens::o200k());
  const std::vector<message> said = conversation_of(60, 40);
  const std::size_t bare =
      place_of(said, provider::fit_from(said, an_endpoint(), 4'096,
                                        provider::wanted{}));
  provider::wanted with_tools{};
  with_tools.tools.push_back(provider::offered{
      .name = "look_up_in_this_conversation",
      .told = "Read back what was said in this very conversation.",
      .taken = {{"what", "the words to look for, as one string"}}});
  const std::size_t offered =
      place_of(said,
               provider::fit_from(said, an_endpoint(), 4'096, with_tools));
  EXPECT_GE(offered, bare);
}

SAID(provider_budget, the_newest_is_asked_about_even_where_it_is_too_wide) {
  tokens::vocabulary(tokens::o200k());
  std::vector<message> said;
  said.push_back(line_of(0, true, "Hello"));
  said.push_back(line_of(1, false, "Good morning"));
  said.push_back(line_of(2, true, many_words(400, 2)));
  // The newest message alone is wider than the smallest window there is, and
  // it is the one that is asked about anyway: Agora keeps a legal reader's
  // word and the newest complete message even where that one alone does not
  // fit, and this is that, in the conversation's own words.
  const message_id from = provider::fit_from(said, an_endpoint(), 4'096,
                                             provider::wanted{});
  EXPECT_EQ(from, said.back().id);
}

SAID(provider_budget, where_nobody_said_how_much_room_nothing_is_left_out) {
  tokens::vocabulary(tokens::o200k());
  const std::vector<message> said = conversation_of(60, 40);
  // A chat of a model this program knows no window of: nobody said how much
  // room there is, so nothing is decided about it, and the whole of it goes
  // as it did before any of this stood here.
  const message_id from = provider::fit_from(said, an_endpoint(), 0,
                                             provider::wanted{});
  EXPECT_EQ(from, said.front().id);
  // And where there is nothing said at all, there is nothing to ask about.
  const std::vector<message> none;
  EXPECT_EQ(provider::fit_from(none, an_endpoint(), 4'096, provider::wanted{}),
            message_id{});
}

SAID(provider_budget, where_not_one_word_is_the_readers_the_newest_is_asked_for) {
  tokens::vocabulary(tokens::o200k());
  // A conversation of answers only, which no wire is started with from the
  // top: the newest is what is asked about, because asking with nothing at
  // all is not an asking.
  std::vector<message> said;
  for (std::size_t at = 0; at < 4; ++at)
    said.push_back(line_of(at, false, many_words(4, at)));
  const message_id from = provider::fit_from(said, an_endpoint(), 4'096,
                                             provider::wanted{});
  EXPECT_EQ(from, said.back().id);
}

// What a round of asking costs the window: it goes back onto the wire with
// the answer that carried it (turns_of), so what it asked for and what came
// back are counted here too -- against the same window, in the words a wire
// is given them in.
SAID(provider_budget, what_a_round_asked_and_answered_counts_against_the_window) {
  tokens::vocabulary(tokens::o200k());
  std::vector<message> said;
  said.push_back(line_of(0, true, many_words(4, 0)));
  said.push_back(line_of(1, false, many_words(4, 1)));
  said.push_back(line_of(2, true, many_words(4, 2)));
  std::vector<message> with_round = said;
  with_round[1].before =
      thought{.said = "> looked\n",
              .calls = {call_made{.tool = "how_much_has_been_said",
                                  .asked = R"({"what":"butter"})",
                                  .answered = many_words(1'500, 9)}}};
  // Whole where nothing of a round was said, and not whole where the round's
  // own words are wider than the window: they are asked about now, so they
  // are counted now.
  EXPECT_EQ(provider::fit_from(said, an_endpoint(), 4'096, provider::wanted{}),
            said.front().id);
  EXPECT_NE(
      provider::fit_from(with_round, an_endpoint(), 4'096, provider::wanted{}),
      said.front().id);
}

// And only what a wire actually takes of a round: an asking written down as
// an object goes out with its words and counts against the window, while one
// written down in plain words goes out with no arguments at all -- so those
// words, which no wire will ever read, are not counted against anything.
SAID(provider_budget, only_what_a_wire_takes_of_a_round_is_counted) {
  tokens::vocabulary(tokens::o200k());
  const std::string words = many_words(1'500, 3);
  auto conversation_with = [&words](const std::string &asked) {
    std::vector<message> said;
    said.push_back(line_of(0, true, many_words(4, 0)));
    said.push_back(line_of(1, false, many_words(4, 1)));
    said.back().before = thought{
        .said = "> looked\n",
        .calls = {call_made{.tool = "look_up_in_this_conversation",
                            .asked = asked,
                            .answered = "two said it, both about the cold"}}};
    said.push_back(line_of(2, true, many_words(4, 2)));
    return said;
  };
  const std::vector<message> plain = conversation_with(words);
  const std::vector<message> object =
      conversation_with("{\"what\":\"" + words + "\"}");
  EXPECT_EQ(provider::fit_from(plain, an_endpoint(), 4'096, provider::wanted{}),
            plain.front().id);
  EXPECT_NE(provider::fit_from(object, an_endpoint(), 4'096, provider::wanted{}),
            object.front().id);
}

SAID(provider_budget, pictures_take_the_room_they_stand_in) {
  tokens::vocabulary(tokens::o200k());
  const std::vector<message> said = conversation_of(60, 40);
  const message_id bare =
      provider::fit_from(said, an_endpoint(), 4'096, provider::wanted{});
  // The same conversation with a picture on every message of it: what is
  // asked about cannot start any earlier than it did without them -- each
  // picture costs the room it stands in, as each does on Agora's side.
  std::vector<message> dressed = said;
  for (message &one : dressed)
    one.images.push_back(image_part{"image/png", "aGVsbG8="});
  const message_id with_pictures =
      provider::fit_from(dressed, an_endpoint(), 4'096, provider::wanted{});
  EXPECT_GE(place_of(dressed, with_pictures), place_of(said, bare));
}

int main() { return pchat::tests::run(); }
