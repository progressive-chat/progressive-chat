// SPDX-License-Identifier: AGPL-3.0-only
// The gemini wire's own parts, said across the one line a module unit and a
// classic translation unit may share: the frames its stream decodes into and
// the reader that turns one payload of them into them.
//
// Agora's router stands behind this file (ports/agora-core); its headers stay
// on the classic side entirely. What crosses into a module unit is only these
// plain types -- std includes in a global module fragment are what every test
// unit in this tree already does with tests.h.
#ifndef PCHAT_PROVIDER_GEMINI_WIRE_H
#define PCHAT_PROVIDER_GEMINI_WIRE_H

#include <cstdint>
#include <string>

#include "../citation.h"
#include "../retry/retry.h"
#include <string_view>
#include <vector>

namespace pchat::provider {

// One decoded frame of a gemini answer: text or thought as it arrives, a tool
// call whole, what it cost with input and output kept apart, why it stopped, or
// why it failed -- the same seven things every wire's frames say here, in the
// words the reader already knows.
struct gemini_frame {
  enum class kind { text, thought, tool, tool_partial, citation, usage, ended, failed } what = kind::text;
  // The piece of text or of thought, or -- where it failed -- why.
  std::string text;
  // Gemini names a call where Anthropic gives it an id: an id is left empty
  // here rather than invented, and the name and its arguments stand alone.
  std::string id;
  std::string name;
  std::string arguments;
  std::uint32_t input_tokens = 0;
  std::uint32_t output_tokens = 0;
  // Why it stopped, as the wire said it: empty only where it did not say.
  std::string stop_reason;
  // Why it failed, the way Agora says it -- the kind of it and what came with
  // it -- set only on a frame whose kind is failed.
  failure why_failed;
  // One source this answer cited, set only on a frame whose kind is citation.
  citation cited;
  // Whether this event carries what the model said, by the ported rule
  // (carries_model_output): a text or thought that said something, or any
  // call. False for usage, a citation and an error. Read at the stream's end
  // to say whether anything of the answer reached the screen.
  bool carried = false;
};

// The reader, one per stream: the wire's own router held behind a pointer so
// that this header stays something a module unit may name.
struct gemini_state {
  void *router = nullptr;
};

gemini_state *gemini_open();
void gemini_close(gemini_state *state);

// One payload, the `data:` lines of one frame, read whole: the events the
// wire decided, and its end where it said there was one.
std::vector<gemini_frame> gemini_step(gemini_state *state, std::string_view payload);

}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_GEMINI_WIRE_H
