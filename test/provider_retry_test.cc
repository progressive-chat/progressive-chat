// SPDX-License-Identifier: AGPL-3.0-only
// What is worth asking again, and how long to wait: Agora's own decision, on
// the failure this tree keeps. The kinds and the words are the ones a wire
// actually says -- a rate limit that will pass, a key that will not, a line
// that would not read -- and each answer is the one the ported policy gives.
module;
#include "tests.h"
#include "../src/provider/retry/retry.h"

export module pchat.test.provider_retry;
import std;
import pchat.provider;

namespace {

using pchat::provider::failure;
using pchat::provider::failure_kind;

// A failure a wire says is worth waiting out: the rate limit, the server that
// is down, the window that ran out.
[[nodiscard]] failure transient(const std::string &message) {
  return {failure_kind::api, std::string(), std::string(), message, 0};
}

}  // namespace

SAID(provider_retry, a_rate_limit_is_asked_again) {
  const failure one = transient("Rate limit reached for requests");
  EXPECT_TRUE(pchat::provider::worth_asking_again(one));
}

SAID(provider_retry, a_key_the_wire_refused_is_not_asked_again) {
  const failure one{failure_kind::api, "invalid_api_key",
                    "authentication_error", "The key was refused", 0};
  EXPECT_FALSE(pchat::provider::worth_asking_again(one));
}

SAID(provider_retry, a_line_that_would_not_read_is_asked_again) {
  const failure one{failure_kind::line, std::string(), std::string(),
                    "Malformed JSON payload", 0};
  EXPECT_TRUE(pchat::provider::worth_asking_again(one));
}

SAID(provider_retry, an_answer_that_timed_out_is_asked_again) {
  const failure one{failure_kind::timed_out, std::string(), std::string(),
                    "the answer timed out", 0};
  EXPECT_TRUE(pchat::provider::worth_asking_again(one));
}

SAID(provider_retry, a_failure_with_nothing_behind_it_is_not_asked_again) {
  EXPECT_FALSE(pchat::provider::worth_asking_again(failure{}));
}

// The waits Agora gives its own retries: one second and a half at most for the
// first ask after a failure, doubling with each, and nothing at all for a
// sixth -- the policy has five.
SAID(provider_retry, the_waits_are_the_ported_backoff) {
  EXPECT_GE(pchat::provider::wait_before_again(1), 500);
  EXPECT_GE(2000, pchat::provider::wait_before_again(1));
  EXPECT_GE(pchat::provider::wait_before_again(5), 8000);
  EXPECT_EQ(pchat::provider::wait_before_again(6), 0);
  EXPECT_EQ(pchat::provider::wait_before_again(0), 0);
}

int main() { return pchat::tests::run(); }
