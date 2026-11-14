// SPDX-License-Identifier: AGPL-3.0-only
// The retry decision, on Agora's own rules: this file is the only place the
// ported headers are named, and what it answers is the plain shape retry.h
// says. ProviderRetryPolicy holds what to ask again and how long to wait; it
// reads a GenerationError, so the failure this tree carries is rebuilt into
// one here and the answer taken back out as a bool and a count of
// milliseconds.
#include "retry.h"

#include <utility>

#include "agora/core/stream_normalization.hpp"

namespace pchat::provider {
namespace sn = agora::core::stream_normalization;

namespace {

// What this tree keeps of a failure, said the way the policy reads it. The
// kind is Agora's own alternative, named for the one it stands for; what of
// code, type and message came with the failure goes in as it came.
[[nodiscard]] sn::GenerationError as_error(const failure &one) {
  switch (one.kind) {
    case failure_kind::network:
      return sn::ErrorNetwork{one.status, one.message};
    case failure_kind::api:
      return sn::ErrorApi{one.code.empty() ? std::nullopt
                                           : std::optional(one.code),
                          one.type.empty() ? std::nullopt
                                           : std::optional(one.type),
                          one.message};
    case failure_kind::line:
      return sn::ErrorSseParse{one.message, one.message};
    case failure_kind::malformed_call:
      return sn::ErrorMalformedToolCall{one.message};
    case failure_kind::unfinished:
      return sn::ErrorIncompleteStream{"", std::nullopt, false, false};
    case failure_kind::truncated:
      return sn::ErrorOutputTruncated{"", std::nullopt};
    case failure_kind::timed_out:
      return sn::ErrorTimeout{};
  }
  return sn::ErrorApi{std::nullopt, std::nullopt, one.message};
}

}  // namespace

bool worth_asking_again(const failure &one) {
  // Nothing behind the failure: not a status, not a code, not a word -- no
  // reason to ask again. A refusal with only its status still has its status
  // to read.
  if (one.message.empty() && one.code.empty() && one.status == 0)
    return false;
  const sn::GenerationError error = as_error(one);
  return sn::ProviderRetryPolicy::should_retry_stream_error(&error);
}

// The same decision Agora makes on a whole answer's end: rebuild its
// StreamTermination from what this tree saw and ask the ported method. The
// fields not set here are the ones this tree has no source for -- a timeout
// is named by no transport we carry -- and the policy reads them as false.
[[nodiscard]] bool worth_asking_again(const termination &one) {
  sn::StreamTermination end;
  end.saw_terminal_marker = one.saw_terminal_marker;
  if (!one.stop_reason.empty())
    end.stop_reason = one.stop_reason;
  end.produced_content = one.produced_content;
  end.tool_call_in_flight = one.tool_call_in_flight;
  end.already_reported_error = one.already_reported_error;
  end.timed_out = one.timed_out;
  return end.is_retryable();
}

std::int64_t wait_before_again(int attempt) {
  // The policy counts its retries from one and throws outside its own five;
  // a sixth ask is one it would not wait for, and is not made.
  if (attempt < 1 || attempt > sn::ProviderRetryPolicy::kMaxRetries)
    return 0;
  return sn::ProviderRetryPolicy::delay_millis(attempt);
}

}  // namespace pchat::provider
