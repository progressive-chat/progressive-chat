// SPDX-License-Identifier: AGPL-3.0-only
// What one .prgrssv archive is put together with, said across the one line a
// module unit and a classic translation unit may share: the files it holds,
// and those files zipped into one. The port's own writer stands behind this
// file and stays on the classic side entirely (src/prgrssv_zip.cpp); what
// crosses into a module unit is only these plain types and nothing of its
// headers (src/provider/anthropic/wire.h does it the same way).
#ifndef PCHAT_PRGRSSV_ZIP_H
#define PCHAT_PRGRSSV_ZIP_H

#include <span>
#include <string>
#include <string_view>

namespace pchat::prgrssv {

// One file inside an archive: where it stands in it, and what it says.
struct stored {
  std::string name;
  std::string body;
};

// Every file in, the whole archive out: a ZIP whose entries are deflated the
// way the port's writer deflates them, so that the reader beside it -- the same
// port's -- reads back everything written here (test/prgrssv_test.cc).
[[nodiscard]] std::string zipped(std::span<const stored> files);

}  // namespace pchat::prgrssv

#endif  // PCHAT_PRGRSSV_ZIP_H
