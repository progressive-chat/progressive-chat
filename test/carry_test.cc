// SPDX-License-Identifier: AGPL-3.0-only
// The round a host can make: the program's own door brought up, one body
// handed to the carrying the platform gives a host, and the bytes it answers
// with read back -- the door and the wire in one pass, no page and no
// network beyond the loopback.
module;
#include "tests.h"
#include "../src/serve.h"

export module pchat.test.carry;
import std;
import pchat.platform.carry;

namespace {

// What every answer of this door is made of: the three frames the stream
// reads whole, asked of and answered on the loopback.
constexpr std::string_view question =
    R"({"model":"haiku-4","messages":[{"role":"user","content":"hi"}]})";

}  // namespace

SAID(carry, a_host_carries_its_own_door_and_reads_it_back) {
  pchat::serve::down();
  EXPECT_TRUE(pchat::serve::up());
  if (!pchat::serve::is_up())
    return;
  pchat::platform::carrying carried;
  EXPECT_TRUE(carried.any());
  if (!carried.any()) {
    pchat::serve::down();
    return;
  }
  const std::string place = "http://127.0.0.1:" +
                            std::to_string(pchat::serve::port()) +
                            "/v1/chat/completions";
  const std::vector<std::pair<std::string, std::string>> fields{
      {"Content-Type", "application/json"}};
  carried.begin(place, fields, question);
  EXPECT_TRUE(carried.over());
  EXPECT_TRUE(carried.status() == 0);  // a 200 keeps no refusal
  EXPECT_TRUE(carried.trouble() == std::nullopt);
  const std::string whole = carried.take();
  EXPECT_TRUE(whole.find("delta") != std::string::npos);
  EXPECT_TRUE(whole.find("data: [DONE]") != std::string::npos);
  // A path the door does not answer: the refusal read the way a refusal is
  // read anywhere -- its number and the body it was said in.
  pchat::platform::carrying refused;
  refused.begin("http://127.0.0.1:" + std::to_string(pchat::serve::port()) +
                    "/somewhere/else",
                fields, question);
  EXPECT_TRUE(refused.over());
  EXPECT_TRUE(refused.status() == 404);
  EXPECT_TRUE(refused.trouble().has_value());
  EXPECT_TRUE(refused.error_body().find("error") != std::string::npos);
  pchat::serve::down();
}

int main() { return pchat::tests::run(); }
