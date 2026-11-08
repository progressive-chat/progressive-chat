// SPDX-License-Identifier: AGPL-3.0-only
// The reading half of an archive test: the port's reader, reached only from
// this file, so that its headers stay out of module units as they do
// everywhere else in this program (src/prgrssv_zip.cpp, the other half).
#include "prgrssv_read.h"

#include "agora/core/zip_reader.hpp"

namespace pchat::test {

std::vector<zipped_entry> read_entries(std::string_view bytes) {
  agora::core::zip::Reader archive =
      agora::core::zip::Reader::from_bytes(std::string(bytes));
  std::vector<zipped_entry> out;
  for (agora::core::zip::Entry &one : archive.entries())
    out.push_back(zipped_entry{std::move(one.name), std::move(one.content)});
  return out;
}

}  // namespace pchat::test
