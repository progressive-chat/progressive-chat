// SPDX-License-Identifier: AGPL-3.0-only
// A minimal test framework. SAID(group, name) defines a test, EXPECT_* checks
// one condition, run() runs every test and returns the exit code for ctest.
//
// A test file is a module unit. It includes this header in its global module
// fragment (before `export module`), because a module does not export macros.
#ifndef PCHAT_TESTS_H
#define PCHAT_TESTS_H

#include <cstdio>
#include <exception>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace pchat::tests {

// Failed checks, each as "file:line: text".
inline std::vector<std::string> &failures() {
  static std::vector<std::string> kept;
  return kept;
}
// Number of checks evaluated so far.
inline std::size_t &checks() {
  static std::size_t kept = 0;
  return kept;
}

inline void note_check() { ++checks(); }

inline void failed(std::string_view said, std::source_location where = std::source_location::current()) {
  failures().push_back(std::string{where.file_name()} + ":" + std::to_string(where.line()) + ": " +
                       std::string{said});
}

// One test: its function and its name. The name points at a string literal.
struct one {
  void (*said)() = nullptr;
  std::string_view name;
};

inline std::vector<one> &all() {
  static std::vector<one> kept;
  return kept;
}

// Adds a test to all() when constructed, which happens before main().
struct registrar {
  registrar(void (*said)(), std::string_view name) { all().push_back(one{said, name}); }
};

template <class Left, class Right>
void that(bool holds, std::string_view text, const Left &, const Right &) {
  note_check();
  if (!holds) failed(text);
}

// Runs every registered test. Returns 0 only if at least one check ran and
// none failed. A test that throws counts as failed and the run continues.
// The name is printed and flushed before the test starts, so the log shows
// which test was running if the program crashes.
inline int run() {
  for (const one &test : all()) {
    const std::size_t before = failures().size();
    std::printf("%s ... ", test.name.data());
    std::fflush(stdout);
    try {
      test.said();
    } catch (const std::exception &thrown) {
      failures().push_back(std::string{test.name} + ": threw " + thrown.what());
    } catch (...) {
      failures().push_back(std::string{test.name} + ": threw a value that is not a std::exception");
    }
    std::printf("%s\n", failures().size() == before ? "ok" : "FAIL");
    std::fflush(stdout);
  }
  if (all().empty() || checks() == 0) {
    std::printf("\nNo check ran: %zu tests registered, 0 checks evaluated.\n", all().size());
    return 1;
  }
  if (!failures().empty()) {
    std::printf("\n%zu of %zu checks did not hold:\n", failures().size(), checks());
    for (const std::string &said : failures()) std::printf("  %s\n", said.c_str());
    return 1;
  }
  std::printf("\n%zu checks held in %zu tests.\n", checks(), all().size());
  return 0;
}

}  // namespace pchat::tests

// Defines one test. The registrar must not be `static`: clang leaves an unused
// internal-linkage variable out of a module unit's object file, so the test
// was never registered and the program ran no test at all. Without `static`
// the variable has module linkage and is always emitted. Each test file is a
// separate program, so the names cannot collide across files.
#define SAID(group, name)                                                                                               \
  static void said_##group##_##name();                                                                                   \
  const ::pchat::tests::registrar said_kept_##group##_##name{&said_##group##_##name, #group "." #name};                 \
  static void said_##group##_##name()

// Each EXPECT_* evaluates its arguments twice, so do not pass an expression
// with a side effect.
#define EXPECT_TRUE(given) ::pchat::tests::that(static_cast<bool>(given), "true: " #given, given, true)
#define EXPECT_FALSE(given) ::pchat::tests::that(!static_cast<bool>(given), "false: " #given, given, false)
#define EXPECT_EQ(left, right) ::pchat::tests::that((left) == (right), #left " == " #right, left, right)
#define EXPECT_NE(left, right) ::pchat::tests::that(!((left) == (right)), #left " != " #right, left, right)
#define EXPECT_LT(left, right) ::pchat::tests::that((left) < (right), #left " < " #right, left, right)
#define EXPECT_GE(left, right) ::pchat::tests::that(!((left) < (right)), #left " >= " #right, left, right)

#endif
