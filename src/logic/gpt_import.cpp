// SPDX-License-Identifier: AGPL-3.0-only
// The reading itself: the ported importer of a ChatGPT export does the whole
// of it -- the zip or the bare array, the graph of the mapping, the branches
// it keeps -- and this file carries what comes out of it into the words the
// lines above say. Agora's headers are seen here and nowhere else, which is
// the arrangement of every classic unit that stands on the ported tree
// (src/provider/budget.cpp, src/provider/http/wire.cpp).
#include "gpt_import.h"

#include <utility>

#include "agora/core/gpt_chat_importer.hpp"

namespace pchat::logic {

gpt_extracted gpt_extract(std::string_view bytes) {
  gpt_extracted out;
  const agora::core::gpt_import::Extraction read =
      agora::core::gpt_import::extract_and_parse(std::string(bytes));
  if (!read.ok) {
    out.failure = read.failure.type;
    if (!read.failure.message.empty())
      out.failure += ": " + read.failure.message;
    return out;
  }
  const agora::core::claude_import::ImportConversations flat =
      agora::core::gpt_import::to_import_format(read.conversations);
  out.ok = true;
  out.conversations.reserve(flat.conversations.size());
  for (const agora::core::claude_import::ImportChatEntity &one :
       flat.conversations)
    out.conversations.push_back(gpt_conversation{one.id, one.title,
                                                 one.last_updated});
  out.messages.reserve(flat.messages.size());
  for (const agora::core::claude_import::ImportMessageEntity &one :
       flat.messages) {
    // Whose it is, said once here: the importer has already said USER and
    // MODEL of every message, and this program names the two by which side
    // of the room they stand on. What a message was also worth to the
    // importer -- a status, a model's own name beside it -- is left where
    // the importer kept it: nothing of either is a field of a message here.
    out.messages.push_back(gpt_message{
        .conversation = one.conversation_id,
        .id = one.id,
        .text = one.text,
        .from_reader = one.participant == "USER",
        .when = one.timestamp,
        .tokens = static_cast<std::int32_t>(one.token_count),
        .thought = one.thoughts.value_or(std::string{}),
        .thought_ms = one.thought_time_ms.value_or(0),
    });
  }
  return out;
}

}  // namespace pchat::logic
