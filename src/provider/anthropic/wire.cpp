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
#include "agora/core/thinking_capability.hpp"

namespace pchat::provider {
namespace {

namespace as = agora::core::anthropic_stream;
namespace sn = agora::core::stream_normalization;

// The reader: the wire's own router, and whether it has already said the end
// of the stream (the router proves an end but emits no event of it).
struct router_box {
  as::AnthropicStreamEventRouter router;
  bool ended_emitted = false;
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

// One event of the wire, in the frames this folder hands over. A tool call is
// only said whole (the router finishes its arguments first), the incremental
// updates behind it are display material for a screen that has none here, and
// citations wait for a screen that reads them.
void frame_of(const as::StreamEvent &event, std::vector<anthropic_frame> &out) {
  using frame = anthropic_frame;
  if (const auto *text = std::get_if<as::TextChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::text, .said = text->text});
  } else if (const auto *thought = std::get_if<as::ThoughtChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::thought, .said = thought->thought});
  } else if (const auto *usage = std::get_if<as::UsageUpdate>(&event)) {
    // The wire counts in ints and can name a negative one; a count of tokens
    // held as unsigned stops at zero, as the counters elsewhere do.
    const int input = usage->usage.input_token_count.value_or(0);
    const int output = usage->usage.output_token_count.value_or(0);
    out.push_back(frame{.what = frame::kind::usage,
                        .used_in = static_cast<std::uint32_t>(input > 0 ? input : 0),
                        .used_out = static_cast<std::uint32_t>(output > 0 ? output : 0)});
  } else if (const auto *call = std::get_if<as::ToolCallRequest>(&event)) {
    out.push_back(frame{.what = frame::kind::tool,
                        .call_id = call->id,
                        .call_name = call->name,
                        .call_arguments = call->arguments});
  } else if (const auto *failure = std::get_if<as::ErrorEvent>(&event)) {
    out.push_back(frame{.what = frame::kind::failed, .said = error_text(failure->error)});
  }
  // as::ToolCallUpdate, as::CitationUpdate: nothing to say here yet.
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
  const std::optional<as::AnthropicStreamEvent> decoded = as::decode_stream_event(payload);
  if (!decoded) {
    out.push_back(frame{.what = frame::kind::failed,
                        .said = "a frame the wire's own events would not read"});
    return out;
  }
  for (const as::StreamEvent &event : reader.router.route(*decoded))
    frame_of(event, out);
  // What the router kept instead of making a frame of it: a failure the
  // provider reported inside the stream is captured rather than emitted
  // (anthropic_stream.hpp, `stream_error`), the first one winning, and left to
  // whoever reads it -- said here as the failure it is, and once, through the
  // router's own word that it has been said.
  if (reader.router.stream_error && !reader.router.reported_error) {
    reader.router.reported_error = true;
    out.push_back(
        frame{.what = frame::kind::failed,
              .said = error_text(*reader.router.stream_error)});
  }
  if (reader.router.saw_terminal_marker() && !reader.ended_emitted) {
    reader.ended_emitted = true;
    out.push_back(frame{.what = frame::kind::ended,
                        .finish = reader.router.stop_reason.value_or("stop")});
  }
  return out;
}

long long pchat_anthropic_ceiling(const char *model) {
  const auto ceiling = agora::core::anthropic::max_output_tokens(model != nullptr ? model : "");
  return ceiling.value_or(32768);
}

anthropic_thinking_cap pchat_anthropic_thinking(const char *model) {
  namespace tc = agora::core::thinking_capability;
  const tc::Capability capability =
      tc::for_model(tc::ThinkingProviderFamily::Anthropic, model != nullptr ? model : "");
  const bool sampling = capability.supports_sampling_params;
  // Whether this model takes a thinking parameter at all, asked the way Agora
  // asks it: a generation that predates extended thinking is given no
  // parameter of it, and nothing about the answer blocks a request.
  if (!capability.supports_effort() && !capability.supports_thinking_budget)
    return {"", 0, sampling};
  // A budget beside it, where this model takes one: what Agora keeps as its
  // default when a reader has said no number of their own, kept to what this
  // model allows -- never under 1024, which is the floor the wire itself
  // holds an enabled thinking to.
  if (capability.supports_thinking_budget)
    return {"enabled", tc::clamp_budget(capability, tc::kDefaultBudgetTokens), sampling};
  // And the adaptive form for a model that offers an effort to choose by and
  // no budget of its own to think with: which is the form no budget may be
  // said beside.
  if (capability.supports_effort())
    return {"adaptive", 0, sampling};
  return {"enabled", tc::clamp_budget(capability, tc::kDefaultBudgetTokens), sampling};
}

}  // namespace pchat::provider
