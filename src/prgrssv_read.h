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

}  // namespace pchat::prgrssv

#endif  // PCHAT_PRGRSSV_READ_H
