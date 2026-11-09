module;
#include "tests.h"
export module pchat.test.provider_gemini;
// SPDX-License-Identifier: AGPL-3.0-only
// What the gemini wire's frames and bodies say through the folder that reads
// them: an answer's SSE -- a thought, a text, a whole tool call, what it cost
// and why it stopped -- a failure the wire reports inside the stream, and a
// body read back to the shape generateContent writes, part by part.
import std;
import pchat.json;
import pchat.provider;
import pchat.provider.stream;

namespace {

using pchat::provider::endpoint;
using pchat::provider::event;
using pchat::provider::role;
using pchat::provider::stream;
using pchat::provider::turn;
using pchat::provider::wanted;
using pchat::provider::wire;

// One answer's bytes as gemini writes them over alt=sse: a data line per
// event, a blank line between them, the thought before the answer as this
// wire's own part of it, and the cost beside the reason it stopped on the
// frame that ends it.
[[nodiscard]] std::string answer_bytes() {
  static constexpr std::array<std::string_view, 3> frames = {
      R"(data: {"candidates":[{"content":{"role":"model","parts":[{"thought":"hmm."}]}}]})",
      R"(data: {"candidates":[{"content":{"role":"model","parts":[{"text":"The answer."}]}}]})",
      R"(data: {"candidates":[{"content":{"role":"model","parts":[{"functionCall":{"name":"get_time","args":{"city":"Rome"}}}]},"finishReason":"STOP"}],"usageMetadata":{"promptTokenCount":7,"candidatesTokenCount":12}})",
  };
  std::string out;
  for (const std::string_view frame : frames) {
    out.append(frame);
    out.append("\n\n");
  }
  return out;
}

}  // namespace

SAID(gemini, one_answer_is_read_in_the_words_every_wire_uses) {
  stream one{wire::gemini};
  one.feed(answer_bytes());

  EXPECT_FALSE(one.failed());
  EXPECT_EQ(std::string(one.text()), std::string("The answer."));
  EXPECT_EQ(std::string(one.thought()), std::string("hmm."));
  EXPECT_TRUE(one.ended());
  EXPECT_EQ(std::string(one.finish()), std::string("stop"));
  EXPECT_EQ(one.cost().in, 7u);
  EXPECT_EQ(one.cost().out, 12u);
  EXPECT_EQ(one.calls().size(), std::size_t{1});
  if (!one.calls().empty()) {
    // Gemini gives a call a name and no id of its own; what answers it comes
    // back against is the name the router gave it, and an id empty at both
    // ends would pair every answer with the first call instead.
    EXPECT_FALSE(one.calls()[0].id.empty());
    EXPECT_EQ(one.calls()[0].name, std::string("get_time"));
    EXPECT_EQ(one.calls()[0].arguments, std::string("{\"city\":\"Rome\"}"));
  }

  // Every kind of frame the answer held was said into the same queue the chat
  // wire fills, in the order it arrived.
  bool saw_text = false;
  bool saw_tool = false;
  bool saw_usage = false;
  bool saw_ended = false;
  for (const event &frame : one.take()) {
    saw_text = saw_text || frame.what == event::kind::text;
    saw_tool = saw_tool || frame.what == event::kind::tool;
    saw_usage = saw_usage || frame.what == event::kind::usage;
    saw_ended = saw_ended || frame.what == event::kind::ended;
  }
  EXPECT_TRUE(saw_text);
  EXPECT_TRUE(saw_tool);
  EXPECT_TRUE(saw_usage);
  EXPECT_TRUE(saw_ended);
}

SAID(gemini, a_failure_the_wire_reports_inside_the_stream_says_why) {
  stream one{wire::gemini};
  one.feed(R"(data: {"error":{"code":429,"message":"quota exceeded","status":"RESOURCE_EXHAUSTED"}})"
           "\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_TRUE(std::string(one.why()).find("quota exceeded") != std::string::npos);
}

SAID(gemini, a_body_is_written_the_way_generateContent_reads_it) {
  const endpoint one{.base = "https://generativelanguage.googleapis.com/v1beta",
                     .key = "test-key",
                     .model = "gemini-2.5-flash",
                     .told = "Answer shortly."};
  const std::vector<turn> turns{
      {.who = role::user, .text = "Two eggs?"},
      {.who = role::assistant,
       .text = "Then scramble.",
       .calls = {pchat::provider::tool_call{
           .id = "call_1", .name = "get_time", .arguments = R"({"city":"Rome"})"}}},
      {.who = role::tool, .own = "call_1", .text = "noon"},
      {.who = role::user, .text = "Thanks."},
  };
  const wanted says{.stream = true,
                    .thinking = true,
                    .temperature = 0.5,
                    .tools = {pchat::provider::offered{
                        "get_time", "Say what time it is",
                        {{"city", "The city the time is for"}}}}};
  const std::string body = pchat::provider::gemini_body_of(one, turns, says);

  auto whole = pchat::json::read(body);
  EXPECT_TRUE(static_cast<bool>(whole));
  if (!whole)
    return;
  const auto *root = &*whole;

  // What it is told stands as an instruction of its own, not as a turn.
  const auto *told = pchat::json::said(root, "systemInstruction");
  EXPECT_TRUE(told != nullptr);
  if (told) {
    const auto *parts = pchat::json::said(pchat::json::said(told, "parts"), 0);
    EXPECT_TRUE(parts != nullptr);
    if (parts)
      EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(parts, "text")).value_or("")),
                std::string("Answer shortly."));
  }

  // Every turn as contents: what a person wrote, what the model answered with
  // its call beside its text, the answer to that call, and the last word.
  const auto *contents = pchat::json::said(root, "contents");
  EXPECT_TRUE(contents != nullptr);
  if (contents) {
    const auto *first = pchat::json::said(contents, 0);
    EXPECT_TRUE(first != nullptr);
    if (first) {
      EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(first, "role")).value_or("")), std::string("user"));
      const auto *first_part = pchat::json::said(pchat::json::said(first, "parts"), 0);
      EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(first_part, "text")).value_or("")),
                std::string("Two eggs?"));
    }

    const auto *second = pchat::json::said(contents, 1);
    EXPECT_TRUE(second != nullptr);
    if (second) {
      EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(second, "role")).value_or("")), std::string("model"));
      const auto *called = pchat::json::said(pchat::json::said(pchat::json::said(second, "parts"), 1), "functionCall");
      EXPECT_TRUE(called != nullptr);
      if (called)
        EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(called, "name")).value_or("")),
                  std::string("get_time"));
    }

    // The answer to the call goes back as a user's part, against the name the
    // call went out under and not the id only this tree keeps.
    const auto *third = pchat::json::said(contents, 2);
    EXPECT_TRUE(third != nullptr);
    if (third) {
      EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(third, "role")).value_or("")), std::string("user"));
      const auto *answered = pchat::json::said(pchat::json::said(pchat::json::said(third, "parts"), 0), "functionResponse");
      EXPECT_TRUE(answered != nullptr);
      if (answered) {
        EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(answered, "name")).value_or("")),
                  std::string("get_time"));
        const auto *response = pchat::json::said(answered, "response");
        EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(response, "result")).value_or("")),
                  std::string("noon"));
      }
    }
    EXPECT_TRUE(pchat::json::said(contents, 3) != nullptr);
  }

  // The choices an answer is made by under one config, and thinking as a
  // budget of tokens: thousands when the reader asked for it.
  const auto *config = pchat::json::said(root, "generationConfig");
  EXPECT_TRUE(config != nullptr);
  if (config) {
    const auto *heat = pchat::json::said(config, "temperature");
    EXPECT_TRUE(heat != nullptr);
    if (heat)
      EXPECT_EQ(*pchat::json::number_of(*heat), 0.5);
    const auto *thinking = pchat::json::said(config, "thinkingConfig");
    EXPECT_TRUE(thinking != nullptr);
    if (thinking)
      EXPECT_EQ(*pchat::json::number_of(*pchat::json::said(thinking, "thinkingBudget")), 8192.0);
  }

  // A tool as this wire reads it: one declaration, one question.
  const auto *tools = pchat::json::said(root, "tools");
  EXPECT_TRUE(tools != nullptr);
  if (tools) {
    const auto *decl = pchat::json::said(pchat::json::said(pchat::json::said(tools, 0), "functionDeclarations"), 0);
    EXPECT_TRUE(decl != nullptr);
    if (decl) {
      EXPECT_EQ(std::string(pchat::json::text_of(*pchat::json::said(decl, "name")).value_or("")),
                std::string("get_time"));
      const auto *takes = pchat::json::said(pchat::json::said(pchat::json::said(decl, "parameters"), "properties"), "city");
      EXPECT_TRUE(takes != nullptr);
    }
  }
}

int main() { return pchat::tests::run(); }
