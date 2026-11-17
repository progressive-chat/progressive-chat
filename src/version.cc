// SPDX-License-Identifier: AGPL-3.0-only
// pchat.version -- the version string of this build.
//
// The interface holds only the declaration. The definition is in
// src/version_impl.cc, the one file compiled with the commit id, so a new
// commit recompiles that file and nothing that imports this module.
export module pchat.version;

import std;

export namespace pchat {

// "0.1-<commit>" when the build knows its commit, otherwise "0.1".
std::string_view build_version();

}  // namespace pchat
