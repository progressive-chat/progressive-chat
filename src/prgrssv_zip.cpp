// SPDX-License-Identifier: AGPL-3.0-only
// The zipped half of an archive: the port's own writer, reached only from this
// file, so its headers stay out of module units the way they do
// everywhere else in this program (src/tokens_worth.cpp, the same rule).
#include "prgrssv_zip.h"

#include "agora/core/zip_writer.hpp"

namespace pchat::prgrssv {

std::string zipped(std::span<const stored> files) {
  agora::core::zip::Writer zip;
  for (const stored &one : files)
    zip.put_entry(one.name, one.body);
  return zip.finish();
}

}  // namespace pchat::prgrssv
