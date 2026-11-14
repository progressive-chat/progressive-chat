// SPDX-License-Identifier: AGPL-3.0-only
// The program's own door, behind one plain header: the ported HTTP server
// (ports/agora-core src/server) on the loopback, answering this program's own
// wire. The answer is canned -- one delta, one cost, the end marker -- because
// what the round proves is that the address answers and the wire reads it:
// a real model behind this door is the back end's own work, not a fixture's.
//
// agora's headers are read here and nowhere else.
#include "serve.h"

// The ported server is built for POSIX hosts only: a page and a phone keep
// their own answering (a page's service worker, nothing on a phone), and where
// this build is one of those, no door stands and the program reaches whatever
// is behind its base address as it always did.
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)

#include <memory>
#include <string>

#include "agora/server/http_server.hpp"

namespace {

// What one answer is made of, as the wire's own frames: a whole sentence
// handed over at once, what it cost, and the marker that ends the stream.
// The shape the client reads (src/provider/stream.cc) is the shape every test
// in this tree already writes by hand.
constexpr std::string_view one_answer =
    "data: {\"choices\":[{\"delta\":{\"content\":\"The door is answered by this "
    "program itself.\"},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"stop\"}],\"usage\":"
    "{\"prompt_tokens\":12,\"completion_tokens\":9}}\n"
    "\n"
    "data: [DONE]\n"
    "\n";

// The ported server takes the threading with it: it accepts and answers there,
// the window's thread none the wiser.
// The ported server takes its own thread (accept and one per connection), so
// standing is the door being made and stopped -- nothing of it to run here.
std::unique_ptr<agora::server::HttpServer> the_door;

// Every wire this program speaks through its own base: the path after '/v1'
// is where each family puts its questions, and this door answers the ones it
// has an answer for.
[[nodiscard]] bool is_own_wire(const std::string &path) {
  return path == "/v1/chat/completions" || path == "/v1/responses" ||
         path == "/v1/messages";
}

agora::server::HttpResponse answer(const agora::server::HttpRequest &asked) {
  if (asked.method != "POST" || !is_own_wire(asked.path()))
    return agora::server::HttpResponse{.status = 404,
                                       .content_type = "application/json",
                                       .has_body = true,
                                       .body = R"({"error":{"message":"this program's door answers its own wire at /v1/chat/completions, /v1/responses and /v1/messages"}})"};
  // The body is read and set aside: what the round proved is that it arrived.
  if (asked.read_body && asked.content_length && *asked.content_length > 0)
    (void)asked.read_body(static_cast<std::size_t>(*asked.content_length));
  return agora::server::HttpResponse{
      .status = 200,
      .content_type = "text/event-stream",
      .has_body = true,
      .body = std::string{one_answer}};
}

}  // namespace

namespace pchat::serve {

bool up() {
  if (the_door)
    return true;
  // The loopback only: this door is the program's own, opened for the program
  // to reach, and not for the room. Port 0 asks for the first one there is.
  try {
    the_door = std::make_unique<agora::server::HttpServer>(
        "127.0.0.1", 0, "progressive-chat/1", &answer);
    the_door->start();
    return true;
  } catch (const std::exception &) {
    // Where the ported server cannot stand (a host without its sockets, a
    // build that kept it out), no door: the program goes on without one.
    the_door.reset();
    return false;
  }
}

bool is_up() { return the_door != nullptr; }

int port() {
  if (!the_door)
    return 0;
  return the_door->bound_port();
}

void down() {
  if (!the_door)
    return;
  the_door->stop();
  the_door.reset();
}

}  // namespace pchat::serve

#else  // the ported server's platforms

namespace pchat::serve {

bool up() { return false; }

bool is_up() { return false; }

int port() { return 0; }

void down() {}

}  // namespace pchat::serve

#endif
