// SPDX-License-Identifier: AGPL-3.0-only
// The definition behind pchat.version. PCHAT_VERSION_SAID is set for this
// file only (CMakeLists.txt) and changes with every commit.
module pchat.version;

import std;

namespace pchat {

std::string_view build_version() { return PCHAT_VERSION_SAID; }

}  // namespace pchat
