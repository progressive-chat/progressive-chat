// SPDX-License-Identifier: AGPL-3.0-only
// Whether a failed answer is worth asking again, and how long to wait before
// asking. The decision itself is Agora's (ports/agora-core
// stream_normalization: ProviderRetryPolicy); what crosses into a module unit
// is the plain shape of a failure and two answers, so no module unit sees the
// ported headers.
//
// A failure is said the way Agora's own Error says it: one of seven kinds, and
// whatever of code, type and message the wire gave it. The kind decides; the
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

// Whether the answer may be asked again: Agora's decision, on this failure.
// A failure with nothing behind it is not asked again.
[[nodiscard]] bool worth_asking_again(const failure &one);

// How long to wait before asking again, in milliseconds: Agora's backoff for
// the given attempt, from one up to the attempts it allows. Zero where the
// attempt is not one it would wait for.
[[nodiscard]] std::int64_t wait_before_again(int attempt);

}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_RETRY_H
