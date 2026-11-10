module;
#if defined(__EMSCRIPTEN__)
#include <emscripten/console.h>
#include <emscripten/fetch.h>
#endif
#include "carry_desktop.h"
#include "carry_phone.h"
// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.carry -- One body handed to the platform the program is
// running on, and the bytes it answers with, taken back out of it.
//
// Nothing here knows what it carries or what it means: a place, its headers
// and a body go in, and whatever arrives comes back out in the sizes it
// arrived in. Which is all a provider's stream needs, and all that differs
// between a page, a window and a phone.
//
// The page carries through the browser's own fetch, because that is what a
// page has; a window carries through this machine's own curl -- the one HTTP
// client the build already links, the ported tree having brought it in for
// its own transport -- pumped from the program's own loop so that nothing
// here waits on a socket (src/platform/carry_desktop.cpp); a phone carries
// through the stack its own machine carries, asked of over JNI on a thread
// of its own and taken back from this same loop (src/platform/carry_phone.
// cpp) -- TLS, certificates and proxies being the machine's, and nothing of
// them brought into this tree. What each platform says of itself is
// `any()`; everything not written for a platform is built with an empty
// carrying in it, and asking it for bytes asks nobody anything.
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
  // asked for is one request, and closing it twice is not a thing to want.
  carrying(carrying &&gone)
      : place_(std::move(gone.place_)), wrote_(std::move(gone.wrote_)),
        fields_(std::move(gone.fields_)), flat_(std::move(gone.flat_)),
        arrived_(std::move(gone.arrived_)), trouble_(std::move(gone.trouble_)),
        over_(gone.over_), read_(gone.read_),
        desk_(std::exchange(gone.desk_, nullptr)),
        ph_(std::exchange(gone.ph_, nullptr)) {
#if defined(__EMSCRIPTEN__)
    held_ = std::exchange(gone.held_, nullptr);
    attempts_ = std::exchange(gone.attempts_, 0);
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
      read_ = gone.read_;
      desk_ = std::exchange(gone.desk_, nullptr);
      ph_ = std::exchange(gone.ph_, nullptr);
#if defined(__EMSCRIPTEN__)
      held_ = std::exchange(gone.held_, nullptr);
      attempts_ = std::exchange(gone.attempts_, 0);
#endif
    }
    return *this;
  }

  // Whether this platform hands bytes over at all: everything that can be
  // asked of it is nothing where it cannot, so that nothing else here has to
  // be a different program on each platform.
  [[nodiscard]] bool any() const {
#if defined(__EMSCRIPTEN__)
    return true;
#elif defined(PCHAT_HAVE_CURL)
    return true;
#elif defined(PCHAT_HAVE_ANDROID_HTTP)
    return true;
#else
    return false;
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
#endif
    for (const auto &[name, value] : headers) {
      fields_.push_back(name);
      fields_.push_back(value);
    }
    wrote_ = std::string(body);
    place_ = std::string(url);
    read_ = 0;
    attempts_ = 0;
#if defined(__EMSCRIPTEN__)
    // The browser reads the names and the values where they were written, and
    // keeps reading them until it is answered: so they are kept here, whole,
    // and not only for the length of this call.
    flat_.reserve(fields_.size() + 1);
    for (const std::string &one : fields_)
      flat_.push_back(one.c_str());
    flat_.push_back(nullptr);
    start_fetch();
#elif defined(PCHAT_HAVE_CURL)
    // A window asks through this machine's own curl: the same words a page
    // would give the browser, said through one line this module never opens
    // itself (carry_desktop.h), and answered while the loop goes on.
    over_ = false;
    arrived_.clear();
    trouble_.reset();
    if (!desk_)
      desk_ = desktop_open();
    if (desk_ == nullptr || !desktop_ask_at(desk_, place_, fields_, wrote_)) {
      trouble_ = "this machine could not ask for it";
      over_ = true;
    }
#elif defined(PCHAT_HAVE_ANDROID_HTTP)
    // A phone asks through the stack its own machine carries, on a thread
    // of its own (carry_phone.cpp), and what it answers with comes back
    // into this carrying the same way as from anything else here.
    over_ = false;
    arrived_.clear();
    trouble_.reset();
    if (!ph_)
      ph_ = phone_open();
    if (ph_ == nullptr || !phone_ask_at(ph_, place_, fields_, wrote_)) {
      trouble_ = "the phone could not ask for it";
      over_ = true;
    }
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
#elif defined(PCHAT_HAVE_CURL)
    pump_desktop();
#elif defined(PCHAT_HAVE_ANDROID_HTTP)
    pump_phone();
#endif
    return std::exchange(arrived_, std::string{});
  }

  // Nothing more is ever coming. Why not is `trouble()` where something went
  // wrong, and nothing at all where it did not.
  [[nodiscard]] bool over() const { return over_; }
  [[nodiscard]] const std::optional<std::string> &trouble() const {
    return trouble_;
  }

  // Not wanted any more: what is still coming is not read, and what asked is
  // closed. Nothing anywhere it has already been answered.
  void stop() { shut(); }

private:
  // Kept for the one C API this is written against, which answers a request
  // through a pointer to a function and not to anything of ours: each one is
  // given this object back in `userData` and says everything to it.
#if defined(__EMSCRIPTEN__)
  // The asking itself, built out of what begin() kept: said apart because a
  // carrying that could not connect asks again with those very same fields
  // rather than building them anew (fetch_failed).
  void start_fetch() {
    emscripten_fetch_attr_t asking;
    emscripten_fetch_attr_init(&asking);
    std::strcpy(asking.requestMethod, "POST");
    asking.requestHeaders = flat_.data();
    asking.requestData = wrote_.data();
    asking.requestDataSize = wrote_.size();
    // Both: the whole of it is wanted even where a browser never hands it
    // over a piece at a time, and where it does, what answers arrives while
    // it is read, which is what an arriving answer looks like.
    asking.attributes =
        EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_STREAM_DATA;
    asking.userData = this;
    asking.onprogress = &carrying::fetch_progress;
    asking.onerror = &carrying::fetch_failed;
    asking.onreadystatechange = &carrying::fetch_status;
    asking.onsuccess = &carrying::fetch_done;
    held_ = emscripten_fetch(&asking, place_.c_str());
    if (!held_) {
      trouble_ = "the page would not ask for it";
      over_ = true;
    }
  }

  // A piece arrived, and where the piece says it arrived: take what of it has
  // not been taken yet, and nothing of it twice. A browser that hands the
  // whole body over at once instead says this too, once.
  static void fetch_progress(emscripten_fetch_t *one) {
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
  static void fetch_failed(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    // Not an answer but the connection: what a client that retries on
    // connection failure asks again -- the port's client_policy() says to
    // (provider/http/wire.h) -- three askings in all, and only while not one
    // byte has come. An answer that says no is never asked again (fetch_status
    // says why).
    emscripten_fetch_close(one);
    self.held_ = nullptr;
    if (self.attempts_ + 1 < kAskings && self.read_ == 0) {
      ++self.attempts_;
      self.start_fetch();
      if (self.held_)
        return;
    }
    if (!self.trouble_)
      self.trouble_ = "the page could not ask for it";
    self.over_ = true;
  }
  static void fetch_done(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    // Everything in one piece, where the browser never handed it over in
    // pieces: taken once, and not again on top of what already came.
    if (self.read_ == 0 && one->data && one->numBytes)
      self.arrived_.append(one->data, static_cast<std::size_t>(one->numBytes));
    self.over_ = true;
  }
  // Answered at all: a server may answer a status and a body, and what it
  // says may still be that it will not.
  static void fetch_status(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (one->readyState != 4 /* done */)
      return;
    // Answered, and answered that it will not: the body it says that in
    // is not the answer asked for, so it is said here and nothing of it
    // is taken.
    if (one->status >= 400) {
      if (!self.trouble_)
        self.trouble_ =
            "the server answered " + std::to_string(one->status) +
            (one->statusText[0] != '\0'
                                     ? ": " + std::string(one->statusText)
                                     : std::string{});
      self.over_ = true;
    }
  }
#endif

  void shut() {
#if defined(__EMSCRIPTEN__)
    if (held_)
      emscripten_fetch_close(held_);
    held_ = nullptr;
#elif defined(PCHAT_HAVE_CURL)
    desktop_close(&desk_);
#elif defined(PCHAT_HAVE_ANDROID_HTTP)
    phone_close(&ph_);
#endif
  }

#if defined(PCHAT_HAVE_CURL)
  // The window's own asking pumped from the program's own loop: what arrived
  // since the last take, and where the asking stands -- with the one case a
  // carrying may ask again of itself: the connection went before a byte of an
  // answer came, three askings in all, exactly as the page asks again
  // (fetch_failed).
  void pump_desktop() {
    if (desk_ == nullptr || over_)
      return;
    std::string piece;
    std::string why;
    const desktop_end stood = desktop_pump(desk_, piece, why);
    switch (stood) {
      case desktop_end::working:
        break;
      case desktop_end::answered:
        over_ = true;
        break;
      case desktop_end::refused:
        if (!trouble_)
          trouble_ = std::move(why);
        over_ = true;
        break;
      case desktop_end::lost:
        if (attempts_ + 1 < kAskings && read_ == 0) {
          ++attempts_;
          if (desktop_ask_at(desk_, place_, fields_, wrote_))
            break;
        }
        if (!trouble_)
          trouble_ = why.empty() ? std::string("the machine could not ask for it")
                                 : why;
        over_ = true;
        break;
    }
    // What a server says it will not answer with is not an answer: a refusal's
    // own body is dropped here, in `trouble`'s words alone -- while everything
    // else a wire says on the way is taken as it comes, piece by piece.
    if (stood != desktop_end::refused && !piece.empty()) {
      read_ += piece.size();
      arrived_ += piece;
    }
    if (over_)
      desktop_close(&desk_);
  }
#endif

#if defined(PCHAT_HAVE_ANDROID_HTTP)
  // The phone's own asking pumped from the program's own loop, the same as a
  // window's: what arrived since the last take, and where it stands -- with
  // the one case a carrying may ask again of itself, said in the same words
  // as everywhere else here: the connection went before an answer, three
  // askings in all.
  void pump_phone() {
    if (ph_ == nullptr || over_)
      return;
    std::string piece;
    std::string why;
    const phone_end stood = phone_pump(ph_, piece, why);
    switch (stood) {
      case phone_end::working:
        break;
      case phone_end::answered:
        over_ = true;
        break;
      case phone_end::refused:
        if (!trouble_)
          trouble_ = std::move(why);
        over_ = true;
        break;
      case phone_end::lost:
        if (attempts_ + 1 < kAskings && read_ == 0) {
          ++attempts_;
          if (phone_ask_at(ph_, place_, fields_, wrote_))
            break;
        }
        if (!trouble_)
          trouble_ = why.empty()
                         ? std::string("the phone could not ask for it")
                         : why;
        over_ = true;
        break;
    }
    // A refusal's own words are not an answer and its body is never read
    // (carry_phone.cpp says so where the status comes); everything else a
    // wire says on the way is taken as it comes, piece by piece.
    if (stood != phone_end::refused && !piece.empty()) {
      read_ += piece.size();
      arrived_ += piece;
    }
    if (over_)
      phone_close(&ph_);
  }
#endif

  // How many askings this carrying made and lost to the connection, and the
  // ceiling they are counted against: three in all, said here where the retry
  // lives (fetch_failed, pump_desktop).
  static constexpr std::uint32_t kAskings = 3;
  std::uint32_t attempts_ = 0;
#if defined(__EMSCRIPTEN__)
  emscripten_fetch_t *held_ = nullptr;
#endif
  // The machine's own asking, where this platform has a machine that asks
  // (carry_desktop.cpp): null everywhere it does not, and kept whole through
  // a move like everything else here.
  desktop_ask *desk_ = nullptr;
  // The phone's own asking, where this platform is a phone (carry_phone.
  // cpp): the same null-everywhere-and-whole-through-a-move as a window's.
  phone_ask *ph_ = nullptr;
  std::string place_;
  std::string wrote_;
  std::vector<std::string> fields_;
  std::vector<const char *> flat_;
  std::string arrived_;
  std::optional<std::string> trouble_;
  bool over_ = false;
  std::uint64_t read_ = 0;   // how much of the body has been taken already
};

} // namespace pchat::platform
