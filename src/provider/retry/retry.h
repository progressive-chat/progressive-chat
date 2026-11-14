// SPDX-License-Identifier: AGPL-3.0-only
// Whether a failed answer is worth asking again, and how long to wait before
// asking. The decision itself is Agora's (ports/agora-core
// stream_normalization: ProviderRetryPolicy); what crosses into a module unit
// is the plain shape of a failure and two answers, so no module unit sees the
// ported headers.
//
// A failure is said the way Agora's own Error says it: one of seven kinds, and
// what of code, type and message the wire gave it. The kind decides; the
// words are what the decision reads when the kind alone is not enough.
#ifndef PCHAT_PROVIDER_RETRY_H
#define PCHAT_PROVIDER_RETRY_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pchat::provider {

// How an answer failed, named the way Agora names the same seven: a network
// that did not answer, a provider that answered with an error, a line that
// would not read, a tool call that was malformed, a stream that stopped
// without its end, an answer cut by the token cap, or one that timed out.
enum class failure_kind {
  network,
  api,
  line,
  malformed_call,
  unfinished,
  truncated,
  timed_out,
};

// One failure, in the words the wire gave it: the kind above, and what of
// code, type and message came with it. Empty where the wire named none.
struct failure {
  failure_kind kind = failure_kind::api;
  std::string code;
  std::string type;
  std::string message;
  int status = 0;
};

// How an answer ended, the way Agora's own StreamTermination says it: whether
// the wire named its end, what it named it, whether anything of the answer
// reached the screen, whether a tool call is still open, whether the failure
// was already named once, and whether it timed out. This tree has no source for
// a timeout (no transport names one), so timed_out is always false here.
struct termination {
  bool saw_terminal_marker = false;
  std::string stop_reason;
  bool produced_content = false;
  bool tool_call_in_flight = false;
  bool already_reported_error = false;
  bool timed_out = false;
};

// Whether the answer may be asked again: Agora's decision, on this failure.
// A failure with nothing behind it is not asked again.
[[nodiscard]] bool worth_asking_again(const failure &one);

// The same question, on how an answer ended rather than why it failed: Agora
// reads it from StreamTermination, which is the one path that sees a stream
// cut short by the transport -- no end named, so it is asked again. A stream
// that named its end is not.
[[nodiscard]] bool worth_asking_again(const termination &one);

// How long to wait before asking again, in milliseconds: Agora's backoff for
// the given attempt, from one up to the attempts it allows. Zero where the
// attempt is not one it would wait for.
[[nodiscard]] std::int64_t wait_before_again(int attempt);

}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_RETRY_H
