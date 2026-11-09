// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider.budget -- What of a conversation is asked about, and what is
// left out of the asking: the window an answerer has decides it, with Agora's
// own preparation standing behind this file (ports/agora-core), over what this
// program counts a message with. The oldest message that fits is said as an
// id, so that whoever asks may say "from this one on" over the conversation
// it holds already.
//
// Nothing here cuts a conversation: every message of it stays where it is, is
// drawn as it was, and is read whole by the tools -- only what goes onto the
// wire is decided here, which is the deciding Agora's limitContext does there.
module;
#include "provider/budget.h"

export module pchat.provider.budget;

import std;
import pchat.core;
import pchat.config;
import pchat.provider;

export namespace pchat::provider {

// The oldest message of what was said that is worth asking about: every
// message from it on is asked about, and none before it, and the newest one
// of it alone where not one of them fits. Where the conversation is empty, or
// where nothing said how much room its answerer has, every message of it is
// asked about -- which is what a question asked before this did too.
[[nodiscard]] inline message_id fit_from(std::span<const message> said,
                                          const endpoint &at,
                                          std::uint32_t window,
                                          const wanted &wants) {
  // What every message is worth as it goes out: a round of asking goes out
  // with its calls and their answers again (turns_of), so those words are
  // against the window too -- and in the words a wire takes them in, which
  // `call_arguments` says for the writing and for this counting both, so that
  // what is counted here and what is written there are the same words.
  std::vector<std::string> whole;
  whole.reserve(said.size());
  for (const message &one : said) {
    std::string words(one.text);
    if (!one.outgoing && one.before)
      for (const call_made &call : one.before->calls) {
        words += ' ';
        words += call_arguments(call.asked);
        words += ' ';
        words += call.answered;
      }
    whole.push_back(std::move(words));
  }
  std::vector<budgeted> seen;
  seen.reserve(said.size());
  for (std::size_t at = 0; at < said.size(); ++at)
    seen.push_back(budgeted{.id = said[at].id.value,
                            .text = whole[at],
                            .outgoing = said[at].outgoing});
  std::vector<budgeted_tool> kit;
  kit.reserve(wants.tools.size());
  for (const offered &one : wants.tools)
    kit.push_back(budgeted_tool{.name = one.name,
                                .told = one.told,
                                .taken = one.taken});
  // The model in the words a cost model is asked with: the family that says
  // which wire's words it reads -- who it is asked through, said before its
  // name with a colon between. A model of a family not known here is named
  // alone, and is priced as any model nobody has heard of.
  const std::string_view family = family_of(at.model);
  const std::string named = family.empty() ? std::string(at.model)
                                            : std::string(family) + ":" +
                                                  std::string(at.model);
  return message_id{budget_fit(seen, kit, window, at.told, named)};
}

}  // namespace pchat::provider
