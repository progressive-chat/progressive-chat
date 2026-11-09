// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.carry_desktop -- The asking itself, over the machine's own
// curl: one easy handle that says the question and one multi handle that runs
// it, pumped from the program's own loop (carry_desktop.h says what each pump
// means). Nothing here knows what the body means -- bytes out, bytes back.
#include "carry_desktop.h"

#include <curl/curl.h>

#include "../provider/http/wire.h"

namespace pchat::platform {

namespace {

// What the answering server writes, straight into the piece the carrying will
// take: no buffering of our own beyond what std::string does, and no piece of
// it read twice.
std::size_t wrote_in(char *got, std::size_t size, std::size_t count, void *held) {
  const std::size_t bytes = size * count;
  if (bytes == 0)
    return 0;
  static_cast<std::string *>(held)->append(got, bytes);
  return bytes;
}

}  // namespace

struct desktop_ask {
  CURLM *multi = nullptr;
  CURL *one = nullptr;
  curl_slist *fields = nullptr;
  bool running = false;
  // What the server has written since the last pump: drained into the piece
  // the carrying takes, at every pump, so that what arrived is said once and
  // while it is still arriving.
  std::string in;
};

desktop_ask *desktop_open() {
  CURLM *multi = curl_multi_init();
  if (!multi)
    return nullptr;
  return new desktop_ask{.multi = multi};
}

void desktop_close(desktop_ask **held) {
  if (held == nullptr || *held == nullptr)
    return;
  desktop_ask *one = *held;
  if (one->one) {
    if (one->running)
      curl_multi_remove_handle(one->multi, one->one);
    curl_easy_cleanup(one->one);
    one->one = nullptr;
  }
  if (one->fields) {
    curl_slist_free_all(one->fields);
    one->fields = nullptr;
  }
  curl_multi_cleanup(one->multi);
  one->multi = nullptr;
  delete one;
  *held = nullptr;
}

bool desktop_ask_at(desktop_ask *held, std::string_view url,
                    std::span<const std::string> fields,
                    std::string_view body) {
  if (held == nullptr || held->multi == nullptr)
    return false;
  // The asking of the last attempt is put away before this one is made: a
  // carrying that asks again (the connection never came) asks with the same
  // words, and nothing of the last asking is left running behind it.
  if (held->one) {
    if (held->running)
      curl_multi_remove_handle(held->multi, held->one);
    curl_easy_cleanup(held->one);
    held->one = nullptr;
  }
  if (held->fields) {
    curl_slist_free_all(held->fields);
    held->fields = nullptr;
  }
  CURL *one = curl_easy_init();
  if (!one)
    return false;
  held->one = one;
  for (std::size_t at = 0; at + 1 < fields.size(); at += 2)
    held->fields = curl_slist_append(
        held->fields, (std::string(fields[at]) + ": " + fields[at + 1]).c_str());
  const std::string asked(body);
  const std::string place(url);
  curl_easy_setopt(one, CURLOPT_URL, place.c_str());
  curl_easy_setopt(one, CURLOPT_POST, 1L);
  curl_easy_setopt(one, CURLOPT_POSTFIELDSIZE, static_cast<long>(asked.size()));
  curl_easy_setopt(one, CURLOPT_COPYPOSTFIELDS, asked.data());
  curl_easy_setopt(one, CURLOPT_HTTPHEADER, held->fields);
  curl_easy_setopt(one, CURLOPT_WRITEFUNCTION, &wrote_in);
  curl_easy_setopt(one, CURLOPT_WRITEDATA, static_cast<void *>(&held->in));
  // No signals: a timeout is read by the loop, and a window that draws on a
  // thread of its own never gets a SIGALRM in the middle of a frame.
  curl_easy_setopt(one, CURLOPT_NOSIGNAL, 1L);
  // What the client policy of this tree says a client is built with
  // (provider/http/wire.h): how long to wait for a connection at all. The
  // rest of the policy is what the carrying reads back, and an answer that
  // comes slowly is still an answer.
  const auto policy = pchat::provider::client_policy();
  curl_easy_setopt(one, CURLOPT_CONNECTTIMEOUT,
                   static_cast<long>(policy.connect_seconds));
  curl_easy_setopt(one, CURLOPT_FOLLOWLOCATION,
                   policy.follow_redirects ? 1L : 0L);
  curl_multi_add_handle(held->multi, one);
  held->running = true;
  return true;
}

desktop_end desktop_pump(desktop_ask *held, std::string &piece,
                         std::string &why) {
  if (held == nullptr || held->multi == nullptr || !held->one || !held->running)
    return desktop_end::working;
  int running = 0;
  curl_multi_perform(held->multi, &running);
  // What came while the loop was away, said before the asking is said to be
  // over: a last piece written on the very pump that ends it is still an
  // answer, and the carrying reads it in the same take it reads the end in.
  if (!held->in.empty()) {
    piece += std::move(held->in);
    held->in.clear();
  }
  CURLMsg *said = nullptr;
  int left = 0;
  while ((said = curl_multi_info_read(held->multi, &left)) != nullptr) {
    if (said->msg != CURLMSG_DONE)
      continue;
    const CURLcode how = said->data.result;
    long status = 0;
    curl_easy_getinfo(said->easy_handle, CURLINFO_RESPONSE_CODE, &status);
    // What the asking is over with: answered, answered that it will not, or
    // not answered at all -- each of them said in its own words, and the
    // handle put away in all three.
    desktop_end over = desktop_end::answered;
    if (how != CURLE_OK) {
      over = desktop_end::lost;
      why = curl_easy_strerror(how);
    } else if (status >= 400) {
      over = desktop_end::refused;
      why = "the server answered " + std::to_string(status);
    }
    curl_multi_remove_handle(held->multi, said->easy_handle);
    curl_easy_cleanup(said->easy_handle);
    held->one = nullptr;
    held->running = false;
    return over;
  }
  return desktop_end::working;
}

}  // namespace pchat::platform
