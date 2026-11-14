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

// The same failure, said the way retry.h says it: the kind Agora names it by,
// and what of code, type and message came with it. A module unit never sees
// the variant, only this.
[[nodiscard]] failure failure_of(const sn::GenerationError &error) {
  if (const auto *api = std::get_if<sn::ErrorApi>(&error))
    return {failure_kind::api, api->code.value_or(std::string()),
            api->type.value_or(std::string()), api->message, 0};
  if (const auto *network = std::get_if<sn::ErrorNetwork>(&error))
    return {failure_kind::network, std::string(), std::string(),
            network->message, network->status_code};
  if (const auto *sse = std::get_if<sn::ErrorSseParse>(&error))
    return {failure_kind::line, std::string(), std::string(), sse->cause, 0};
  if (const auto *bad_call = std::get_if<sn::ErrorMalformedToolCall>(&error))
    return {failure_kind::malformed_call, std::string(), std::string(),
            bad_call->cause, 0};
  if (const auto *incomplete = std::get_if<sn::ErrorIncompleteStream>(&error)) {
    failure out{failure_kind::unfinished, std::string(), std::string(),
                incomplete->provider, 0};
    if (incomplete->tool_call_in_flight)
      out.message = "a tool call it asked for was never finished";
    return out;
  }
  if (std::holds_alternative<sn::ErrorOutputTruncated>(error))
    return {failure_kind::truncated, std::string(), std::string(),
            "the answer ran out of the model's window", 0};
  if (std::holds_alternative<sn::ErrorTimeout>(error))
    return {failure_kind::timed_out, std::string(), std::string(),
            "the answer timed out", 0};
  return {failure_kind::api, std::string(), std::string(),
          "the wire stopped the answer", 0};
}

// The citation, said the way citation.h says it: the ported record's fields a
// screen shows, so no module unit sees the ported header.
[[nodiscard]] citation citation_of(const agora::core::citations::CitationRecord &record) {
  return {record.source_id, record.provider, record.kind, record.title,
          record.url.value_or(std::string()),
          record.excerpt.value_or(std::string{})};
}

// One event of the wire, in the frames this folder hands over. A tool call is
// only said whole, the router having gathered its pieces; Gemini gives a call
// a name and no id, and the empty id is the honest thing to hand on. What the
// hosted calls and the citations say is display material for a screen that has
// none here yet.
void frame_of(const gs::StreamEvent &event, std::vector<gemini_frame> &out) {
  const std::size_t before = out.size();
  using frame = gemini_frame;
  if (const auto *text = std::get_if<sn::TextChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::text, .text = text->text});
  } else if (const auto *thought = std::get_if<sn::ThoughtChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::thought, .text = thought->thought});
  } else if (const auto *usage = std::get_if<gs::UsageUpdate>(&event)) {
    // The wire counts in ints and can leave one out; a count of tokens held as
    // unsigned stops at zero, as the counters elsewhere do.
    const int input = usage->usage.input_token_count.value_or(0);
    const int output = usage->usage.output_token_count.value_or(0);
    out.push_back(frame{.what = frame::kind::usage,
                        .input_tokens = static_cast<std::uint32_t>(input > 0 ? input : 0),
                        .output_tokens = static_cast<std::uint32_t>(output > 0 ? output : 0)});
  } else if (const auto *call = std::get_if<sn::ToolCallRequest>(&event)) {
    out.push_back(frame{.what = frame::kind::tool,
                        .id = call->id,
                        .name = call->name,
                        .arguments = call->arguments});
  } else if (const auto *update = std::get_if<sn::ToolCallUpdate>(&event)) {
    // A call its arguments are still arriving for: named, and as much of its
    // arguments as the wire has sent. The whole call comes as a request when
    // the block stops; this says what stood in between. Where the wire gave no
    // id, the stream key is what it is called by.
    out.push_back(frame{.what = frame::kind::tool_partial,
                        .id = update->id.value_or(update->stream_key),
                        .name = update->name,
                        .arguments = update->arguments});
  } else if (const auto *hosted = std::get_if<sn::HostedToolCallUpdate>(&event)) {
    // A call the wire runs itself -- a search, a code execution -- said as it
    // runs: its name, what it was given, and what came back where it is back.
    out.push_back(frame{.what = frame::kind::tool_partial,
                        .id = hosted->stream_key,
                        .name = hosted->name,
                        .arguments = hosted->arguments});
  } else if (const auto *cited = std::get_if<gs::CitationUpdate>(&event)) {
    // A source the answer leaned on: what it came from, what it is called,
    // where to read it -- said as the wire sent it, kept so a screen can show
    // what the answer was built from.
    out.push_back(frame{.what = frame::kind::citation,
                        .cited = citation_of(cited->citation)});
  }
  // The ported rule says whether this event carries what the model said:
  // set on the one frame this event made, so the end of the stream can
  // ask it without keeping the event.
  if (!out.empty() && out.size() > before)
    out.back().carried = gs::carries_model_output(event);
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
                          .text = error_text(*reader.router.stream_error),
                          .why_failed = failure_of(*reader.router.stream_error)});
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
                        .text = error_text(*reader.router.stream_error),
                        .why_failed = failure_of(*reader.router.stream_error)});
    return out;
  }
  if (reader.router.saw_done && !reader.ended_emitted) {
    reader.ended_emitted = true;
    out.push_back(frame{.what = frame::kind::ended,
                        .stop_reason = reader.router.finish_reason.value_or("stop")});
  }
  return out;
}

}  // namespace pchat::provider
