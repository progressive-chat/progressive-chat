module;
#include "tests.h"
#include "provider/http/wire.h"
export module pchat.test.provider_http;
// SPDX-License-Identifier: AGPL-3.0-only
// The http policy the folder keeps: the ceilings a client is built with, what
// this program will not send in the clear, and which hosts count as this
// machine's own -- said through wire.h, with the port's own headers kept on the
// classic side of it.
import std;

using namespace pchat;

SAID(http, the_client_is_built_with_the_port_s_ceilings) {
  const provider::http_policy one = provider::client_policy();
  EXPECT_EQ(one.connect_seconds, 30);
  EXPECT_EQ(one.read_seconds, 300);
  EXPECT_EQ(one.write_seconds, 30);
  EXPECT_TRUE(one.retry_on_connection_failure);
  EXPECT_TRUE(one.follow_redirects);
  EXPECT_TRUE(one.follow_ssl_redirects);
}

SAID(http, a_credential_may_not_cross_in_the_clear) {
  const std::vector<std::pair<std::string, std::string>> key{
      {"x-api-key", "secret"}};
  const std::vector<std::pair<std::string, std::string>> bearer{
      {"authorization", "Bearer secret"}};
  // Encrypted, or nothing of a credential in the asking: both go.
  EXPECT_TRUE(provider::cleartext_is_safe("https://api.example/v1", key));
  EXPECT_TRUE(provider::cleartext_is_safe("http://api.example/v1", {}));
  // A credential over plain http:// to a host that is not this machine's:
  // refused, whichever of the four names the wire puts it under.
  EXPECT_FALSE(provider::cleartext_is_safe("http://api.example/v1", key));
  EXPECT_FALSE(provider::cleartext_is_safe("http://api.example/v1", bearer));
  // The same refusal does not reach this machine's own hosts: a key for a
  // model running at home is how this program is meant to speak to it.
  EXPECT_TRUE(provider::cleartext_is_safe("http://127.0.0.1:11434/v1", key));
  EXPECT_TRUE(provider::cleartext_is_safe("http://ollama.local/v1", key));
}

SAID(http, which_hosts_this_machine_counts_as_home) {
  EXPECT_TRUE(provider::local_host("localhost"));
  EXPECT_TRUE(provider::local_host("printer.local"));
  EXPECT_TRUE(provider::local_host("box.lan"));
  EXPECT_TRUE(provider::local_host("nas"));       // a bare LAN name
  EXPECT_TRUE(provider::local_host("192.168.1.7"));
  EXPECT_TRUE(provider::local_host("10.0.0.5"));
  EXPECT_TRUE(provider::local_host("127.0.0.1"));
  EXPECT_TRUE(provider::local_host("100.64.0.9"));  // Tailscale's own range
  EXPECT_FALSE(provider::local_host("api.example"));
  EXPECT_FALSE(provider::local_host("1.1.1.1"));
}

int main() { return pchat::tests::run(); }
