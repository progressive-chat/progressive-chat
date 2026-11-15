// SPDX-License-Identifier: AGPL-3.0-only
// The read half of an archive: the port's own reader, reached only from this
// file, so its headers stay out of module units the way they do everywhere
// else in this program (src/prgrssv_zip.cpp, the other half).
#include "prgrssv_read.h"

#include "agora/core/zip_reader.hpp"

namespace pchat::prgrssv {

std::vector<held> read(std::string_view bytes) {
  agora::core::zip::Reader archive =
      agora::core::zip::Reader::from_bytes(std::string(bytes));
  std::vector<held> out;
  for (agora::core::zip::Entry &one : archive.entries())
    out.push_back(held{std::move(one.name), std::move(one.content)});
  return out;
}

}  // namespace pchat::prgrssv
