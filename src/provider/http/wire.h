// SPDX-License-Identifier: AGPL-3.0-only
// The HTTP client's own policy, said across the one line a module unit and a
// classic translation unit may share: the ceilings a carrying is built with,
// whether one that could not connect is worth asking again, what this program
// refuses to send in the clear, and which hosts it counts as this machine's
// own. The port's policy stands behind this file (ports/agora-core,
// http_policy); its headers stay on the classic side entirely -- the same rule
// the anthropic and gemini folders keep in their wire.h.
#ifndef PCHAT_PROVIDER_HTTP_WIRE_H
#define PCHAT_PROVIDER_HTTP_WIRE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pchat::provider {

// What one HTTP client is built with: the ceilings the Kotlin app gives its
// OkHttp (30 seconds to connect, five minutes to be answered, 30 to finish
// saying), whether a carrying that could not connect is asked again rather
// than failed at once, and whether a redirect is followed where it leads.
struct http_policy {
  std::int64_t connect_seconds = 0;
  std::int64_t read_seconds = 0;
  std::int64_t write_seconds = 0;
  bool retry_on_connection_failure = false;
  bool follow_redirects = false;
  bool follow_ssl_redirects = false;
};

// What this build's client is built with: the port's own client_policy(), in
// the words this side keeps its own in.
[[nodiscard]] http_policy client_policy();

// Whether a body carrying `headers` may go to `url` as it stands: false where
// an API credential would cross plain http:// to a host that is not one of
// this machine's own. The port fails closed with an exception where this
// program has none to throw at, and this says no where the port throws.
[[nodiscard]] bool cleartext_is_safe(
    std::string_view url,
    const std::vector<std::pair<std::string, std::string>> &headers);

// Whether a host is one this machine itself answers on: loopback, the LAN
// ranges, Tailscale's own, bare hostnames, and the neighbourhood names a home
// router gives out (.local, .lan, .home, .internal).
[[nodiscard]] bool local_host(std::string_view host);

}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_HTTP_WIRE_H
