// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.formatting -- A message's text as the runs it is drawn in:
// bold, italic, struck, code, underlined, a link. Read once here, and the
// screen draws runs and never looks at a star again.
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
inline constexpr run_style struck = 1u << 4;
inline constexpr run_style underline = 1u << 5;
// A block of code: what stands between two fences of three backticks, said
// in the face code is said in and as it stands, marks and all.
inline constexpr run_style block = 1u << 6;

struct run {
  std::string_view text;
  run_style style = plain;
  std::string_view target;   // where a link goes
  int quoted = 0;            // how deep in quotes its line stands
  friend bool operator==(const run&, const run&) = default;
};

// One run and where it stands: where it begins and ends in the line as it is
// drawn, which is what the screen is told, and nothing is looked up by
// counting marks again.
struct run_place {
  std::size_t first = 0;
  std::size_t last = 0;
  run_style style = plain;
  int quoted = 0;            // how deep in quotes the line it stands in is
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
  {std::string_view{"__"}, underline, false},
  {std::string_view{"_"}, italic, false},
  {std::string_view{"~~"}, struck, false},
  {std::string_view{"`"}, code, false},
};

[[nodiscard]] inline std::optional<std::pair<std::size_t, run_style>> opening_at(
    std::string_view text, std::size_t at, run_style open) {
  // What stands inside code is said as it stands: a star in it is a star and
  // not the beginning of slanted, so nothing opens there but the backtick
  // that ends it.
  if (open & code) {
    if (text[at] == '`') return std::pair{std::size_t{1}, code};
    return std::nullopt;
  }
  for (const mark& one : marks) {
    if (text.substr(at).starts_with(one.open)) return std::pair{one.open.size(), one.style};
  }
  return std::nullopt;
}

// A link written the way writing writes one: [what it is called](where it
// goes), and nothing after either of them is part of it. What it is called
// is drawn as it stands, one run of the line, and nothing in it is read for
// marks of its own.
[[nodiscard]] inline std::optional<std::tuple<std::string_view, std::string_view, std::size_t>>
link_written_at(std::string_view text, std::size_t at) {
  const std::size_t shut = text.find("](", at + 1);
  if (shut == std::string_view::npos) return std::nullopt;
  const std::size_t gone = text.find(')', shut + 2);
  if (gone == std::string_view::npos) return std::nullopt;
  return std::tuple{text.substr(at + 1, shut - at - 1),
                    text.substr(shut + 2, gone - shut - 2), gone + 1 - at};
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

// How deep in quotes a line stands, said by stops of quote at the head of
// it -- one more of them for every line quoted inside the last -- and how
// much of the line they took to say it. None of them is drawn, as no mark
// is; what stands in their place is quoted.
[[nodiscard]] inline std::pair<std::size_t, int>
quoted_at(std::string_view text, std::size_t at) {
  std::size_t taken = 0;
  int depth = 0;
  while (at + taken < text.size() && text[at + taken] == '>') {
    ++depth;
    ++taken;
    if (at + taken < text.size() && text[at + taken] == ' ') ++taken;
  }
  return depth > 0 ? std::pair{taken, depth} : std::pair{std::size_t{0}, 0};
}

}  // namespace formatting_detail

// The runs of one message's text. A mark that never closes is text, which is
// what any other reader of it would show, too.
[[nodiscard]] inline std::vector<run> runs_of(std::string_view text) {
  using formatting_detail::marks;
  std::vector<run> runs;
  run_style open = plain;
  int quoted = 0;
  std::size_t from = 0;
  for (std::size_t at = 0; at < text.size();) {
    // A run never crosses a line, for how deep a line stands in quotes is
    // said of that line and of no other, and what a run would carry across
    // two is not one thing to carry.
    if (text[at] == '\n') {
      runs.push_back({.text = text.substr(from, at + 1 - from),
                      .style = open,
                      .quoted = quoted});
      quoted = 0;
      from = at + 1;
      ++at;
      continue;
    }
    // A fence of three backticks at the head of a line: what stands between
    // it and the next fence is one block, said in the face code is said in
    // and as it stands, marks and all -- no star in it is read as a mark --
    // and neither fence is said. What stood before it on the line it stands
    // on is said as it was.
    if ((!(open & code) || (open & block)) &&
        (at == 0 || text[at - 1] == '\n') && text.substr(at).starts_with("```")) {
      if (at > from)
        runs.push_back({.text = text.substr(from, at - from),
                        .style = open,
                        .quoted = quoted});
      const std::size_t whole = text.find('\n', at);
      const std::size_t after =
          whole == std::string_view::npos ? text.size() : whole + 1;
      if (open & block)
        open &= ~(code | block);
      else
        open |= code | block;
      at = after;
      from = after;
      continue;
    }
    // At the head of a line: the stops of quote it stands under, taken out,
    // and how deep they say it stands.
    if (at == 0 || text[at - 1] == '\n') {
      const auto [taken, depth] = formatting_detail::quoted_at(text, at);
      if (taken > 0) {
        quoted = depth;
        at += taken;
        from = at;
        continue;
      }
    }
    // A link written as one is written in what a model says: what it is
    // called in square stops, and where it goes in round ones after them.
    // Neither pair is drawn, as no mark is.
    if (!(open & code) && text[at] == '[') {
      if (const auto said_place = formatting_detail::link_written_at(text, at)) {
        const auto [said, target, taken] = *said_place;
        if (at > from)
          runs.push_back({.text = text.substr(from, at - from),
                          .style = open,
                          .quoted = quoted});
        runs.push_back({.text = said,
                        .style = open | link,
                        .target = target,
                        .quoted = quoted});
        at += taken;
        from = at;
        continue;
      }
    }
    // What a block is said in no mark opens: a star in it is a star.
    const auto found = !(open & block)
        ? formatting_detail::opening_at(text, at, open)
        : decltype(formatting_detail::opening_at(text, at, open)){};
    if (found) {
      const auto [taken, style] = *found;
      if (at > from)
        runs.push_back({.text = text.substr(from, at - from),
                        .style = open,
                        .quoted = quoted});
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
  if (text.size() > from)
    runs.push_back(
        {.text = text.substr(from), .style = open, .quoted = quoted});
  return runs;
}

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
  std::vector<run_place> runs;     // each stretch that is not plain
  std::vector<place> places;       // each link written in it, and where it goes
};

[[nodiscard]] inline drawn drawn_of(std::string_view text) {
  drawn out;
  for (const run &one : runs_of(text)) {
    if (one.style != plain || one.quoted != 0)
      out.runs.push_back({.first = out.said.size(),
                          .last = out.said.size() + one.text.size(),
                          .style = one.style,
                          .quoted = one.quoted});
    if (one.style & link)
      out.places.push_back({.first = out.said.size(),
                            .last = out.said.size() + one.text.size(),
                            .target = std::string{one.target}});
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
