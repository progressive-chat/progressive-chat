module;
#include "tests.h"
export module pchat.test.provider;
// SPDX-License-Identifier: AGPL-3.0-only
// What one conversation asks of a model, and what one answer coming back says:
// the body written for a wire, and the stream read out of it, whole and cut in
// two the way a transport hands it over.
import std;
import pchat.core;
import pchat.config;
import pchat.json;
import pchat.provider;
import pchat.provider.stream;

using namespace pchat;

namespace {

// A stream such as a chat wire writes one: text in two pieces, what it thought
// before it spoke, two tool calls by their index, the cost, and the frame that
// says it stopped for those calls.
constexpr std::string_view one_stream =
    "data: {\"choices\":[{\"delta\":{\"content\":\"Good \"},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"first the weather\\n\"},"
    "\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"content\":\"morning\"},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_1\","
    "\"function\":{\"name\":\"weather\",\"arguments\":\"{\\\"city\\\":\\\"Oslo\\\"}\"}}]},"
    "\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":1,\"id\":\"call_2\","
    "\"function\":{\"name\":\"mail\",\"arguments\":\"{}\"}}]},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"tool_calls\"}],\"usage\":"
    "{\"prompt_tokens\":12,\"completion_tokens\":7}}\n"
    "\n"
    "data: [DONE]\n"
    "\n";

[[nodiscard]] std::string said(const provider::stream &one) { return std::string(one.text()); }

}  // namespace

SAID(provider, a_body_is_written_for_a_wire_and_read_back_whole) {
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini",
                              .told = "You are brief."};
  EXPECT_EQ(provider::wire_of(at), provider::wire::chat);
  EXPECT_EQ(provider::url_of(at),
            std::string("https://api.openai.com/v1/chat/completions"));
  std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Hello"},
      provider::turn{.who = provider::role::assistant, .text = "Hi"},
      provider::turn{.who = provider::role::user, .text = "Say \"go\"\n"}};
  const std::string body = provider::body_of(at, turns);
  const auto read_back = json::read(body);
  EXPECT_TRUE(read_back.has_value());
  EXPECT_EQ(std::string(json::text_of(*json::said(*read_back, "model")).value_or("")),
            std::string("o4-mini"));
  EXPECT_TRUE(json::truth_of(*json::said(*read_back, "stream")).value_or(false));
  const json::value *said = json::said(json::said(*read_back, "messages"), 0);
  EXPECT_EQ(std::string(json::text_of(*json::said(said, "content")).value_or("")),
            std::string("You are brief."));
  EXPECT_EQ(std::string(json::text_of(*json::said(
                              json::said(json::said(*read_back, "messages"), 3), "content"))
                            .value_or("")),
            std::string("Say \"go\"\n"));
  EXPECT_TRUE(json::truth_of(*json::said(json::said(*read_back, "stream_options"),
                                         "include_usage"))
                  .value_or(false));
}

SAID(provider, how_the_program_asks_is_said_in_the_body) {
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  asking how{.temperature = 0.35,
             .top_p = 0.8,
             .max_tokens = 1024,
             .presence = -0.5,
             .often = 0.25};
  const std::string body =
      provider::body_of(at, turns, provider::wants_of(how));
  const auto read_back = json::read(body);
  EXPECT_TRUE(read_back.has_value());
  EXPECT_EQ(json::number_of(*json::said(*read_back, "temperature")).value_or(0.0),
            0.35);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "top_p")).value_or(0.0), 0.8);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "max_tokens")).value_or(0.0),
            1024.0);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "presence_penalty")).value_or(0.0),
            -0.5);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "frequency_penalty")).value_or(0.0),
            0.25);
}

SAID(provider, a_body_asked_for_nothing_about_choosing_says_nothing_of_it) {
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  const std::string body = provider::body_of(at, turns);
  const auto read_back = json::read(body);
  EXPECT_TRUE(read_back.has_value());
  EXPECT_FALSE(json::said(*read_back, "temperature"));
  EXPECT_FALSE(json::said(*read_back, "max_tokens"));
}

SAID(provider, a_second_wire_is_known_by_how_its_base_ends) {
  provider::endpoint one{.base = "https://generativelanguage.googleapis.com/v1beta"};
  EXPECT_EQ(provider::wire_of(one), provider::wire::responses);
  EXPECT_EQ(provider::url_of(one),
            std::string("https://generativelanguage.googleapis.com/v1beta/responses"));
}

SAID(provider, what_the_model_keeps_is_turns_on_a_wire) {
  chats_root root;
  root.messages.push_back(message{.id = message_id{"1"},
                                  .text = "Hello",
                                  .outgoing = true});
  root.messages.push_back(message{.id = message_id{"2"},
                                  .text = "Hi",
                                  .before = thought{.took_ms = 100, .said = "> looked\n"}});
  const std::vector<provider::turn> turns = provider::turns_of(root.messages);
  EXPECT_EQ(turns.size(), 2u);
  EXPECT_EQ(turns[0].who, provider::role::user);
  EXPECT_EQ(turns[1].who, provider::role::assistant);
  EXPECT_EQ(turns[1].thought, std::string("> looked\n"));
}

SAID(provider, a_stream_is_read_whole) {
  provider::stream one;
  one.feed(one_stream);
  EXPECT_EQ(said(one), std::string("Good morning"));
  EXPECT_EQ(std::string(one.thought()),
            std::string("first the weather\n> weather({\"city\":\"Oslo\"})\n> mail({})\n"));
  EXPECT_EQ(one.calls().size(), 2u);
  EXPECT_EQ(one.calls()[0].name, std::string("weather"));
  EXPECT_EQ(one.calls()[0].arguments, std::string("{\"city\":\"Oslo\"}"));
  EXPECT_EQ(one.calls()[1].id, std::string("call_2"));
  EXPECT_EQ(one.cost().in, 12u);
  EXPECT_EQ(one.cost().out, 7u);
  EXPECT_TRUE(one.ended());
  EXPECT_FALSE(one.failed());
  EXPECT_EQ(std::string(one.finish()), std::string("tool_calls"));
}

SAID(provider, a_stream_cut_anywhere_says_the_same) {
  provider::stream one;
  // Fed in the sizes a transport hands over: three characters at a time.
  for (std::size_t at = 0; at < one_stream.size(); at += 3)
    one.feed(one_stream.substr(at, 3));
  EXPECT_EQ(said(one), std::string("Good morning"));
  EXPECT_EQ(one.calls().size(), 2u);
  EXPECT_EQ(one.cost().out, 7u);
  EXPECT_TRUE(one.ended());
  EXPECT_FALSE(one.failed());
}

SAID(provider, what_the_stream_said_frame_by_frame) {
  provider::stream one;
  one.feed(one_stream);
  const std::vector<provider::event> said = one.take();
  std::size_t texts = 0;
  std::size_t tools = 0;
  std::size_t ended = 0;
  for (const provider::event &one : said) {
    texts += one.what == provider::event::kind::text ? 1u : 0u;
    tools += one.what == provider::event::kind::tool ? 1u : 0u;
    ended += one.what == provider::event::kind::ended ? 1u : 0u;
  }
  EXPECT_EQ(texts, 2u);
  EXPECT_EQ(tools, 2u);
  EXPECT_EQ(ended, 1u);
  // What it thought and what it cost are said too, and taken only once.
  EXPECT_TRUE(one.take().empty());
}

SAID(provider, a_frame_that_is_not_written_in_json_fails_the_stream) {
  provider::stream one;
  one.feed("data: {not json\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_FALSE(one.why().empty());
  EXPECT_FALSE(one.ended());
}

SAID(provider, an_error_the_wire_itself_says_fails_the_stream) {
  provider::stream one;
  one.feed("data: {\"error\":{\"message\":\"the key was refused\"}}\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_EQ(std::string(one.why()),
            std::string("the wire's own error: the key was refused"));
}

SAID(provider, done_without_a_reason_stops_because_it_said_stop) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hi\"}}]}\n\ndata: [DONE]\n\n");
  EXPECT_EQ(said(one), std::string("Hi"));
  EXPECT_TRUE(one.ended());
  EXPECT_EQ(std::string(one.finish()), std::string("stop"));
}

// Thinking a wire carries in no field of its own but inside the answer,
// wrapped in a tag: kept apart from what is said, and apart wherever the tag
// itself is cut in two by the frame that carried it.
SAID(provider, thinking_written_inside_the_answer_is_not_the_answer) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"<think>let me see</think>Hi\"}}]}\n\n");
  EXPECT_EQ(said(one), std::string("Hi"));
  EXPECT_EQ(std::string(one.thought()), std::string("let me see"));
}

SAID(provider, a_tag_cut_in_two_by_a_frame_is_whole_when_it_is_read) {
  provider::stream one;
  // The tag arrives in halves, each half at the edge of a frame, and the
  // thought inside it is said before its tag is shut.
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"the <thi\"}}]}\n\ndata: "
           "{\"choices\":[{\"delta\":{\"content\":\"nk>thou\"}}]}\n\n");
  EXPECT_EQ(said(one), std::string("the "));
  EXPECT_EQ(std::string(one.thought()), std::string("thou"));
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"ght</thi\"}}]}\n\n"
           "data: {\"choices\":[{\"delta\":{\"content\":\"nk>Hi\"}}]}\n\n");
  EXPECT_EQ(std::string(one.thought()), std::string("thought"));
  EXPECT_EQ(said(one), std::string("the Hi"));
}

SAID(provider, thinking_that_never_opened_a_tag_is_thinking_until_it_closes_one) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"hmm...\"}}]}\n\n"
           "data: {\"choices\":[{\"delta\":{\"content\":\"</think>Hi\"}}]}\n\n");
  EXPECT_EQ(said(one), std::string("Hi"));
  EXPECT_EQ(std::string(one.thought()), std::string("hmm..."));
}

SAID(provider, thought_left_open_when_the_stream_ends_is_thought) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"<think>let me see\"}}]}\n\n"
           "data: [DONE]\n\n");
  EXPECT_TRUE(said(one).empty());
  EXPECT_EQ(std::string(one.thought()), std::string("let me see"));
}

// A tool asked for in the answer's own words, which is how every model that
// was given no fields for them asks: cut out of the answer, and said as the
// call it is.
SAID(provider, a_tool_written_in_the_answer_is_a_call_and_not_text) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"<tool_call>{\\\"name\\\":"
           "\\\"weather\\\",\\\"arguments\\\":{\\\"city\\\":\\\"Oslo\\\"}}"
           "</tool_call>\"}}]}\n\n"
           "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n");
  EXPECT_TRUE(said(one).empty());
  EXPECT_EQ(one.calls().size(), std::size_t(1));
  EXPECT_EQ(one.calls().front().name, std::string("weather"));
  EXPECT_EQ(one.calls().front().arguments, std::string("{\"city\":\"Oslo\"}"));
}

// Thinking, asked for in the words each family takes it in: openai's own
// reasoning effort, the relay's thing inside the body, and deepseek, which is
// never asked because it thinks whenever it answers.
SAID(provider, thinking_is_asked_for_as_each_family_names_it) {
  const std::vector<provider::turn> no_turns;
  const provider::wanted most{.thinking = true, .how_hard = effort::most};
  const provider::endpoint openai{.base = "https://api.openai.com/v1", .model = "o4-mini"};
  const provider::endpoint relay{.base = "https://openrouter.ai/api/v1", .model = "o4-mini"};
  const provider::endpoint deep{.base = "https://api.deepseek.com/v1", .model = "deepseek-r1"};
  EXPECT_TRUE(provider::body_of(openai, no_turns, most).find("\"reasoning_effort\":\"high\"") !=
              std::string::npos);
  EXPECT_TRUE(provider::body_of(relay, no_turns, most).find(
                  "\"reasoning\":{\"effort\":\"high\"}") != std::string::npos);
  EXPECT_TRUE(provider::body_of(deep, no_turns, most).find("reasoning") == std::string::npos);
}

SAID(provider, thinking_turned_off_is_asked_for_nowhere) {
  const std::vector<provider::turn> no_turns;
  const provider::wanted none{.thinking = false};
  const provider::endpoint openai{.base = "https://api.openai.com/v1", .model = "o4-mini"};
  EXPECT_TRUE(provider::body_of(openai, no_turns, none).find("reasoning") == std::string::npos);
}

namespace {

// What stands under one name in a body, or nothing where the body says
// nothing of it -- so that a body missing a field is a check that did not
// hold and not a read past the end of one.
[[nodiscard]] const json::value *in(const json::value &body, std::string_view name) {
  return json::said(&body, name);
}

// The number under one name, or -1 where there is none.
[[nodiscard]] double number_in(const json::value &body, std::string_view name) {
  const json::value *one = in(body, name);
  if (one == nullptr)
    return -1.0;
  return json::number_of(*one).value_or(-1.0);
}

// The word under one name, or nothing where there is none.
[[nodiscard]] std::string text_in(const json::value &body, std::string_view name) {
  const json::value *one = in(body, name);
  if (one == nullptr)
    return std::string{};
  return std::string(json::text_of(*one).value_or(""));
}

}  // namespace

// What a thinking costs the body it is asked for in: a budget in the words
// this family takes it in, room for an answer beside that budget -- and the
// two numbers that choose the words taken off it, because this family does
// not read them beside a thinking at all.
SAID(provider, thinking_on_anthropic_is_a_budget_and_room_to_answer) {
  const provider::endpoint at{.base = "https://api.anthropic.com/v1",
                              .model = "haiku-4"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  const asking how{.thinking = true, .temperature = 0.7, .top_p = 1.0,
                   .max_tokens = 2048};
  const auto body = json::read(provider::body_of(at, turns, provider::wants_of(how)));
  EXPECT_TRUE(body.has_value());
  if (!body.has_value())
    return;
  const json::value *thinking = in(*body, "thinking");
  EXPECT_TRUE(thinking != nullptr);
  if (thinking == nullptr)
    return;
  EXPECT_EQ(text_in(*thinking, "type"), std::string("enabled"));
  const double budget = number_in(*thinking, "budget_tokens");
  EXPECT_GE(budget, 1024.0);
  // Anthropic reads the budget and the answer out of the same max_tokens, so
  // the room asked for is the budget plus what is left to answer in with it.
  EXPECT_LT(budget, number_in(*body, "max_tokens"));
  EXPECT_TRUE(in(*body, "temperature") == nullptr);
  EXPECT_EQ(number_in(*body, "top_p"), 1.0);
}

// And the same body where the reader asked for no thinking: nothing said of
// it, and the two numbers back where they were.
SAID(provider, thinking_off_anthropic_leaves_the_numbers_where_they_were) {
  const provider::endpoint at{.base = "https://api.anthropic.com/v1",
                              .model = "haiku-4"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  const asking how{.thinking = false, .temperature = 0.7, .top_p = 0.8,
                   .max_tokens = 2048};
  const auto body = json::read(provider::body_of(at, turns, provider::wants_of(how)));
  EXPECT_TRUE(body.has_value());
  if (!body.has_value())
    return;
  EXPECT_TRUE(in(*body, "thinking") == nullptr);
  EXPECT_EQ(number_in(*body, "temperature"), 0.7);
  EXPECT_EQ(number_in(*body, "top_p"), 0.8);
  EXPECT_EQ(number_in(*body, "max_tokens"), 2048.0);
}

// A top_p beside a thinking, kept only where this family still allows one:
// 0.95 and above go out with it, and anything under that is left out rather
// than sent to be refused.
SAID(provider, a_top_p_beside_a_thinking_is_kept_only_in_095_and_up) {
  const provider::endpoint at{.base = "https://api.anthropic.com/v1",
                              .model = "haiku-4"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  const asking borderline{.thinking = true, .top_p = 0.95};
  const auto kept =
      json::read(provider::body_of(at, turns, provider::wants_of(borderline)));
  EXPECT_TRUE(kept.has_value());
  if (kept.has_value())
    EXPECT_EQ(number_in(*kept, "top_p"), 0.95);
  const asking under{.thinking = true, .top_p = 0.7};
  const auto gone =
      json::read(provider::body_of(at, turns, provider::wants_of(under)));
  EXPECT_TRUE(gone.has_value());
  if (gone.has_value())
    EXPECT_TRUE(in(*gone, "top_p") == nullptr);
}

// And a model that never thought -- a generation from before extended
// thinking, which the capability says so about -- is given no thinking
// parameter at all, however much the reader asked for one.
SAID(provider, a_model_that_never_thought_is_asked_for_no_thinking) {
  const provider::endpoint at{.base = "https://api.anthropic.com/v1",
                              .model = "claude-3-haiku"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  const asking how{.thinking = true, .temperature = 0.7};
  const auto body = json::read(provider::body_of(at, turns, provider::wants_of(how)));
  EXPECT_TRUE(body.has_value());
  if (!body.has_value())
    return;
  EXPECT_TRUE(in(*body, "thinking") == nullptr);
  EXPECT_EQ(number_in(*body, "temperature"), 0.7);
}

namespace {

// One conversation where an answer had stopped to ask for something on the
// way: what the reader asked, what the model said while asking, what the call
// answered, and the answer as it stood -- kept as this tree keeps such an
// answer, in one message with the round behind its thought.
[[nodiscard]] std::vector<message> a_round_of_asking() {
  const conversation_id chat{"kitchen"};
  std::vector<message> said;
  said.push_back(message{.id = message_id{"kitchen-1"},
                         .chat = chat,
                         .text = "What was said about butter?",
                         .outgoing = true});
  said.push_back(message{
      .id = message_id{"kitchen-2"},
      .chat = chat,
      .text = "Butter was said twice, both about keeping it cold.",
      .before = thought{.took_ms = 40,
                        .tool_calls = 1,
                        .said = "> looked\n",
                        .calls = {call_made{.tool = "look_up_in_this_conversation",
                                            .asked = R"({"what":"butter"})",
                                            .answered = "two said it, both about the cold"}}}});
  return said;
}

// The place of every round of asking in a body: the assistant turn that made
// the calls, where it is, and the id both ends of the round were given.
[[nodiscard]] std::size_t place_of_asking(const json::value &messages,
                                          std::string &own) {
  for (std::size_t at = 0; at < 20; ++at) {
    const json::value *one = json::said(&messages, at);
    if (one == nullptr)
      break;
    const json::value *calls = in(*one, "tool_calls");
    if (calls == nullptr)
      continue;
    const json::value *first = json::said(calls, std::size_t{0});
    if (first == nullptr)
      continue;
    own = text_in(*first, "id");
    return at;
  }
  return 20;
}

}  // namespace

// An answer that had stopped to ask: what was asked goes back as the model
// asked it, what the call answered goes back against it, and the answer as it
// stood comes after -- in one id at both ends, which this program makes
// itself, because what the wire named the calls with then was never written
// down with the answer.
SAID(provider, a_round_of_asking_and_answering_is_asked_of_the_wire_again) {
  const std::vector<message> said = a_round_of_asking();
  const std::vector<provider::turn> turns = provider::turns_of(said);
  EXPECT_EQ(turns.size(), 4u);
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini"};
  const auto body = json::read(provider::body_of(at, turns));
  EXPECT_TRUE(body.has_value());
  if (!body.has_value())
    return;
  const json::value *messages = in(*body, "messages");
  EXPECT_TRUE(messages != nullptr);
  if (messages == nullptr)
    return;
  std::string own;
  const std::size_t asking_at = place_of_asking(*messages, own);
  EXPECT_EQ(asking_at, std::size_t{1});
  EXPECT_FALSE(own.empty());
  const json::value *asking = json::said(messages, asking_at);
  const json::value *calls = asking ? in(*asking, "tool_calls") : nullptr;
  const json::value *call = calls ? json::said(calls, std::size_t{0}) : nullptr;
  EXPECT_TRUE(call != nullptr);
  if (call != nullptr) {
    const json::value *function = in(*call, "function");
    EXPECT_TRUE(function != nullptr);
    if (function != nullptr) {
      EXPECT_EQ(text_in(*function, "name"),
                std::string("look_up_in_this_conversation"));
      EXPECT_EQ(text_in(*function, "arguments"), std::string(R"({"what":"butter"})"));
    }
    EXPECT_EQ(text_in(*call, "id"), own);
  }
  const json::value *answered = json::said(messages, asking_at + 1);
  EXPECT_TRUE(answered != nullptr);
  if (answered != nullptr) {
    EXPECT_EQ(text_in(*answered, "role"), std::string("tool"));
    EXPECT_EQ(text_in(*answered, "tool_call_id"), own);
    EXPECT_EQ(text_in(*answered, "content"),
              std::string("two said it, both about the cold"));
  }
  const json::value *after = json::said(messages, asking_at + 2);
  EXPECT_TRUE(after != nullptr);
  if (after != nullptr) {
    EXPECT_EQ(text_in(*after, "role"), std::string("assistant"));
    EXPECT_EQ(text_in(*after, "content"),
              std::string("Butter was said twice, both about keeping it cold."));
  }
}

// The same round in anthropic's own words: a tool_use block under the model,
// the answer as a tool_result against the same id, and the answer itself as
// the text block after them.
SAID(provider, the_round_again_in_anthropics_own_words) {
  const std::vector<message> said = a_round_of_asking();
  const std::vector<provider::turn> turns = provider::turns_of(said);
  const provider::endpoint at{.base = "https://api.anthropic.com/v1",
                              .model = "haiku-4"};
  const auto body = json::read(provider::body_of(at, turns));
  EXPECT_TRUE(body.has_value());
  if (!body.has_value())
    return;
  const json::value *messages = in(*body, "messages");
  EXPECT_TRUE(messages != nullptr);
  if (messages == nullptr)
    return;
  // messages[0] is the reader's; the asking is the model's turn after it.
  const json::value *asking = json::said(messages, std::size_t{1});
  const json::value *blocks = asking ? in(*asking, "content") : nullptr;
  const json::value *block = blocks ? json::said(blocks, std::size_t{0}) : nullptr;
  EXPECT_TRUE(block != nullptr);
  if (block == nullptr)
    return;
  EXPECT_EQ(text_in(*block, "type"), std::string("tool_use"));
  EXPECT_EQ(text_in(*block, "name"), std::string("look_up_in_this_conversation"));
  const std::string own = text_in(*block, "id");
  EXPECT_FALSE(own.empty());
  const json::value *answered = json::said(messages, std::size_t{2});
  const json::value *answer_blocks = answered ? in(*answered, "content") : nullptr;
  const json::value *answer =
      answer_blocks ? json::said(answer_blocks, std::size_t{0}) : nullptr;
  EXPECT_TRUE(answer != nullptr);
  if (answer != nullptr) {
    EXPECT_EQ(text_in(*answer, "type"), std::string("tool_result"));
    EXPECT_EQ(text_in(*answer, "tool_use_id"), own);
    EXPECT_EQ(text_in(*answer, "content"),
              std::string("two said it, both about the cold"));
  }
  const json::value *after = json::said(messages, std::size_t{3});
  const json::value *after_blocks = after ? in(*after, "content") : nullptr;
  const json::value *text =
      after_blocks ? json::said(after_blocks, std::size_t{0}) : nullptr;
  EXPECT_TRUE(text != nullptr);
  if (text != nullptr)
    EXPECT_EQ(text_in(*text, "text"),
              std::string("Butter was said twice, both about keeping it cold."));
}

// What a call asked for, where the asking was written down in plain words --
// which is how this tree's own sample keeps them: an object is what a wire
// takes, and there is none here, so the call goes out without arguments
// rather than with words the wire would read as JSON.
SAID(provider, an_asking_kept_in_plain_words_goes_out_without_arguments) {
  const conversation_id chat{"kitchen"};
  std::vector<message> said;
  said.push_back(message{.id = message_id{"kitchen-1"},
                         .chat = chat,
                         .text = "What was said about butter?",
                         .outgoing = true});
  said.push_back(message{
      .id = message_id{"kitchen-2"},
      .chat = chat,
      .text = "Two things, both about keeping it cold.",
      .before = thought{
          .took_ms = 40,
          .said = "> looked\n",
          .calls = {call_made{.tool = "look_up_in_this_conversation",
                              .asked = "what was said here about butter",
                              .answered = "two said it"}}}});
  const std::vector<provider::turn> turns = provider::turns_of(said);
  EXPECT_EQ(turns.size(), 4u);
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini"};
  const auto body = json::read(provider::body_of(at, turns));
  EXPECT_TRUE(body.has_value());
  if (!body.has_value())
    return;
  const json::value *messages = in(*body, "messages");
  EXPECT_TRUE(messages != nullptr);
  if (messages == nullptr)
    return;
  std::string own;
  const std::size_t asking_at = place_of_asking(*messages, own);
  const json::value *asking = json::said(messages, asking_at);
  const json::value *calls = asking ? in(*asking, "tool_calls") : nullptr;
  const json::value *call = calls ? json::said(calls, std::size_t{0}) : nullptr;
  EXPECT_TRUE(call != nullptr);
  if (call != nullptr) {
    const json::value *function = in(*call, "function");
    EXPECT_TRUE(function != nullptr);
    if (function != nullptr)
      EXPECT_EQ(text_in(*function, "arguments"), std::string("{}"));
  }
}

SAID(provider, whose_words_an_unknown_model_speaks_is_said_by_the_door) {
  // The answerer a model is known by speaks first, whatever door it was
  // asked through: a Claude named in a conversation still speaks its own
  // family's words at a base that has nothing to do with them.
  const provider::endpoint known{.base = "https://api.deepseek.com/v1",
                                 .model = "haiku-4",
                                 .family = "openai"};
  EXPECT_EQ(provider::wire_of(known), provider::wire::anthropic);

  // A model nobody here has heard of: the door says whose words it speaks,
  // and they go where those words are said -- Anthropic's path at
  // Anthropic's own base, whatever it ends in.
  const provider::endpoint theirs{
      .base = "https://api.anthropic.com/v1",
      .model = "claude-3-5-haiku-20241022",
      .family = "anthropic"};
  EXPECT_EQ(provider::wire_of(theirs), provider::wire::anthropic);
  EXPECT_EQ(provider::url_of(theirs),
            std::string("https://api.anthropic.com/v1/messages"));

  // Google's door the same: its own method carries the model inside it,
  // and the door's words say so before the base's ending gets a word in.
  const provider::endpoint gem{
      .base = "https://generativelanguage.googleapis.com/v1beta",
      .model = "some-new-flash",
      .family = "gemini"};
  EXPECT_EQ(provider::wire_of(gem), provider::wire::gemini);

  // Most of these answer in OpenAI's shape, and which of its two wires is
  // still the base's own ending to say.
  const provider::endpoint compat{.base = "https://api.groq.com/openai/v1",
                                  .model = "llama-3.3-70b-versatile",
                                  .family = "openai"};
  EXPECT_EQ(provider::wire_of(compat), provider::wire::chat);
  EXPECT_EQ(provider::url_of(compat),
            std::string("https://api.groq.com/openai/v1/chat/completions"));
  const provider::endpoint both{.base = "https://their.example/v1beta",
                                .model = "somebody-new"};
  EXPECT_EQ(provider::wire_of(both), provider::wire::responses);
}

SAID(provider, the_key_of_the_door_goes_out_once_where_that_wire_asks) {
  const auto has = [](const std::vector<std::pair<std::string, std::string>>
                          &said,
                      std::string_view name, std::string_view value) {
    for (const auto &[key, got] : said)
      if (key == name && got == value)
        return true;
    return false;
  };
  // OpenAI's shape: the bearer, and nothing asking for it twice.
  const provider::endpoint openai{.base = "https://api.openai.com/v1",
                                  .key = "sk-openai-one",
                                  .model = "o4-mini"};
  const auto openai_headers = provider::headers_of(openai);
  EXPECT_TRUE(has(openai_headers, "Authorization", "Bearer sk-openai-one"));
  EXPECT_TRUE(!has(openai_headers, "x-api-key", ""));

  // Anthropic's own: its own name for the key beside the version its wire
  // is written as.
  const provider::endpoint anthropic{.base = "https://api.anthropic.com/v1",
                                     .key = "sk-ant-one",
                                     .model = "haiku-4"};
  const auto anthropic_headers = provider::headers_of(anthropic);
  EXPECT_TRUE(has(anthropic_headers, "x-api-key", "sk-ant-one"));
  EXPECT_TRUE(has(anthropic_headers, "anthropic-version", "2023-06-01"));
  EXPECT_TRUE(!has(anthropic_headers, "Authorization", ""));

  // Google's: its own name, and nothing beside it.
  const provider::endpoint google{
      .base = "https://generativelanguage.googleapis.com/v1beta",
      .key = "sk-goog-one",
      .model = "gemini-2.5-flash"};
  const auto google_headers = provider::headers_of(google);
  EXPECT_TRUE(has(google_headers, "x-goog-api-key", "sk-goog-one"));
  EXPECT_TRUE(!has(google_headers, "Authorization", ""));

  // No key in the door: no header asking for one, whichever door stands.
  const provider::endpoint bare{.base = "https://api.openai.com/v1",
                                .model = "o4-mini"};
  for (const auto &[key, got] : provider::headers_of(bare)) {
    EXPECT_TRUE(key != "Authorization");
    EXPECT_TRUE(key != "x-api-key");
    EXPECT_TRUE(key != "x-goog-api-key");
  }
}

SAID(provider, a_capsule_goes_out_as_context_and_not_as_anyones_words) {
  chats_root root;
  root.messages.push_back(message{.id = message_id{"c"},
                                  .text = "what was said, kept",
                                  .state = delivery::sent,
                                  .capsule = true});
  root.messages.push_back(message{.id = message_id{"1"},
                                  .text = "and this after it",
                                  .outgoing = true});
  const std::vector<provider::turn> turns = provider::turns_of(root.messages);
  EXPECT_EQ(turns.size(), 2u);
  EXPECT_EQ(turns[0].who, provider::role::system);
  EXPECT_EQ(turns[0].text, std::string("what was said, kept"));
  EXPECT_EQ(turns[1].who, provider::role::user);
  EXPECT_EQ(turns[1].text, std::string("and this after it"));
}

SAID(provider, a_picture_goes_out_the_way_each_family_reads_one) {
  chats_root root;
  message said{.id = message_id{"1"},
               .text = "look at this",
               .outgoing = true};
  said.images.push_back(image_part{"image/png", "aGVsbG8="});
  root.messages.push_back(std::move(said));
  const std::vector<provider::turn> turns = provider::turns_of(root.messages);
  EXPECT_EQ(turns.size(), 1u);
  EXPECT_EQ(turns[0].images.size(), 1u);
  EXPECT_EQ(turns[0].images[0].media, std::string("image/png"));

  // A chat wire: content becomes the list, the picture a data URL in it.
  const provider::endpoint chat{.base = "https://api.openai.com/v1",
                                .model = "o4-mini"};
  const auto chat_json = json::read(provider::body_of(chat, turns));
  EXPECT_TRUE(chat_json.has_value());
  if (chat_json) {
    const json::value *messages = json::said(*chat_json, "messages");
    const json::value *first = messages ? json::item_of(*messages, 0) : nullptr;
    EXPECT_TRUE(first != nullptr);
    if (first) {
      const json::value *content = json::said(*first, "content");
      EXPECT_TRUE(content && json::item_of(*content, 1) != nullptr);
      if (content && json::item_of(*content, 1)) {
        const json::value *picture = json::item_of(*content, 1);
        EXPECT_EQ(text_in(*picture, "type"), std::string("image_url"));
        const json::value *url = json::said(*picture, "image_url");
        EXPECT_TRUE(url != nullptr);
        if (url)
          EXPECT_EQ(text_in(*url, "url"),
                    std::string("data:image/png;base64,aGVsbG8="));
      }
    }
  }

  // Anthropic's own: a block of its own name, base64 source beneath it.
  const provider::endpoint anthropic{.base = "https://api.anthropic.com/v1",
                                     .model = "haiku-4"};
  const auto ant_json = json::read(provider::body_of(anthropic, turns));
  EXPECT_TRUE(ant_json.has_value());
  if (ant_json) {
    const json::value *messages = json::said(*ant_json, "messages");
    const json::value *first = messages ? json::item_of(*messages, 0) : nullptr;
    EXPECT_TRUE(first != nullptr);
    if (first) {
      const json::value *content = json::said(*first, "content");
      bool found_image = false;
      if (content)
        for (std::size_t at = 0u;; ++at) {
          const json::value *block = json::item_of(*content, at);
          if (block == nullptr)
            break;
          if (text_in(*block, "type") == "image") {
            found_image = true;
            const json::value *source = json::said(*block, "source");
            EXPECT_TRUE(source != nullptr);
            if (source) {
              EXPECT_EQ(text_in(*source, "media_type"),
                        std::string("image/png"));
              EXPECT_EQ(text_in(*source, "data"),
                        std::string("aGVsbG8="));
            }
          }
        }
      EXPECT_TRUE(found_image);
    }
  }

  // Google's: inline_data in the parts of the contents.
  const provider::endpoint gemini{
      .base = "https://generativelanguage.googleapis.com/v1beta",
      .model = "gemini-2.5-flash"};
  const auto gem_json = json::read(provider::body_of(gemini, turns));
  EXPECT_TRUE(gem_json.has_value());
  if (gem_json) {
    const json::value *contents = json::said(*gem_json, "contents");
    const json::value *first = contents ? json::item_of(*contents, 0) : nullptr;
    EXPECT_TRUE(first != nullptr);
    if (first) {
      const json::value *parts = json::said(*first, "parts");
      bool found_image = false;
      if (parts)
        for (std::size_t at = 0u;; ++at) {
          const json::value *part = json::item_of(*parts, at);
          if (part == nullptr)
            break;
          const json::value *inline_data = json::said(*part, "inline_data");
          if (inline_data) {
            found_image = true;
            EXPECT_EQ(text_in(*inline_data, "mime_type"),
                      std::string("image/png"));
            EXPECT_EQ(text_in(*inline_data, "data"),
                      std::string("aGVsbG8="));
          }
        }
      EXPECT_TRUE(found_image);
    }
  }
}

int main() { return pchat::tests::run(); }
