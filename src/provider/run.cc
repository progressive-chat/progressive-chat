// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider.run -- One answer, from the moment it is asked for to the
// moment nothing more is coming: what is carried out, what comes back as it
// comes, what stopped it, and the answer as the model keeps it.
//
// Nothing here carries anything, and nothing here is called into by a
// transport. A transport takes the asking -- the place, its headers, the
// body -- as data, hands the bytes back here as data, and says when it has
// finished carrying them. That is what lets one answer be said whole by a
// test with no network under it, and the same answer be carried later by a
// page's fetch, a socket, or fibers.
module;
#include "retry/retry.h"

export module pchat.provider.run;

import std;
import pchat.core;
import pchat.provider;
import pchat.provider.stream;

export namespace pchat::provider {

// Where one answer stands: asked for and not carried yet, arriving, or over
// for good.
enum class standing { ready, going, done };

// How it ended, in the words a screen shows it in: it finished, the reader
// stopped it, or it could not be had at all. `none` is an answer that has
// not ended.
enum class over { none, complete, stopped, failed };

// Everything a transport is given to carry: where, with what headers, and
// what body. Written out when the run is made, not when something asks, so
// the sent bytes are the same whoever carries them and a test can read them
// without carrying them anywhere.
struct asking {
  std::string url;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;
};

// Where an answer is put when it is over: the conversation it was said in,
// whose it is written as, when it was said, and how long it took to come.
struct put_in {
  message_id id;
  conversation_id chat;
  std::string author;
  moment sent = 0;
  std::int64_t took_ms = 0;
};

// One answer written into a message, wherever the answer came from: one run,
// or one question asked again before it was answered. Author, place, duration
// and failed where nothing of it came, set here so both callers say the same
// thing twice.
[[nodiscard]] inline message answer_of(const put_in &into, std::string_view said,
                                       std::string_view thinking,
                                       std::size_t asking, usage cost,
                                       bool failed,
                                       const std::optional<std::string> &why = {}) {
  message out{.id = into.id,
              .chat = into.chat,
              .author = into.author,
              .outgoing = false,
              .text = std::string(said),
              .sent = into.sent,
              .state = failed ? delivery::failed : delivery::sent};
  // What said it was a failure: held only where it was one, so a message that
  // went through says nothing about a reason it has none of.
  if (failed && why)
    out.why_failed = *why;
  out.tokens = static_cast<std::uint32_t>(cost.in + cost.out);
  // Said in full: `thought` is what an answer thought, and not the same word
  // as the type what it thought is kept in.
  if (!thinking.empty() || into.took_ms != 0 || asking != 0)
    out.before = ::pchat::thought{.took_ms = into.took_ms,
                                  .tool_calls = static_cast<std::uint32_t>(asking),
                                  .said = std::string(thinking)};
  return out;
}

// One answer. It owns the body asked with and the stream that comes back,
// and is the only thing between them. The screens read from here and nowhere
// else while an answer arrives: what it has said so far, what it thought,
// what it cost, why it is over.
class run {
public:
  // The turns as any range that holds them, for a body written once and
  // never again.
  template <std::ranges::forward_range Turns>
    requires std::same_as<std::ranges::range_value_t<Turns>, turn>
  run(endpoint at, Turns &&turns, const wanted &wants = {})
      : asking_{.url = std::string(url_of(at)),
                .headers = headers_of(at),
                .body = body_of(at, turns, wants)},
      coming_(wire_of(at)) {}

  // ---- what a transport reads and is given back --------------------------

  [[nodiscard]] const asking &wants() const { return asking_; }

  // It is being carried now. Nothing comes before this is said, and saying
  // it twice is the same as saying it once.
  void carried() {
    if (now_ == standing::ready)
      now_ = standing::going;
  }

  // A piece of the answer, in any size it arrives in. Nothing is taken of
  // an answer that was stopped, or of one that is over.
  void arrive(std::string_view bytes) {
    if (now_ != standing::going)
      return;
    coming_.feed(bytes);
    if (coming_.failed())
      stop_over(over::failed, std::string(coming_.why()),
                coming_.why_failed());
  }

  // What carried it has finished: `why` is its own words where it could not
  // carry it (no route, no answer, the wire closed early) and nothing at all
  // where it carried it whole.
  void carried_over(std::optional<std::string> why) {
    if (now_ == standing::done)
      return;
    if (why)
      stop_over(over::failed, std::move(*why));
    else
      stop_over(over::complete, std::nullopt);
  }

  // The reader has stopped it: what is wanted of the answer is no longer
  // wanted, so nothing more of it is taken, and nothing after this is said
  // as read.
  void stopped() {
    if (now_ == standing::going)
      stop_over(over::stopped, std::nullopt);
  }

  // ---- what the screens read --------------------------------------------

  [[nodiscard]] standing where() const { return now_; }
  [[nodiscard]] bool going() const { return now_ == standing::going; }
  [[nodiscard]] bool done() const { return now_ == standing::done; }
  [[nodiscard]] over ended() const { return ended_; }
  // Why it could not be had, whole, where it could not.
  [[nodiscard]] std::optional<std::string> trouble() const { return trouble_; }
  // Why it failed, the way Agora says it: the kind of it, and what came with
  // it. Set only where it failed, and read by whoever decides whether to ask
  // again.
  [[nodiscard]] const failure &why_failed() const { return failed_why_; }

  [[nodiscard]] std::string_view said() const { return coming_.text(); }
  [[nodiscard]] std::string_view thought() const { return coming_.thought(); }
  [[nodiscard]] usage cost() const { return coming_.cost(); }
  [[nodiscard]] std::size_t tool_calls() const { return coming_.calls().size(); }
  // Every tool it stopped to ask for, whole, in the order it asked. Read out
  // of here and answered nowhere in particular: this runs nothing.
  [[nodiscard]] const std::vector<tool_call> &called() const {
    return coming_.calls();
  }
  // Why the wire says it stopped: stop, length, tool_calls, content_filter.
  [[nodiscard]] std::string_view finish() const { return coming_.finish(); }

  // What came since the last take(), oldest first: text, thought, a tool it
  // asked for, what it cost, and the end of it.
  [[nodiscard]] std::vector<event> take() { return coming_.take(); }

  // The answer as the model keeps it: one message of the model's, with what
  // it thought before it spoke, what it cost, and how it stands -- failed
  // where nothing of it came to be read. Nothing here answers for a model,
  // and nothing here is put into a conversation: where it goes is asked
  // elsewhere.
  [[nodiscard]] message answer_of(const put_in &into) const {
    return provider::answer_of(into, said(), thought(), tool_calls(), cost(),
                               ended_ == over::failed, trouble_);
  }

private:
  // Over, one way or the other, set once: after that, only what already
  // came, nothing more.
  void stop_over(over how, std::optional<std::string> why,
                 const failure &why_failed = {}) {
    now_ = standing::done;
    ended_ = how;
    trouble_ = std::move(why);
    failed_why_ = why_failed;
  }

  asking asking_;
  stream coming_;
  standing now_ = standing::ready;
  over ended_ = over::none;
  std::optional<std::string> trouble_;
  failure failed_why_;
};

} // namespace pchat::provider
