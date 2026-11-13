// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.doc -- The design, read into lines each knowing what it is: how
// many hashes stand in front of it, saying how large it is to be read,
// whether it is one of a list, or nothing but the room it leaves between two
// others.
//
// Read once, out of the string the build wrote from design-doc.md, so what
// the room draws reads against what was written. It is read from its own
// words, so tests can say it with no window under them.
module;
#include <pchat/design.h>
export module pchat.logic.doc;

import std;

export namespace pchat::logic {

// What a line is read as: the largest is a title, and a line of nothing is
// the room between two others -- a kind too.
enum class doc_kind {
  title,
  heading,
  subheading,
  bullet,
  words,
  gap,
};

struct doc_line {
  doc_kind kind = doc_kind::words;
  std::string said;
};

// How many hashes a line may open with and still be one of three sizes: more
// than three hashes reads as small as three.
inline constexpr std::size_t doc_deepest = 3;

// The list of them, read out of a whole document: every line kept in the
// order it was written, for a design read out of order is not the design
// written.
[[nodiscard]] inline std::vector<doc_line> read_doc(std::string_view text) {
  std::vector<doc_line> out;
  while (!text.empty()) {
    // One line: up to the next break, that break left behind it.
    const std::size_t broken = text.find('\n');
    std::string_view said = broken == std::string_view::npos
                                ? text
                                : text.substr(0u, broken);
    text = broken == std::string_view::npos ? std::string_view{}
                                            : text.substr(broken + 1u);
    if (!said.empty() && said.back() == '\r')
      said.remove_suffix(1u);
    // Empty but for spaces is empty.
    const std::size_t begun = said.find_first_not_of(" \t");
    if (begun == std::string_view::npos) {
      out.push_back(doc_line{.kind = doc_kind::gap});
      continue;
    }
    if (said[begun] == '#') {
      // How many hashes: a run of them, counted, nothing else in front.
      std::size_t hashes = 0u;
      while (begun + hashes < said.size() && said[begun + hashes] == '#')
        ++hashes;
      const std::size_t after = said.find_first_not_of(" \t", begun + hashes);
      if (after != std::string_view::npos) {
        doc_kind kind = doc_kind::subheading;
        if (hashes == 1u)
          kind = doc_kind::title;
        else if (hashes == 2u)
          kind = doc_kind::heading;
        out.push_back(doc_line{.kind = kind,
                               .said = std::string(said.substr(after))});
        continue;
      }
    }
    if (said[begun] == '*' || said[begun] == '-') {
      const std::size_t after = said.find_first_not_of(" \t", begun + 1u);
      if (after != std::string_view::npos && after > begun) {
        out.push_back(doc_line{.kind = doc_kind::bullet,
                               .said = std::string(said.substr(after))});
        continue;
      }
    }
    out.push_back(doc_line{.kind = doc_kind::words, .said = std::string(said)});
  }
  return out;
}

// The design itself, as this build was made with it: what stands at the top
// of the tree, shown in the room as well.
[[nodiscard]] inline std::string_view the_design() { return design_doc; }

} // namespace pchat::logic
