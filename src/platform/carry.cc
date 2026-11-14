module;
#include "../serve.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten/console.h>
#include <emscripten/fetch.h>
#else
#include <arpa/inet.h>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>
#endif
// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.carry -- One body handed to the platform the program is
// running on, and the bytes it answers with, taken back out of it.
//
// Nothing here knows what it carries or what it means: a place, its
// headers and a body go in, and what arrives comes back out in the
// sizes it arrived in. That is all a provider's stream needs, and all that
// differs between a page, a window and a phone.
//
// Only the page carries anything today, through the browser's own fetch, and
// everywhere else this says so (`any()`): everything not written for the
// page is built with an empty carrying in it, and asking it for bytes asks
// nobody anything. What a desk and a phone will carry with (Asio on
// fibers, as mux carries Matrix) is written here when it is written, and
// nothing above changes.
export module pchat.platform.carry;

import std;

export namespace pchat::platform {

// One thing being carried, then nothing.
class carrying {
public:
  carrying() = default;
  ~carrying() { shut(); }

  carrying(const carrying &) = delete;
  carrying &operator=(const carrying &) = delete;

  // Given away whole and never in two places at once: what the browser was
  // asked for is one request; closing it twice would close one twice.
  carrying(carrying &&gone)
      : place_(std::move(gone.place_)), wrote_(std::move(gone.wrote_)),
        fields_(std::move(gone.fields_)), flat_(std::move(gone.flat_)),
        arrived_(std::move(gone.arrived_)), trouble_(std::move(gone.trouble_)),
        over_(gone.over_), read_(gone.read_), status_(gone.status_),
        status_text_(std::move(gone.status_text_)),
        body_(std::move(gone.body_)) {
#if defined(__EMSCRIPTEN__)
    held_ = std::exchange(gone.held_, nullptr);
#endif
  }
  carrying &operator=(carrying &&gone) {
    if (this != &gone) {
      shut();
      place_ = std::move(gone.place_);
      wrote_ = std::move(gone.wrote_);
      fields_ = std::move(gone.fields_);
      flat_ = std::move(gone.flat_);
      arrived_ = std::move(gone.arrived_);
      trouble_ = std::move(gone.trouble_);
      over_ = gone.over_;
      status_ = gone.status_;
      status_text_ = std::move(gone.status_text_);
      body_ = std::move(gone.body_);
      read_ = gone.read_;
#if defined(__EMSCRIPTEN__)
      held_ = std::exchange(gone.held_, nullptr);
#endif
    }
    return *this;
  }

  // Whether this platform hands bytes over at all: everything that can be
  // asked of it is nothing where it cannot, so nothing else here has to
  // be a different program on each platform.
  [[nodiscard]] bool any() const {
#if defined(__EMSCRIPTEN__)
    return true;
#else
    // A host carries only where it knows the way: the program's own door, up
    // on the loopback (serve.h says its port). The asking side asks this
    // before it makes a request, so a host with no door asks nobody and says
    // nothing.
    return pchat::serve::port() > 0;
#endif
  }

  // Given the place, its headers and the body, and asked for nothing more.
  template <std::ranges::forward_range Headers>
    requires std::same_as<std::ranges::range_value_t<Headers>,
                          std::pair<std::string, std::string>>
  void begin(std::string_view url, Headers &&headers, std::string_view body) {
#if defined(__EMSCRIPTEN__)
    if (held_)
      return;
    for (const auto &[name, value] : headers) {
      fields_.push_back(name);
      fields_.push_back(value);
    }
    // The browser reads the names and the values where they were written and
    // keeps reading them until it is answered, so they are kept here whole,
    // not only for the length of this call.
    flat_.reserve(fields_.size() + 1);
    for (const std::string &one : fields_)
      flat_.push_back(one.c_str());
    flat_.push_back(nullptr);
    wrote_ = std::string(body);
    place_ = std::string(url);
    read_ = 0;

    emscripten_fetch_attr_t asking;
    emscripten_fetch_attr_init(&asking);
    std::strcpy(asking.requestMethod, "POST");
    asking.requestHeaders = flat_.data();
    asking.requestData = wrote_.data();
    asking.requestDataSize = wrote_.size();
    // Both: the whole of it is wanted even where a browser never hands it
    // over a piece at a time. Where it does, what answers arrives while
    // it is read.
    asking.attributes =
        EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_STREAM_DATA;
    asking.userData = this;
    asking.onprogress = &carrying::went_along;
    asking.onerror = &carrying::went_wrong;
    asking.onreadystatechange = &carrying::went_on;
    asking.onsuccess = &carrying::went_well;
    held_ = emscripten_fetch(&asking, place_.c_str());
    if (!held_) {
      trouble_ = "the page would not ask for it";
      over_ = true;
    }
#else
    // A host asks its own door whole: connect to the loopback, send the
    // request, and read the answer back -- status line, headers, then the
    // body the length names. One pass, because the door answers over a
    // connection it closes at the end of it.
    read_ = 0;
    place_ = std::string(url);
    wrote_ = std::string(body);
    const std::string where = place_;
    const std::size_t scheme = where.find("://");
    const std::string host_port =
        scheme == std::string::npos
            ? std::string{}
            : where.substr(scheme + 3, where.find('/', scheme + 3) - (scheme + 3));
    const std::string path = scheme == std::string::npos
                                 ? where
                                 : where.substr(where.find('/', scheme + 3));
    const std::size_t colon = host_port.rfind(':');
    const std::string host =
        colon == std::string::npos ? host_port : host_port.substr(0, colon);
    const int port = colon == std::string::npos
                         ? 80
                         : std::atoi(host_port.c_str() + colon + 1);
    if (host != "127.0.0.1" && host != "localhost") {
      // The only door a host knows the way to: its own, on the loopback.
      // Anything else would need a client this build does not carry, and
      // saying so is what the asking side reads.
      trouble_ = "this build carries its own door only";
      over_ = true;
      return;
    }
    const int socket_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in there{};
    there.sin_family = AF_INET;
    there.sin_port = htons(static_cast<std::uint16_t>(port));
    ::inet_pton(AF_INET, "127.0.0.1", &there.sin_addr);
    if (socket_fd < 0 ||
        ::connect(socket_fd, reinterpret_cast<sockaddr *>(&there),
                  sizeof(there)) != 0) {
      if (socket_fd >= 0)
        ::close(socket_fd);
      trouble_ = "the door did not answer the connection";
      over_ = true;
      return;
    }
    std::string flat_head = "POST " + path +
                            " HTTP/1.1\r\nHost: " + host_port +
                            "\r\nConnection: close\r\n";
    for (const auto &[name, value] : headers)
      flat_head += name + ": " + value + "\r\n";
    flat_head += "Content-Length: " + std::to_string(wrote_.size()) +
                 "\r\n\r\n";
    const std::string all = flat_head + wrote_;
    if (::send(socket_fd, all.data(), all.size(), 0) < 0) {
      ::close(socket_fd);
      trouble_ = "the door would not take the question";
      over_ = true;
      return;
    }
    std::string said;
    char piece[4096];
    for (;;) {
      const ssize_t got_some = ::read(socket_fd, piece, sizeof(piece));
      if (got_some <= 0)
        break;
      said.append(piece, static_cast<std::size_t>(got_some));
    }
    ::close(socket_fd);
    const std::size_t line_end = said.find("\r\n");
    const std::size_t head_end = said.find("\r\n\r\n");
    if (line_end == std::string::npos || head_end == std::string::npos) {
      trouble_ = "the door said nothing whole";
      over_ = true;
      return;
    }
    // "HTTP/1.0 200 OK" -- the number and the words beside it, kept the way
    // the page keeps them: a refusal read the same either side.
    const std::size_t space = said.find(' ');
    const std::size_t second = space == std::string::npos
                                   ? std::string::npos
                                   : said.find(' ', space + 1);
    const std::string status_line =
        space == std::string::npos ? std::string{} : said.substr(0, space);
    const std::string status_words =
        second == std::string::npos ? std::string{}
                                    : said.substr(second + 1,
                                                  line_end - second - 1);
    const int status = space == std::string::npos
                           ? 0
                           : std::atoi(said.c_str() + space + 1);
    const std::string heads = said.substr(0, head_end);
    const std::string rest = said.substr(head_end + 4);
    std::size_t length = rest.size();
    const std::size_t length_at = heads.find("Content-Length:");
    if (length_at != std::string::npos)
      length = static_cast<std::size_t>(
          std::atol(heads.c_str() + length_at + 15));
    arrived_ = rest.substr(0, length);
    read_ = arrived_.size();
    if (status >= 400) {
      status_ = status;
      status_text_ = status_words;
      body_ = arrived_;
      trouble_ = "the server answered " + std::to_string(status) +
                 (status_words.empty() ? std::string{} : ": " + status_words);
    }
    (void)status_line;
    over_ = true;
#endif
  }

  // What has come since the last take, whole, and nothing where nothing has.
  [[nodiscard]] std::string take() {
#if defined(__EMSCRIPTEN__)
    // Nothing carries anything any more: what was asked of the browser is
    // closed the once it has been answered, and only then.
    if (over_ && held_) {
      emscripten_fetch_close(held_);
      held_ = nullptr;
    }
#endif
    return std::exchange(arrived_, std::string{});
  }

  // Nothing more is ever coming. Why not is `trouble()` where something went
  // wrong, and nothing at all where it did not.
  [[nodiscard]] bool over() const { return over_; }
  [[nodiscard]] const std::optional<std::string> &trouble() const {
    return trouble_;
  }
  // The status the server answered with, where it answered with one, and the
  // words it gave beside it. Zero where nothing answered with a status: a
  // page that could not ask at all has none. Read by whoever decides whether
  // a refusal is worth asking again.
  [[nodiscard]] int status() const { return status_; }
  [[nodiscard]] const std::string &status_text() const { return status_text_; }
  // What the server said when it refused, whole: the body of the refusal, if
  // the browser handed it over. A reader asks this where it wants what the
  // server itself said rather than the words beside its number.
  [[nodiscard]] const std::string &error_body() const { return body_; }

  // Not wanted any more: what is still coming is not read, and what asked is
  // closed. Nothing anywhere it has already been answered.
  void stop() { shut(); }

private:
  // Kept for the one C API this is written against: it answers a request
  // through a pointer to a function, so each one is
  // given this object back in `userData` and says everything to it.
#if defined(__EMSCRIPTEN__)
  // A piece arrived, and where the piece says it arrived: take what of it has
  // not been taken yet, and nothing of it twice. A browser that hands the
  // whole body over at once says this too, once.
  static void went_along(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (!one->data || one->numBytes == 0)
      return;
    const std::uint64_t said = static_cast<std::uint64_t>(one->numBytes);
    const std::uint64_t began = one->dataOffset;
    if (began + said <= self.read_)
      return;
    const std::size_t from =
        began < self.read_ ? static_cast<std::size_t>(self.read_ - began) : 0;
    self.arrived_.append(one->data + from,
                         static_cast<std::size_t>(said - from));
    self.read_ = began + said;
  }
  static void went_wrong(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (!self.trouble_)
      self.trouble_ = "the page could not ask for it";
    self.over_ = true;
  }
  static void went_well(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    // Everything in one piece: the browser never handed it over in pieces,
    // so it is taken once, not again on top of what already
    if (self.read_ == 0 && one->data && one->numBytes)
      self.arrived_.append(one->data, static_cast<std::size_t>(one->numBytes));
    self.over_ = true;
  }
  // Answered at all: a server may answer a status and a body, and what it
  // says may still be that it will not.
  static void went_on(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (one->readyState != 4 /* done */)
      return;
    // Answered, and answered that it will not: the body it says that in
    // is not the answer asked for, so it is said here and nothing of it
    // is taken.
    if (one->status >= 400) {
      // The status is kept whole -- the number and the words beside it -- so a
      // reader can ask whether this refusal is the kind that passes, without
      // parsing the sentence the trouble was written in.
      self.status_ = static_cast<int>(one->status);
      self.status_text_ =
          one->statusText[0] != '\0' ? std::string(one->statusText)
                                      : std::string{};
      // The refusal's own body, where the browser gave one: what the server
      // said about itself, kept apart from the sentence the trouble is
      // written in.
      if (one->data && one->numBytes)
        self.body_.assign(one->data, static_cast<std::size_t>(one->numBytes));
      if (!self.trouble_)
        self.trouble_ =
            "the server answered " + std::to_string(one->status) +
            (self.status_text_.empty() ? std::string{}
                                       : ": " + self.status_text_);
      self.over_ = true;
    }
  }
#endif

  void shut() {
#if defined(__EMSCRIPTEN__)
    if (held_)
      emscripten_fetch_close(held_);
    held_ = nullptr;
#endif
  }

#if defined(__EMSCRIPTEN__)
  emscripten_fetch_t *held_ = nullptr;
#endif
  std::string place_;
  std::string wrote_;
  std::vector<std::string> fields_;
  std::vector<const char *> flat_;
  std::string arrived_;
  std::optional<std::string> trouble_;
  int status_ = 0;
  std::string status_text_;
  std::string body_;
  bool over_ = false;
  std::uint64_t read_ = 0;   // how much of the body has been taken already
};

} // namespace pchat::platform
