// SPDX-License-Identifier: AGPL-3.0-only
// The fitting itself, in the classic translation unit where Agora's own
// headers are at home: what is decided here is which messages a wire is given,
// decided with Agora's ContextBudget and its ContextLimiter over what this
// program counts a message with -- and none of agora's headers is ever seen by
// a module unit, which is the same arrangement as src/tokens_worth.cpp and
// src/provider/http/wire.cpp.
#include "provider/budget.h"

#include <algorithm>
#include <optional>

#include "agora/core/context_budget.hpp"
#include "agora/core/message_preparation.hpp"
#include "agora/core/openai_request.hpp"
#include "tokens_worth.h"

namespace pchat::provider {
namespace {

namespace context = agora::core::context_budget;
namespace prep = agora::core::prep;

// What one piece of text is worth, priced the way this program counts it for
// the circle in the foot: the carried o200k vocabulary, handed in once where
// the program starts (src/app/program.cc), and the character-class heuristic
// until then.
[[nodiscard]] std::int64_t counted(std::string_view text) {
  return ::pchat::tokens_worth(text.data(), text.size());
}

// What the messages are, in the words the preparation reads them in: one row
// per message of the conversation, in the order it was said.
[[nodiscard]] std::vector<prep::ChatMessage> as_prep(std::span<const budgeted> said) {
  std::vector<prep::ChatMessage> out;
  out.reserve(said.size());
  for (const budgeted &one : said) {
    prep::ChatMessage each;
    each.id.assign(one.id);
    each.text.assign(one.text);
    each.participant = one.outgoing ? prep::Participant::User : prep::Participant::Model;
    // One word per picture, for the estimator to price: a user message's
    // images are each worth the model's own image cost, and without
    // dimensions anywhere in this tree that is the reservation every family
    // falls back to -- the same 1024 a picture adds to the message's own
    // count where it is written (pchat.logic.composer, image_window_cost).
    for (std::uint32_t at = 0u; at < one.images; ++at)
      each.images.push_back("image");
    out.push_back(std::move(each));
  }
  return out;
}

// What the tools are, in the words their cost is counted in: the shape a chat
// wire is given them in (src/provider.cc, body_of), where the schema is the
// one this program writes and not one a provider was handed from elsewhere.
[[nodiscard]] std::vector<agora::core::openai::ToolDefinition> as_prep(
    std::span<const budgeted_tool> tools) {
  std::vector<agora::core::openai::ToolDefinition> out;
  out.reserve(tools.size());
  for (const budgeted_tool &one : tools) {
    agora::core::openai::ToolDefinition each;
    each.type = "function";
    each.function.name = one.name;
    each.function.description = one.told;
    each.function.parameters.type = "object";
    for (const auto &[takes, said] : one.taken) {
      agora::core::openai::ToolProperty property;
      property.type = "string";
      property.description = said;
      each.function.parameters.properties.push_back({takes, std::move(property)});
      each.function.parameters.required.push_back(takes);
    }
    out.push_back(std::move(each));
  }
  return out;
}

}  // namespace

std::string budget_fit(std::span<const budgeted> said,
                       std::span<const budgeted_tool> tools,
                       std::uint32_t window, std::string_view told,
                       std::string_view named) {
  if (said.empty())
    return std::string{};
  // No window says nothing about how much room there is: where nobody said,
  // nothing is cut, and a conversation of a model this program knows no
  // window of is asked about whole, as it was before.
  if (window == 0)
    return std::string(said.front().id);

  // The budget as Agora sets it at every read of one: a number kept from an
  // older build, a default where none was set, and the same bounds everywhere.
  const int room = context::normalize(static_cast<int>(window));
  const prep::ContextCostModel costs = prep::cost_model_for(
      prep::parse_prefixed_model_id(std::string(named)), counted);
  const prep::CostModelContextEstimator prices(costs);
  // What stands ahead of the messages comes off the window before they are
  // counted against it -- what the model is told, and the tools it may ask
  // for on the way, priced as Agora prices the same two before it limits.
  const std::optional<std::string> prompt =
      told.empty() ? std::nullopt : std::optional<std::string>{std::string(told)};
  const int fixed =
      prices.estimate_fixed(prompt, as_prep(tools), std::nullopt, false, false, false);
  const int left = std::max(room - fixed, 1);

  // False, and said why: a thought of an earlier answer is not written back
  // to a wire in this tree, so it is not counted against the window either.
  // Said here rather than left to be found later, for whoever one is written
  // back to has to turn this over at the same time.
  const std::vector<prep::ChatMessage> kept =
      prep::limit_context(as_prep(said), left, /*include_assistant_reasoning=*/false, costs);
  // Not one message of it is from the reader, which is the only way nothing
  // is kept: the newest is asked about anyway, because a question is asked
  // with something in it, and a chat that never had a reader's word in it is
  // not a reason to ask a wire with nothing at all.
  if (kept.empty())
    return std::string(said.back().id);
  // The oldest that is kept, said in the conversation's own words: the
  // limiting keeps a run of the newest messages and gives up the ones before
  // them, so the oldest kept one is where the asking starts.
  const std::string_view oldest = kept.front().id;
  for (const budgeted &one : said)
    if (one.id == oldest)
      return std::string(one.id);
  return std::string(said.back().id);
}

}  // namespace pchat::provider
