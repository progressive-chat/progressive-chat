// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.formatting -- A message's text as the runs it is drawn in:
// bold, italic, code, a link. Read once here, and the screen draws runs and
// never looks at a star again.
export module pchat.logic.formatting;

import std;

export namespace pchat::logic {

// What a run is. They are bits, because a run can be more than one of them
// at once -- bold italic, a bold link.
using run_style = unsigned;
inline constexpr run_style plain = 0u;
inline constexpr run_style bold = 1u << 0;
inline constexpr run_style italic = 1u << 1;
inline constexpr run_style code = 1u << 2;
inline constexpr run_style link = 1u << 3;

struct run {
  std::string_view text;
  run_style style = plain;
  std::string_view target;   // where a link goes
  friend bool operator==(const run&, const run&) = default;
};

// One run and where it stands: where it begins and ends in the line as it is
// drawn, which is what the screen is told, and nothing is looked up by
// counting marks again.
struct run_place {
  std::size_t first = 0;
  std::size_t last = 0;
  run_style style = plain;
  friend bool operator==(const run_place&, const run_place&) = default;
};

namespace formatting_detail {

// One mark, and the run it opens.
struct mark {
  std::string_view open;
  run_style style;
  bool link_target = false;
};

inline constexpr mark marks[] = {
  {std::string_view{"**"}, bold, false},
  {std::string_view{"*"}, italic, false},
  {std::string_view{"_"}, italic, false},
  {std::string_view{"`"}, code, false},
};

[[nodiscard]] inline std::optional<std::pair<std::size_t, run_style>> opening_at(
    std::string_view text, std::size_t at) {
  for (const mark& one : marks) {
    if (text.substr(at).starts_with(one.open)) return std::pair{one.open.size(), one.style};
  }
  return std::nullopt;
}

// What begins a place said in a line, longest first so that https:// is read
// as itself and not as http:// with an s left over.
inline constexpr std::string_view place_begun_by[] = {"https://", "http://", "www."};
// Marks no place runs past: what wraps one in writing, and what a line is
// made of.
[[nodiscard]] inline bool link_stops_at(char unit) {
  constexpr std::string_view stops = " \t\n\r<>()[]{}\"'`|";
  return stops.find(unit) != std::string_view::npos;
}
// What a sentence says after a place rather than in it: the stop at the end
// of one, and the commas and colons a list of them is written with.
[[nodiscard]] inline bool said_after_place(char unit) {
  constexpr std::string_view after = ".,;:!?";
  return after.find(unit) != std::string_view::npos;
}

}  // namespace formatting_detail

// The runs of one message's text. A mark that never closes is text, which is
// what any other reader of it would show, too.
[[nodiscard]] inline std::vector<run> runs_of(std::string_view text) {
  using formatting_detail::marks;
  std::vector<run> runs;
  run_style open = plain;
  std::size_t from = 0;
  for (std::size_t at = 0; at < text.size();) {
    if (const auto found = formatting_detail::opening_at(text, at)) {
      const auto [taken, style] = *found;
      if (at > from) runs.push_back({.text = text.substr(from, at - from), .style = open});
      if (open & style) {
        open &= ~style;
      } else {
        open |= style;
      }
      at += taken;
      from = at;
      continue;
    }
    ++at;
  }
  if (text.size() > from) runs.push_back({.text = text.substr(from), .style = open});
  return runs;
}

// The words of a run, as the lines it is broken into want them.
[[nodiscard]] inline bool is_break(char unit) { return unit == ' ' || unit == '\n' || unit == '\t'; }

// Where a link stands in a line, called a place as the writing here calls
// it: the first unit of it, one past its last, and where it goes. Read as a
// reader reads one -- begun by http://, https:// or www., and running until
// something no place goes in -- with what a sentence says after a place left
// out of it. (`link` is the bit a run carries, which is why this is not.)
struct place {
  std::size_t first = 0;
  std::size_t last = 0;
  std::string target;
  friend bool operator==(const place&, const place&) = default;
};

// The line as the screen draws it: what is said with every mark that said it
// taken out of it, and where each run stands in what is left. A star and a
// backtick are how a model says bold and code: the screen draws what they
// said, and says none of them.
struct drawn {
  std::string said;
  std::vector<run_place> runs;   // each stretch that is not plain
};

[[nodiscard]] inline drawn drawn_of(std::string_view text) {
  drawn out;
  for (const run &one : runs_of(text)) {
    if (one.style != plain)
      out.runs.push_back({.first = out.said.size(),
                          .last = out.said.size() + one.text.size(),
                          .style = one.style});
    out.said += one.text;
  }
  return out;
}

[[nodiscard]] inline std::vector<place> links_of(std::string_view text) {
  using formatting_detail::link_stops_at;
  using formatting_detail::place_begun_by;
  using formatting_detail::said_after_place;
  std::vector<place> links;
  for (std::size_t at = 0; at < text.size();) {
    std::string_view begun;
    for (const std::string_view one : place_begun_by) {
      if (text.substr(at).starts_with(one)) {
        begun = one;
        break;
      }
    }
    if (begun.empty()) {
      ++at;
      continue;
    }
    std::size_t to = at + begun.size();
    while (to < text.size() && !link_stops_at(text[to])) ++to;
    while (to > at + begun.size() && said_after_place(text[to - 1])) --to;
    if (to - at > begun.size()) {
      std::string target{text.substr(at, to - at)};
      // A place begun www. is https:// to the machine, said without either.
      if (begun == "www.") target = "https://" + target;
      links.push_back({.first = at, .last = to, .target = std::move(target)});
    }
    at = to;
  }
  return links;
}

}  // namespace pchat::logic
