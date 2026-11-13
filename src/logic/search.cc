// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.search -- Finding words in a conversation's messages: every
// place a query is said, in the order the timeline has them, so what keeps a
// conversation for saying it can say which of them that was.
export module pchat.logic.search;

import std;
import pchat.core;
import pchat.logic.lists;

export namespace pchat::logic {

// One place a query is said.
struct hit {
  const message* found;
  std::size_t at = 0;
  friend bool operator==(const hit&, const hit&) = default;
};

// Every place it is said in one conversation, in the order the timeline has
// them: by the moment each message was sent, and inside one message by where
// the query stands in it. The order is the message's own `sent`, not the order
// the rows stand in storage: what was written first is read first even where
// a merge or an import left the rows in another order.
[[nodiscard]] inline std::vector<hit> hits_of(const chats_root& root, const conversation_id& chat,
                                             std::string_view query) {
  if (query.empty()) return {};
  const std::string needle = folded(query);
  std::vector<hit> hits;
  for (const message& said : root.messages) {
    if (said.chat != chat) continue;
    // Folding is ASCII's, one unit in and one unit out: an offset in the
    // folded text stands on the same unit of the message's own words, so what
    // a caller reads at `at` is what it sees.
    const std::string haystack = folded(said.text);
    for (std::size_t at = haystack.find(needle); at != std::string::npos;
         at = haystack.find(needle, at + 1)) {
      hits.push_back({.found = &said, .at = at});
    }
  }
  std::ranges::sort(hits, [](const hit& left, const hit& right) {
    if (left.found->sent != right.found->sent)
      return left.found->sent < right.found->sent;
    return left.at < right.at;
  });
  return hits;
}

}  // namespace pchat::logic
