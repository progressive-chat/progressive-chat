// SPDX-License-Identifier: AGPL-3.0-only
// What a test reads an archive back with: the entries it holds, as plain names
// and bodies. The reader itself is the port's own and stays on the classic
// side (prgrssv_read.cpp), the same rule the program keeps its writer by
// (src/prgrssv_zip.h).
#ifndef PCHAT_PRGRSSV_READ_H
#define PCHAT_PRGRSSV_READ_H

#include <string>
#include <string_view>
#include <vector>

namespace pchat::test {

// One file as it stands in an archive: where it stands, and what it says.
struct zipped_entry {
  std::string name;
  std::string body;
};

// Every entry of an archive, in the order they stand in it. What the bytes are
// not an archive at all is said by the port's own word for it, thrown here.
std::vector<zipped_entry> read_entries(std::string_view bytes);

}  // namespace pchat::test

#endif  // PCHAT_PRGRSSV_READ_H
