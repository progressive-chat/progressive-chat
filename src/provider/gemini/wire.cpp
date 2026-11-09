// SPDX-License-Identifier: AGPL-3.0-only
// The gemini wire's own parts: Agora's router behind the plain types the
// modules name. One payload of one frame goes in; what the wire's events
// decided comes out in the words pchat says every wire's frames in -- and the
// router, its response types and their JSON stay on this side of wire.h, so a
// module unit never sees them.
#include "wire.h"

#include <variant>

#include "agora/core/gemini_stream.hpp"
#include "agora/core/stream_normalization.hpp"

namespace pchat::provider {
namespace {

namespace gs = agora::core::gemini_stream;
namespace sn = agora::core::stream_normalization;

// The reader: the wire's own router, and whether it has already said the end
// or the failure of the stream (the router holds both as state, and emits
// neither by itself).
struct router_box {
  gs::GeminiStreamRouter router;
  bool ended_emitted = false;
  bool failed_emitted = false;
  router_box() : router([] { return sn::random_uuid_v4(); }, "Gemini") {}
};

// What a failure says where a screen reads why an answer failed: the message
// the wire carried, in its own words. The same shapes of failure the other
// wires' routers carry, so both folders speak them alike.
std::string error_text(const sn::GenerationError &error) {
  if (const auto *api = std::get_if<sn::ErrorApi>(&error))
    return api->message;
  if (const auto *network = std::get_if<sn::ErrorNetwork>(&error))
    return network->message;
  if (const auto *sse = std::get_if<sn::ErrorSseParse>(&error))
    return sse->cause;
  if (const auto *bad_call = std::get_if<sn::ErrorMalformedToolCall>(&error))
    return bad_call->cause;
  if (std::holds_alternative<sn::ErrorTimeout>(error))
    return "the answer timed out";
  return "the wire stopped the answer";
}

// One event of the wire, in the frames this folder hands over. A tool call is
// only said whole, the router having gathered its pieces; Gemini gives a call
// no id of its own, and the name the router gives it is what an answer to it
// comes back against -- an id empty at both ends would pair every answer with
// the first call instead. What the hosted calls and the citations say is
// display material for a screen that has none here yet.
void frame_of(const gs::StreamEvent &event, std::vector<gemini_frame> &out) {
  using frame = gemini_frame;
  if (const auto *text = std::get_if<sn::TextChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::text, .said = text->text});
  } else if (const auto *thought = std::get_if<sn::ThoughtChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::thought, .said = thought->thought});
  } else if (const auto *usage = std::get_if<gs::UsageUpdate>(&event)) {
    // The wire counts in ints and can leave one out; a count of tokens held as
    // unsigned stops at zero, as the counters elsewhere do.
    const int input = usage->usage.input_token_count.value_or(0);
    const int output = usage->usage.output_token_count.value_or(0);
    out.push_back(frame{.what = frame::kind::usage,
                        .used_in = static_cast<std::uint32_t>(input > 0 ? input : 0),
                        .used_out = static_cast<std::uint32_t>(output > 0 ? output : 0)});
  } else if (const auto *call = std::get_if<sn::ToolCallRequest>(&event)) {
    out.push_back(frame{.what = frame::kind::tool,
                        .call_id = call->id,
                        .call_name = call->name,
                        .call_arguments = call->arguments});
  }
  // sn::ToolCallUpdate, sn::HostedToolCallUpdate, gs::CitationUpdate: nothing
  // to say here yet.
}

}  // namespace

gemini_state *gemini_open() {
  gemini_state *state = new gemini_state();
  state->router = new router_box();
  return state;
}

void gemini_close(gemini_state *state) {
  if (!state)
    return;
  delete static_cast<router_box *>(state->router);
  delete state;
}

std::vector<gemini_frame> gemini_step(gemini_state *state, std::string_view payload) {
  using frame = gemini_frame;
  std::vector<gemini_frame> out;
  if (state == nullptr || state->router == nullptr)
    return out;
  router_box &reader = *static_cast<router_box *>(state->router);
  gs::DecodeFailure failure;
  const std::optional<gs::ApiStreamResponse> decoded =
      gs::decode_api_stream_response(payload, &failure);
  if (!decoded) {
    // What the router keeps of a line its events would not read: said as the
    // failure it is, once, in the words the wire gave it.
    reader.router.capture_parse_error(payload, failure.message);
    if (reader.router.stream_error && !reader.failed_emitted) {
      reader.failed_emitted = true;
      out.push_back(frame{.what = frame::kind::failed,
                          .said = error_text(*reader.router.stream_error)});
    }
    return out;
  }
  for (const gs::StreamEvent &event : reader.router.route(*decoded))
    frame_of(event, out);
  // A failure ends the reading of the stream here: nothing after it is an
  // answer, and the end the wire may still name is not one worth saying.
  if (reader.router.stream_error && !reader.failed_emitted) {
    reader.failed_emitted = true;
    out.push_back(frame{.what = frame::kind::failed,
                        .said = error_text(*reader.router.stream_error)});
    return out;
  }
  // The end of it: where the wire said the reason it stopped on, or where a
  // relay forwarded the `[DONE]` that ends an OpenAI-shaped stream -- a
  // Gemini of its own says it with `finishReason` on its last candidate, and
  // `saw_done` only ever comes from the second kind.
  if ((reader.router.saw_done || reader.router.finish_reason.has_value()) &&
      !reader.ended_emitted) {
    reader.ended_emitted = true;
    out.push_back(frame{.what = frame::kind::ended,
                        .finish = reader.router.finish_reason.value_or("stop")});
  }
  return out;
}

}  // namespace pchat::provider
