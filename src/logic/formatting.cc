// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.formatting -- A message's text as the runs it is drawn in:
// bold, italic, struck, code, underlined, a link. Read once here; the
// screen draws runs and never looks at a star again.
export module pchat.logic.formatting;

import std;

export namespace pchat::logic {

// A run is bits, because a run can be more than one
// at once: bold italic, a bold link.
using run_style = unsigned;
inline constexpr run_style plain = 0u;
inline constexpr run_style bold = 1u << 0;
inline constexpr run_style italic = 1u << 1;
inline constexpr run_style code = 1u << 2;
inline constexpr run_style link = 1u << 3;
inline constexpr run_style struck = 1u << 4;
inline constexpr run_style underline = 1u << 5;
// A block of code: what stands between two fences of three backticks,
// in the face code is said in, marks and all.
inline constexpr run_style block = 1u << 6;
// What a line is made of at its head, read out of the marks that say it and
// drawn without them: a heading (`#` to `######`) in the size of its level,
// a list item (`-`, `*`, `+` or a number followed by `.`) as a list. Exactly
// one of h1..h6 stands on a heading's runs; a line is a heading or it is not.
inline constexpr run_style heading = 1u << 7;
inline constexpr run_style h1 = 1u << 8;
inline constexpr run_style h2 = 1u << 9;
inline constexpr run_style h3 = 1u << 10;
inline constexpr run_style h4 = 1u << 11;
inline constexpr run_style h5 = 1u << 12;
inline constexpr run_style h6 = 1u << 13;
inline constexpr run_style list = 1u << 14;
// What a model said for later: `||` either side of it. Read as one stretch
// of the line and drawn covered behind a plate of dots, so it is there and
// is not said until the screen is asked to show it.
inline constexpr run_style spoiler = 1u << 15;

struct run {
  std::string_view text;
  run_style style = plain;
  std::string_view target;   // where a link goes
  int quoted = 0;            // how deep in quotes its line stands
  friend bool operator==(const run&, const run&) = default;
};

// One run and where it stands: where it begins and ends in the line as it is
// drawn, what the screen is told. Nothing is looked up by
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
  {std::string_view{"||"}, spoiler, false},
};

[[nodiscard]] inline std::optional<std::pair<std::size_t, run_style>> opening_at(
    std::string_view text, std::size_t at, run_style open) {
  // What stands inside code is said as it stands: a star in it is a star,
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
// goes); nothing after either of them is part of it. What it is called
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

// What begins a place said in a line, longest first: https:// is read
// as itself, not as http:// with an s left over.
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
// it -- one more for every line quoted inside the last -- and how
// much of the line they took. None of them is drawn, as no mark
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

// What a line is made of at its head, read out of its marks: a heading of
// one to six `#` each followed by a space, or a list begun by `-`, `*`, `+`
// or a number and a dot, each followed by a space. It says how much of the
// line the marks took, and the run the line stands in. A mark that
// does not say all of that is not one: `#nospace` is a word, `--` is two
// dashes, `1.` at the end of a line is a number. None of the marks is drawn.
[[nodiscard]] inline std::pair<std::size_t, run_style>
line_head_at(std::string_view text, std::size_t at) {
  // A heading: one to six `#`, the space after them taken too.
  if (text[at] == '#') {
    std::size_t taken = 0;
    while (at + taken < text.size() && text[at + taken] == '#') ++taken;
    if (taken <= 6 && at + taken < text.size() && text[at + taken] == ' ') {
      ++taken;
      const run_style level = h1 << (taken - 2);  // 2..7 marks -> h1..h6
      return {taken, heading | level};
    }
    return {0, plain};
  }
  // A list begun by a dash, star or plus: the one mark and its space.
  if (text[at] == '-' || text[at] == '*' || text[at] == '+') {
    if (at + 2 <= text.size() && text[at + 1] == ' ')
      return {2, list};
    return {0, plain};
  }
  // A list begun by a number and a dot: one to nine digits (a longer run
  // is a number, not the head of a list), the dot and the space after them.
  if (text[at] >= '1' && text[at] <= '9') {
    std::size_t taken = 0;
    while (at + taken < text.size() && text[at + taken] >= '0' &&
           text[at + taken] <= '9')
      ++taken;
    if (taken <= 9 && at + taken + 1 < text.size() &&
        text[at + taken] == '.' && text[at + taken + 1] == ' ') {
      return {taken + 2, list};
    }
    return {0, plain};
  }
  return {0, plain};
}

}  // namespace formatting_detail

// The runs of one message's text. A mark that never closes is text, as any
// other reader would show it.
[[nodiscard]] inline std::vector<run> runs_of(std::string_view text) {
  using formatting_detail::marks;
  std::vector<run> runs;
  run_style open = plain;
  int quoted = 0;
  std::size_t from = 0;
  for (std::size_t at = 0; at < text.size();) {
    // A run never crosses a line: how deep a line stands in quotes is
    // said of that line and of no other, and what a run would carry across
    // two is not one thing to carry.
    if (text[at] == '\n') {
      runs.push_back({.text = text.substr(from, at + 1 - from),
                      .style = open,
                      .quoted = quoted});
      quoted = 0;
      // What a line is made of is said of that line and of no other: the
      // marks of a heading or a list are read again at the next line's
      // head, so the bits that said them do not stand over the newline.
      open &= ~(heading | h1 | h2 | h3 | h4 | h5 | h6 | list | spoiler);
      from = at + 1;
      ++at;
      continue;
    }
    // A fence of three backticks at the head of a line: what stands between
    // it and the next fence is one block, in the face code is said in,
    // marks and all -- no star in it is read as a mark --
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
      // What the line is made of at its head: a heading or a list, its marks
      // taken out, the run they say the line stands in standing for them.
      // A list keeps a bullet where its mark stood, as the doc draws one
      // (`src/ui/doc.cc`), so the words are drawn and the mark is not.
      const auto [head_taken, head_style] =
          formatting_detail::line_head_at(text, at);
      if (head_taken > 0) {
        if (at > from)
          runs.push_back({.text = text.substr(from, at - from),
                          .style = open,
                          .quoted = quoted});
        if (head_style & list)
          runs.push_back({.text = std::string_view{"\u2022  "},
                          .style = open | head_style,
                          .quoted = quoted});
        open |= head_style;
        at += head_taken;
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
    // In a block no mark opens: a star in it is a star.
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

// Where a link stands in a line, a place as this writing calls it: the first
// unit of it, one past its last, and where it goes. Read as a reader reads
// one -- begun by http://, https:// or www., running until something no
// place goes in -- with what a sentence says after a place left out.
// (`link` is the bit a run carries; this is not.)
struct place {
  std::size_t first = 0;
  std::size_t last = 0;
  std::string target;
  friend bool operator==(const place&, const place&) = default;
};

// The line as the screen draws it: what is said with every mark that said it
// taken out, and where each run stands in what is left. A star and a
// backtick are how a model says bold and code: the screen draws what they
// said and says none of them.
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
