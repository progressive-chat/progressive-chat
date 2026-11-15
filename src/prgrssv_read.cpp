// SPDX-License-Identifier: AGPL-3.0-only
// The read half of an archive: the port's own reader and the ported JSON
// reader, reached only from this file, so their headers stay out of module
// units the way they do everywhere else in this program (src/prgrssv_zip.cpp,
// the other half).
#include "prgrssv_read.h"

#include <cstdlib>
#include <optional>

#include "agora/core/json.hpp"
#include "agora/core/zip_reader.hpp"

namespace pchat::prgrssv {

namespace {

using agora::core::json::Kind;
using agora::core::json::Value;

// One object's member by its name, read through the ported reader's own walk
// (duplicates kept in source order; the first is the one that answers, as
// this tree reads a duplicated name everywhere else).
[[nodiscard]] const Value *at(const Value &one, std::string_view name) {
  if (one.kind != Kind::Object)
    return nullptr;
  for (const agora::core::json::Member &member : one.members)
    if (member.key == name)
      return &member.value;
  return nullptr;
}
// A field said as text: empty when it stands under another name or is not a
// string.
[[nodiscard]] std::string text_of(const Value &one, std::string_view name) {
  const Value *said = at(one, name);
  if (!said || said->kind != Kind::String)
    return {};
  return said->text;
}
// A field said as a whole number: empty when it stands under another name or
// is not a number.
[[nodiscard]] std::optional<std::int64_t> number_of(const Value &one,
                                                     std::string_view name) {
  const Value *said = at(one, name);
  // Numbers are what the ported reader calls a Literal: the raw run of
  // characters, read whole by strtod the way the ported field readers do.
  if (!said || said->kind != Kind::Literal)
    return std::nullopt;
  return static_cast<std::int64_t>(std::strtod(said->text.c_str(), nullptr));
}

}  // namespace

std::vector<held> read(std::string_view bytes) {
  agora::core::zip::Reader archive =
      agora::core::zip::Reader::from_bytes(std::string(bytes));
  std::vector<held> out;
  for (agora::core::zip::Entry &one : archive.entries())
    out.push_back(held{std::move(one.name), std::move(one.content)});
  return out;
}

preview preview_of(std::string_view bytes) {
  preview out;
  std::vector<held> files;
  try {
    files = read(bytes);
  } catch (const std::exception &) {
    return out;
  }
  out.is_archive = true;
  for (const held &one : files) {
    if (one.name.starts_with("conversations/") &&
        one.name.ends_with(".json")) {
      out.conversations += 1;
      const agora::core::json::ParseResult room =
          agora::core::json::parse(one.body);
      if (!room.value)
        continue;
      const Value *messages = at(*room.value, "messages");
      if (messages && messages->kind == Kind::Array)
        out.messages += static_cast<int>(messages->elements.size());
      continue;
    }
    if (one.name == "settings.json") {
      out.settings = true;
      continue;
    }
    if (one.name == "manifest.json") {
      const agora::core::json::ParseResult manifest =
          agora::core::json::parse(one.body);
      if (manifest.value && manifest.value->kind == Kind::Object) {
        out.format_version =
            static_cast<int>(number_of(*manifest.value, "version").value_or(0));
        out.program = text_of(*manifest.value, "program");
      }
    }
  }
  return out;
}

}  // namespace pchat::prgrssv
