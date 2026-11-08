// SPDX-License-Identifier: AGPL-3.0-only
// The http policy's own parts: the port's client policy and its cleartext
// guard, reached only from this file, so that its headers stay out of module
// units as they do everywhere else in this program (src/tokens_worth.cpp, the
// same rule).
#include "wire.h"

#include "agora/core/http_policy.hpp"

namespace pchat::provider {

http_policy client_policy() {
  const agora::core::http_policy::ClientPolicy one =
      agora::core::http_policy::client_policy();
  return http_policy{.connect_seconds = one.connect_timeout_seconds,
                     .read_seconds = one.read_timeout_seconds,
                     .write_seconds = one.write_timeout_seconds,
                     .retry_on_connection_failure = one.retry_on_connection_failure,
                     .follow_redirects = one.follow_redirects,
                     .follow_ssl_redirects = one.follow_ssl_redirects};
}

bool cleartext_is_safe(
    std::string_view url,
    const std::vector<std::pair<std::string, std::string>> &headers) {
  try {
    agora::core::http_policy::guard_cleartext_credentials(url, headers);
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool local_host(std::string_view host) {
  return agora::core::http_policy::is_local_host(host);
}

}  // namespace pchat::provider
