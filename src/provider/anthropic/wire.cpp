// SPDX-License-Identifier: AGPL-3.0-only
// The anthropic wire's own parts: Agora's router behind the plain types the
// modules name. One payload of one frame goes in; what the wire's events
// decided comes out in the words pchat says every wire's frames in -- and the
// router, its event types and their JSON stay on this side of wire.h, so a
// module unit never sees them.
#include "wire.h"

#include <variant>

#include "agora/core/anthropic_request.hpp"
#include "agora/core/anthropic_stream.hpp"
#include "agora/core/stream_normalization.hpp"

namespace pchat::provider {
namespace {

namespace as = agora::core::anthropic_stream;
namespace sn = agora::core::stream_normalization;

// The reader: the wire's own router, and whether it has already said the end
// of the stream (the router proves an end but emits no event of it).
struct router_box {
  as::AnthropicStreamEventRouter router;
  bool ended_emitted = false;
  bool failed_emitted = false;
  router_box() : router([] { return sn::random_uuid_v4(); }) {}
};

// What a failure says where a screen reads why an answer failed: the message
// the wire carried, in its own words.
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

// One event of the wire, in the frames this folder hands over. A tool call is
// only said whole (the router finishes its arguments first), the incremental
// updates behind it are display material for a screen that has none here, and
// citations wait for a screen that reads them.
void frame_of(const as::StreamEvent &event, std::vector<anthropic_frame> &out) {
  using frame = anthropic_frame;
  if (const auto *text = std::get_if<as::TextChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::text, .text = text->text});
  } else if (const auto *thought = std::get_if<as::ThoughtChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::thought, .text = thought->thought});
  } else if (const auto *usage = std::get_if<as::UsageUpdate>(&event)) {
    // The wire counts in ints and can name a negative one; a count of tokens
    // held as unsigned stops at zero, as the counters elsewhere do.
    const int input = usage->usage.input_token_count.value_or(0);
    const int output = usage->usage.output_token_count.value_or(0);
    out.push_back(frame{.what = frame::kind::usage,
                        .input_tokens = static_cast<std::uint32_t>(input > 0 ? input : 0),
                        .output_tokens = static_cast<std::uint32_t>(output > 0 ? output : 0)});
  } else if (const auto *call = std::get_if<as::ToolCallRequest>(&event)) {
    out.push_back(frame{.what = frame::kind::tool,
                        .id = call->id,
                        .name = call->name,
                        .arguments = call->arguments});
  } else if (const auto *update = std::get_if<as::ToolCallUpdate>(&event)) {
    // A call its arguments are still arriving for: named, and as much of its
    // arguments as the wire has sent. The whole call comes as a request when
    // the block stops; this says what stood in between. Where the wire gave no
    // id, the stream key is what it is called by, the way the router's own
    // completion names it.
    out.push_back(frame{.what = frame::kind::tool_partial,
                        .id = update->id.value_or(update->stream_key),
                        .name = update->name,
                        .arguments = update->arguments});
  } else if (const auto *failure = std::get_if<as::ErrorEvent>(&event)) {
    out.push_back(frame{.what = frame::kind::failed,
                        .text = error_text(failure->error),
                        .why_failed = failure_of(failure->error)});
  }
  // as::CitationUpdate: display material for a screen that reads citations,
  // which this tree has not got yet.
}

}  // namespace

anthropic_state *anthropic_open() {
  anthropic_state *state = new anthropic_state();
  state->router = new router_box();
  return state;
}

void anthropic_close(anthropic_state *state) {
  if (!state)
    return;
  delete static_cast<router_box *>(state->router);
  delete state;
}

std::vector<anthropic_frame> anthropic_step(anthropic_state *state, std::string_view payload) {
  using frame = anthropic_frame;
  std::vector<anthropic_frame> out;
  if (state == nullptr || state->router == nullptr)
    return out;
  router_box &reader = *static_cast<router_box *>(state->router);
  as::DecodeFailure failure;
  const std::optional<as::AnthropicStreamEvent> decoded =
      as::decode_stream_event(payload, &failure);
  if (!decoded) {
    // What the router keeps of a line its events would not read: said as the
    // failure it is, once, in the words the wire gave it -- the same as the
    // shaped wire beside this one, so both folders speak a bad line alike.
    reader.router.capture_parse_error(std::string(payload), failure.message);
    if (reader.router.stream_error && !reader.failed_emitted) {
      reader.failed_emitted = true;
      out.push_back(frame{.what = frame::kind::failed,
                          .text = error_text(*reader.router.stream_error),
                          .why_failed = failure_of(*reader.router.stream_error)});
    }
    return out;
  }
  for (const as::StreamEvent &event : reader.router.route(*decoded))
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
  if (reader.router.saw_terminal_marker() && !reader.ended_emitted) {
    reader.ended_emitted = true;
    out.push_back(frame{.what = frame::kind::ended,
                        .stop_reason = reader.router.stop_reason.value_or("stop")});
  }
  // The wire ended before a tool name was complete: said as the failure it is,
  // the way the wire's own client ends a stream. The router keeps it on the
  // stream itself; no event comes back, so the failure is read after it.
  reader.router.report_incomplete_blocks();
  if (reader.router.stream_error && !reader.failed_emitted) {
    reader.failed_emitted = true;
    out.push_back(frame{.what = frame::kind::failed,
                        .text = error_text(*reader.router.stream_error),
                        .why_failed = failure_of(*reader.router.stream_error)});
  }
  return out;
}

long long pchat_anthropic_ceiling(const char *model) {
  const auto ceiling = agora::core::anthropic::max_output_tokens(model != nullptr ? model : "");
  return ceiling.value_or(32768);
}

}  // namespace pchat::provider
