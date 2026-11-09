// SPDX-License-Identifier: AGPL-3.0-only
// The zipped half of an archive: the port's own writer, reached only from this
// file, so that its headers stay out of module units the way they do
// everywhere else in this program (src/tokens_worth.cpp, the same rule).
#include "prgrssv_zip.h"

#include <exception>

#include "agora/core/zip_reader.hpp"
#include "agora/core/zip_writer.hpp"

namespace pchat::prgrssv {

std::string zipped(std::span<const stored> files) {
  agora::core::zip::Writer zip;
  for (const stored &one : files)
    zip.put_entry(one.name, one.body);
  return zip.finish();
}

bool unzipped(std::string_view bytes, std::vector<stored> &out) {
  // Read whole, not appended to: what comes back is only ever this archive's
  // own files -- said here because a check names its expression twice
  // (test/tests.h, EXPECT_TRUE), and a reader that appended would answer the
  // second asking with the files twice over.
  out.clear();
  try {
    agora::core::zip::Reader archive =
        agora::core::zip::Reader::from_bytes(std::string(bytes));
    for (agora::core::zip::Entry &one : archive.entries())
      out.push_back(stored{std::move(one.name), std::move(one.content)});
  } catch (const std::exception &) {
    // What the port's reader says of bytes it cannot read: said as false here,
    // and nowhere above does anything have to know that it threw.
    out.clear();
    return false;
  }
  return !out.empty();
}

}  // namespace pchat::prgrssv
