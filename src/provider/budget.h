// SPDX-License-Identifier: AGPL-3.0-only
// What of a conversation fits the window its answerer has, said across the
// one line a module unit and a classic translation unit may share: Agora's
// own preparation stands behind this file (ports/agora-core, its ContextBudget
// and its ContextLimiter), and its headers stay on the classic side entirely,
// as the counters' do (src/tokens_worth.h).
#ifndef PCHAT_PROVIDER_BUDGET_H
#define PCHAT_PROVIDER_BUDGET_H

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pchat::provider {

// One message of a conversation, as the fitting reads it: what it said, who
// said it, and what it is called. Nothing else of it is read -- a thought of
// an earlier answer is not written back to a wire in this tree, so it is not
// priced here either.
struct budgeted {
  std::string_view id;
  std::string_view text;
  bool outgoing = false;
  // How many pictures went out with it: priced by the cost model of the
  // model asked, one reservation each while their dimensions stand nowhere
  // in this tree (pchat.provider/budget.cpp says which reservation that is).
  std::uint32_t images = 0;
};

// One tool the asking offers, as the fitting prices it: the same three the
// body says -- its name, what it is told it does, and what it takes.
struct budgeted_tool {
  std::string name;
  std::string told;
  std::vector<std::pair<std::string, std::string>> taken;
};

// What one call's asking is given to a wire as: an object, which is what
// every call this program answers itself carries -- and where the asking was
// written down without one (a sample's, an older record's), there is nothing
// honest to send but no arguments at all. Both halves of the asking say it
// here: the turns that are written out of it (src/provider.cc, `turns_of`)
// and the words this fitting counts against the window.
[[nodiscard]] inline std::string call_arguments(std::string_view asked) {
  return (!asked.empty() && asked.front() == '{') ? std::string(asked)
                                                  : std::string("{}");
}

// The oldest message of what was said that is worth asking about: every
// message from it on is asked about, and none before it -- Agora's
// limitContext, the newest protocol-complete suffix that fits the window,
// with what stands ahead of the messages (what the model is told, and the
// tools it may ask for on the way) taken off the window first.
//
// Said as the id of a message rather than as a count, so that whoever holds
// the conversation may say "from this one on" over a range it has read
// already. The newest message's own id where not one of it fits, because a
// question is asked with something in it; the first message's own id where
// nothing said how much room there is to fit into; and nothing at all where
// nothing was said.
//
// `named` is the model in the words Agora names one for its cost models -- a
// provider and its model with a colon between, "anthropic:sonnet-4" -- so that
// a Claude is priced with Claude's envelope and a model nobody knows of with
// the plain one.
std::string budget_fit(std::span<const budgeted> said,
                       std::span<const budgeted_tool> tools,
                       std::uint32_t window, std::string_view told,
                       std::string_view named);

}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_BUDGET_H
