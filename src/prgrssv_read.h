// SPDX-License-Identifier: AGPL-3.0-only
// The read half of an archive, beside the write half: the port's own reader,
// reached only from this file, so its headers stay out of module units the
// way the writer's do (src/prgrssv_zip.h, the same rule). What an archive
// holds, one file at a time, in the order they stand in it.
#ifndef PCHAT_PRGRSSV_READ_H
#define PCHAT_PRGRSSV_READ_H

#include <string>
#include <string_view>
#include <vector>

namespace pchat::prgrssv {

// One file as it stands in an archive: where it stands, and what it says.
struct held {
  std::string name;
  std::string body;
};

// Every file of an archive, in the order they stand in it. What the bytes
// are not an archive at all is said by the port's own word for it, thrown
// here -- nothing catches it above, because a file that is not one is not
// read as one.
[[nodiscard]] std::vector<held> read(std::string_view bytes);

// What an archive holds, said before it is read whole: the manifest's own
// words and how much of everything stands behind them -- the counts read off
// the file names and the one index, the way the ported importer's preview
// reads a native backup (DataImporter.preview: manifest, then counts, no
// conversation opened). A file that is not one of these archives says
// nothing -- every count zero, `is_archive` false.
struct preview {
  bool is_archive = false;
  int format_version = 0;
  std::string program;   // what wrote it, as the manifest says
  int conversations = 0;  // every conversations/*.json
  int messages = 0;       // every message inside them, counted in the JSON
  bool settings = false;  // the page's settings, present or not
};

// What an archive holds, without reading any conversation back: the manifest
// first (the file that says what the rest are), then the counts. The bytes
// are read once; nothing of them is kept.
[[nodiscard]] preview preview_of(std::string_view bytes);

}  // namespace pchat::prgrssv

#endif  // PCHAT_PRGRSSV_READ_H
