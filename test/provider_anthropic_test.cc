module;
#include "tests.h"
export module pchat.test.provider_anthropic;
// SPDX-License-Identifier: AGPL-3.0-only
// What the anthropic wire's frames say through the folder that reads them: a
// whole answer's SSE -- its framing, a thought, a text block, a tool call whose
// arguments arrive piecewise, what it cost and why it stopped -- and a failure
// the wire reports inside the stream, both said in the words every wire's
// frames are said in here.
import std;
import pchat.provider;
import pchat.provider.stream;

namespace {

using pchat::provider::event;
using pchat::provider::stream;
using pchat::provider::wire;

// One answer's bytes as anthropic writes them: `event:` lines beside `data:`,
// a blank line ending every frame, and a comment left in the stream that
// nothing reads and nothing trips on.
[[nodiscard]] std::string answer_bytes() {
  static constexpr std::array<std::string_view, 13> frames = {
      R"(data: {"type":"message_start","message":{"id":"msg_1","type":"message","role":"assistant","model":"claude-test","usage":{"input_tokens":7}}})",
      R"(: keepalive, and nothing of the answer waits on it)",
      R"(event: content_block_start)",
      R"(data: {"type":"content_block_start","index":0,"content_block":{"type":"thinking"}})",
      R"(data: {"type":"content_block_delta","index":0,"delta":{"type":"thinking_delta","thinking":"hmm."}})",
      R"(event: content_block_start)",
      R"(data: {"type":"content_block_start","index":1,"content_block":{"type":"text","text":""}})",
      R"(data: {"type":"content_block_delta","index":1,"delta":{"type":"text_delta","text":"The answer."}})",
      R"(data: {"type":"content_block_start","index":2,"content_block":{"type":"tool_use","id":"toolu_01","name":"get_time","input":{}}})",
      R"(data: {"type":"content_block_delta","index":2,"delta":{"type":"input_json_delta","partial_json":"{\"city\":\"Rome\"}"}})",
      R"(data: {"type":"content_block_stop","index":2})",
      R"(data: {"type":"message_delta","delta":{"stop_reason":"tool_use"},"usage":{"output_tokens":12}})",
      R"(data: {"type":"message_stop"})",
  };
  std::string out;
  for (const std::string_view frame : frames) {
    out.append(frame);
    out.append("\n\n");
  }
  return out;
}

}  // namespace

SAID(anthropic, one_answer_is_read_in_the_words_every_wire_uses) {
  stream one{wire::anthropic};
  one.feed(answer_bytes());

  EXPECT_FALSE(one.failed());
  EXPECT_EQ(std::string(one.text()), std::string("The answer."));
  EXPECT_EQ(std::string(one.thought()), std::string("hmm."));
  EXPECT_TRUE(one.ended());
  EXPECT_EQ(std::string(one.finish()), std::string("tool_use"));
  EXPECT_EQ(one.cost().in, 7u);
  EXPECT_EQ(one.cost().out, 12u);
  EXPECT_EQ(one.calls().size(), std::size_t{1});
  if (!one.calls().empty()) {
    EXPECT_EQ(one.calls()[0].id, std::string("toolu_01"));
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

// A call the wire opened and never finished: its arguments arrive piece by
// piece, and what arrived is kept as the call it is building -- said, not
// dropped -- so a screen can show it while it is still being said. The whole
// call comes when the block stops, and replaces what was arriving.
SAID(anthropic, a_call_the_wire_never_finished_keeps_what_arrived) {
  stream one{wire::anthropic};
  const std::string frame_one =
      R"(data: {"type":"content_block_start","index":0,)"
      R"("content_block":{"type":"tool_use","id":"toolu_01",)"
      R"("name":"get_time","input":{}}})";
  const std::string frame_two =
      R"(data: {"type":"content_block_delta","index":0,)"
      R"("delta":{"type":"input_json_delta",)"
      R"("partial_json":"{\"tz\":\"Rome\"}"}})";
  one.feed(frame_one + "\n\n" + frame_two + "\n\n");
  // The wire never sent the block stop: the call is not whole, so it is not
  // among the calls, but what arrived of it is kept.
  EXPECT_EQ(one.calls().size(), std::size_t(0));
  EXPECT_TRUE(!one.partial_calls().empty());
  if (!one.partial_calls().empty()) {
    EXPECT_EQ(one.partial_calls().front().name, std::string("get_time"));
    EXPECT_EQ(one.partial_calls().front().arguments,
              std::string("{\"tz\":\"Rome\"}"));
  }
}

// A source the answer leaned on: the wire sends it with the block it belongs
// to, and it is kept -- where it came from, what it is called, where to read
// it -- so a screen can show what the answer was built from. Nothing kept
// where it cited nothing.
SAID(anthropic, a_source_the_answer_leaned_on_is_kept) {
  stream one{wire::anthropic};
  const std::string frame =
      R"(data: {"type":"content_block_start","index":0,)"
      R"("content_block":{"type":"text","text":"Snow.",)"
      R"("citations":[{"type":"web_search_result_location","cited_text":"Snow",)"
      R"("document_index":0,"document_title":"Weather",)"
      R"("start_char_index":0,"end_char_index":4,)"
      R"("url":"https://example.test/snow"}]}})";
  one.feed(frame + "\n\n"
           R"(data: {"type":"content_block_stop","index":0})" "\n\n");
  EXPECT_TRUE(!one.citations().empty());
  if (!one.citations().empty()) {
    EXPECT_EQ(one.citations().front().title, std::string("Weather"));
    EXPECT_EQ(one.citations().front().url,
              std::string("https://example.test/snow"));
  }
}

// Whether anything of the answer reached the screen, said the way Agora's
// termination says it: a stream that spoke has it, a stream that was cut
// before it spoke does not. The policy reads it when it decides whether to
// ask again, so it is read here from a real stream and not from a literal.
SAID(anthropic, a_stream_that_spoke_says_it_produced_something) {
  stream one{wire::anthropic};
  one.feed(answer_bytes());
  EXPECT_TRUE(one.ended_by().produced_content);
}

// An answer that asked for a call and said no text: the ported rule counts a
// call as what the model said, so it is an answer that produced something --
// where a rule that reads only text and thought would say it did not.
SAID(anthropic, an_answer_that_asked_for_a_call_produced_something) {
  stream one{wire::anthropic};
  one.feed(R"(data: {"type":"content_block_start","index":0,)"
           "\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_1\","
           "\"name\":\"weather\",\"input\":{}}})"
           "\n\n"
           "data: {\"type\":\"content_block_delta\",\"index\":0,"
           "\"delta\":{\"type\":\"input_json_delta\","
           "\"partial_json\":\"{}\"}})"
           "\n\n"
           "data: {\"type\":\"content_block_stop\",\"index\":0})"
           "\n\n");
  EXPECT_TRUE(one.ended_by().produced_content);
}

SAID(anthropic, a_stream_that_never_spoke_says_it_produced_nothing) {
  stream one{wire::anthropic};
  EXPECT_FALSE(one.ended_by().produced_content);
}

SAID(anthropic, a_failure_the_wire_reports_inside_the_stream_says_why) {
  stream one{wire::anthropic};
  one.feed(R"(data: {"type":"error","error":{"type":"api_error","message":"overloaded"}})"
           "\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_TRUE(std::string(one.why()).find("overloaded") != std::string::npos);
}

// A line the wire's own events will not read: said as the failure it is, in the
// words the wire gave it, rather than a sentence this folder made up.
SAID(anthropic, a_line_the_wire_cannot_read_fails_in_the_wires_own_words) {
  stream one{wire::anthropic};
  one.feed("data: {not json\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_FALSE(one.why().empty());
  EXPECT_FALSE(one.ended());
}

int main() { return pchat::tests::run(); }
