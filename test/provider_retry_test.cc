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
using pchat::provider::termination;

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

// The status a server refused with, the way a transport reports it: a
// service that is down and a rate limit pass, a bad request does not -- the
// policy reads the status, not the sentence the trouble was written in.
SAID(provider_retry, a_status_the_policy_waits_out_is_asked_again) {
  const failure down{failure_kind::network, std::string(), std::string(),
                     "Service Unavailable", 503};
  EXPECT_TRUE(pchat::provider::worth_asking_again(down));
  const failure busy{failure_kind::network, std::string(), std::string(),
                     "Too Many Requests", 429};
  EXPECT_TRUE(pchat::provider::worth_asking_again(busy));
}

SAID(provider_retry, a_status_the_policy_refuses_is_not_asked_again) {
  const failure bad{failure_kind::network, std::string(), std::string(),
                    "Bad Request", 400};
  EXPECT_FALSE(pchat::provider::worth_asking_again(bad));
  const failure denied{failure_kind::network, std::string(), std::string(),
                       "Unauthorized", 401};
  EXPECT_FALSE(pchat::provider::worth_asking_again(denied));
}

// A stream the transport closed before the wire named its end: Agora reads it
// from its termination -- no terminal marker means it is asked again -- and a
// stream that named its end is not. The tool call still open and what reached
// the screen are what the policy weighs.
SAID(provider_retry, a_stream_cut_short_is_asked_again) {
  const termination cut{/*saw_terminal_marker*/ false, std::string(),
                        /*produced_content*/ true,
                        /*tool_call_in_flight*/ false,
                        /*already_reported_error*/ false,
                        /*timed_out*/ false};
  EXPECT_TRUE(pchat::provider::worth_asking_again(cut));
}

SAID(provider_retry, a_stream_that_named_its_end_is_not_asked_again) {
  const termination done{/*saw_terminal_marker*/ true, "stop",
                         /*produced_content*/ true, false, false, false};
  EXPECT_FALSE(pchat::provider::worth_asking_again(done));
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
