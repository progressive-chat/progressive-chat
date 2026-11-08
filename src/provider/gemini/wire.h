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
#include <string_view>
#include <vector>

namespace pchat::provider {

// One decoded frame of a gemini answer: text or thought as it arrives, a tool
// call whole, what it cost with input and output kept apart, why it stopped, or
// why it failed -- the same seven things every wire's frames say here, in the
// words the reader already knows.
struct gemini_frame {
  enum class kind { text, thought, tool, usage, ended, failed } what = kind::text;
  // The piece of text or of thought, or -- where it failed -- why.
  std::string said;
  // Gemini names a call where Anthropic gives it an id: an id is left empty
  // here rather than invented, and the name and its arguments stand alone.
  std::string call_id;
  std::string call_name;
  std::string call_arguments;
  std::uint32_t used_in = 0;
  std::uint32_t used_out = 0;
  // Why it stopped, as the wire said it: empty only where it did not say.
  std::string finish;
};

// The reader, one per stream: the wire's own router held behind a pointer so
// that this header stays something a module unit may name.
struct gemini_state {
  void *router = nullptr;
};

gemini_state *gemini_open();
void gemini_close(gemini_state *state);

// One payload -- whatever the `data:` lines of one frame said -- read whole:
// what the wire's events decided, and the end of it where it said there was one.
std::vector<gemini_frame> gemini_step(gemini_state *state, std::string_view payload);

}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_GEMINI_WIRE_H
