// SPDX-License-Identifier: AGPL-3.0-only
// The program's own door, answered: brought up, asked the way the client asks
// (a POST to /v1/chat/completions), and the words read back whole -- one
// round through the ported server, with no provider and no network beyond
// the loopback.
module;
#include "tests.h"
#include "../src/serve.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

export module pchat.test.serve;
import std;

namespace {

// One question, sent whole to the door, and everything it answers read back.
[[nodiscard]] std::string ask_door(int port) {
  const int socket_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0)
    return {};
  sockaddr_in there{};
  there.sin_family = AF_INET;
  there.sin_port = htons(static_cast<std::uint16_t>(port));
  ::inet_pton(AF_INET, "127.0.0.1", &there.sin_addr);
  if (::connect(socket_fd, reinterpret_cast<sockaddr *>(&there),
                sizeof(there)) != 0) {
    ::close(socket_fd);
    return {};
  }
  const std::string body =
      R"({"model":"haiku-4","messages":[{"role":"user","content":"hi"}]})";
  const std::string head =
      "POST /v1/chat/completions HTTP/1.1\r\nHost: 127.0.0.1\r\n"
      "Content-Type: application/json\r\nContent-Length: " +
      std::to_string(body.size()) + "\r\n\r\n";
  const std::string all = head + body;
  if (::write(socket_fd, all.data(), all.size()) < 0) {
    ::close(socket_fd);
    return {};
  }
  std::string got;
  char piece[4096];
  for (;;) {
    const ssize_t read_some = ::read(socket_fd, piece, sizeof(piece));
    if (read_some <= 0)
      break;
    got.append(piece, static_cast<std::size_t>(read_some));
  }
  ::close(socket_fd);
  return got;
}

}  // namespace

SAID(serve, the_programs_own_door_answers_its_own_wire) {
  EXPECT_TRUE(pchat::serve::up());
  if (!pchat::serve::is_up())
    return;
  EXPECT_TRUE(pchat::serve::port() > 0);
  const std::string answered = ask_door(pchat::serve::port());
  // Whole: the status line the wire reads its failures from, and the three
  // frames every test in this tree writes by hand -- here read back over a
  // socket from the ported server.
  EXPECT_TRUE(answered.find("200 OK") != std::string::npos);
  EXPECT_TRUE(answered.find("text/event-stream") != std::string::npos);
  EXPECT_TRUE(answered.find("delta") != std::string::npos);
  EXPECT_TRUE(answered.find("\"finish_reason\":\"stop\"") != std::string::npos);
  EXPECT_TRUE(answered.find("data: [DONE]") != std::string::npos);
  pchat::serve::down();
  EXPECT_FALSE(pchat::serve::is_up());
}

int main() { return pchat::tests::run(); }
