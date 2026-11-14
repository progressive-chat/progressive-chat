// SPDX-License-Identifier: AGPL-3.0-only
// The ported tree's own store, reached through one plain header: open an empty
// database with the ported chat-dao schema on it, keep a conversation and a
// message the way ChatDao keeps them, and read them back. The schema is copied
// into this file by the build from the ported tree's own DDL (Room schema 46),
// because a program has no file beside it to read a schema from -- the same
// reason the vocabulary is carried as bytes.
//
// agora's headers are read here and nowhere else: the header above names only
// ids, titles and counts, so a module unit may include it.
#include "keep.h"

// The store stands only where the ported tree built its own database layer:
// AGORA_CORE_HAVE_SQLITE3 is set by the ported build when sqlite3 and the
// executor are there (a host), and is not set where they are not (a phone
// keeps Room, a web build has no file). Where it is not set the store says
// so at open() and keeps nothing, so the program builds everywhere.
#ifdef AGORA_CORE_HAVE_SQLITE3

#include <memory>
#include <sqlite3.h>
#include <vector>

#include "agora/core/chat_dao.hpp"
#include "agora/core/chat_entities.hpp"
#include "agora/core/sqlite_executor.hpp"

namespace {

// The schema, as the build wrote it: ports/agora-core/tools/golden/
// chat-dao/platform/schema.sql, verbatim.
const char kSchema[] =
    "-- Room-generated DDL for ChatDatabase schema 46 (entity createSql + indices), copied\n"
"-- verbatim from app/schemas/com.newoether.agora.data.local.ChatDatabase/46.json.\n"
"\n"
"CREATE TABLE IF NOT EXISTS `conversations` (`id` TEXT NOT NULL, `title` TEXT NOT NULL, `lastUpdated` INTEGER NOT NULL, `dataChangedAt` INTEGER NOT NULL, `selectedBranchesJson` TEXT, `systemPromptId` TEXT, `modelId` TEXT, `taskId` TEXT, `origin` TEXT NOT NULL, `graduated` INTEGER NOT NULL, `draftText` TEXT NOT NULL, `draftAttachments` TEXT, `selectedRunBranchesJson` TEXT, `hasUnreadGeneration` INTEGER NOT NULL, `isPinned` INTEGER NOT NULL DEFAULT 0, `todoJson` TEXT, `parentConversationId` TEXT, `createdAt` INTEGER NOT NULL DEFAULT 0, `subagentResultJson` TEXT, `subagentResultId` TEXT, `subagentResultStatus` TEXT, `subagentDeliveredResultId` TEXT, PRIMARY KEY(`id`));\n"
"CREATE INDEX IF NOT EXISTS `index_conversations_taskId` ON `conversations` (`taskId`);\n"
"CREATE INDEX IF NOT EXISTS `index_conversations_parentConversationId` ON `conversations` (`parentConversationId`);\n"
"CREATE INDEX IF NOT EXISTS `index_conversations_modelId` ON `conversations` (`modelId`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `runs` (`id` TEXT NOT NULL, `conversationId` TEXT NOT NULL, `parentRunId` TEXT, `status` TEXT NOT NULL, `activeSlot` INTEGER, `startedAt` INTEGER NOT NULL, `lastCheckpointAt` INTEGER NOT NULL, `stopRequestedAt` INTEGER, `endedAt` INTEGER, `endReason` TEXT, `currentPass` INTEGER NOT NULL, `legacyAmbiguous` INTEGER NOT NULL, PRIMARY KEY(`id`), FOREIGN KEY(`conversationId`) REFERENCES `conversations`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE , FOREIGN KEY(`parentRunId`) REFERENCES `runs`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"CREATE INDEX IF NOT EXISTS `index_runs_conversationId` ON `runs` (`conversationId`);\n"
"CREATE INDEX IF NOT EXISTS `index_runs_parentRunId` ON `runs` (`parentRunId`);\n"
"CREATE UNIQUE INDEX IF NOT EXISTS `index_runs_conversationId_activeSlot` ON `runs` (`conversationId`, `activeSlot`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `messages` (`id` TEXT NOT NULL, `conversationId` TEXT NOT NULL, `parentId` TEXT, `text` TEXT NOT NULL, `images` TEXT NOT NULL, `thoughts` TEXT, `thoughtTitle` TEXT, `tokenCount` INTEGER NOT NULL, `inputTokenCount` INTEGER, `cachedInputTokenCount` INTEGER, `cacheWriteInputTokenCount` INTEGER, `uncachedInputTokenCount` INTEGER, `outputTokenCount` INTEGER, `reasoningTokenCount` INTEGER, `generationDurationMs` INTEGER, `status` TEXT NOT NULL, `participant` TEXT NOT NULL, `timestamp` INTEGER NOT NULL, `thoughtTimeMs` INTEGER, `modelName` TEXT, `toolCallJson` TEXT, `attachmentMeta` TEXT, `runId` TEXT NOT NULL, `runSequence` INTEGER NOT NULL, `consumedAtPass` INTEGER, `sourceJson` TEXT, PRIMARY KEY(`id`), FOREIGN KEY(`conversationId`) REFERENCES `conversations`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE , FOREIGN KEY(`runId`) REFERENCES `runs`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"CREATE INDEX IF NOT EXISTS `index_messages_conversationId` ON `messages` (`conversationId`);\n"
"CREATE INDEX IF NOT EXISTS `index_messages_runId` ON `messages` (`runId`);\n"
"CREATE INDEX IF NOT EXISTS `index_messages_runId_runSequence` ON `messages` (`runId`, `runSequence`);\n"
"CREATE INDEX IF NOT EXISTS `index_messages_conversationId_id` ON `messages` (`conversationId`, `id`);\n"
"CREATE INDEX IF NOT EXISTS `index_messages_modelName_id_conversationId` ON `messages` (`modelName`, `id`, `conversationId`);\n"
"CREATE INDEX IF NOT EXISTS `index_messages_parentId` ON `messages` (`parentId`);\n"
"CREATE INDEX IF NOT EXISTS `index_messages_id_images_attachmentMeta` ON `messages` (`id`, `images`, `attachmentMeta`);\n"
"CREATE INDEX IF NOT EXISTS `index_messages_timestamp_id` ON `messages` (`timestamp`, `id`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `embeddings` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `messageId` TEXT NOT NULL, `modelId` TEXT NOT NULL, `embedding` BLOB NOT NULL, `chunkText` TEXT NOT NULL, `dimension` INTEGER NOT NULL, `sourceFingerprint` TEXT, `generation` INTEGER NOT NULL DEFAULT 0);\n"
"CREATE UNIQUE INDEX IF NOT EXISTS `index_embeddings_messageId_modelId` ON `embeddings` (`messageId`, `modelId`);\n"
"CREATE INDEX IF NOT EXISTS `index_embeddings_modelId` ON `embeddings` (`modelId`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `tasks` (`id` TEXT NOT NULL, `name` TEXT NOT NULL, `prompt` TEXT NOT NULL, `systemPrompt` TEXT, `systemPromptId` TEXT, `modelId` TEXT, `cronExpr` TEXT NOT NULL, `runAt` INTEGER, `nextRunAt` INTEGER NOT NULL, `enabled` INTEGER NOT NULL, `createdAt` INTEGER NOT NULL, `lastRunAt` INTEGER, PRIMARY KEY(`id`));\n"
"\n"
"CREATE TABLE IF NOT EXISTS `loops` (`conversationId` TEXT NOT NULL, `intervalMs` INTEGER NOT NULL, `prompt` TEXT, `nextFireAt` INTEGER NOT NULL, `cycleCount` INTEGER NOT NULL, `maxCycles` INTEGER, `active` INTEGER NOT NULL, `revision` INTEGER NOT NULL, PRIMARY KEY(`conversationId`), FOREIGN KEY(`conversationId`) REFERENCES `conversations`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"\n"
"CREATE TABLE IF NOT EXISTS `new_chat_persist` (`id` INTEGER NOT NULL, `modelId` TEXT, `systemPromptId` TEXT, `conversationSettingsJson` TEXT, `draftText` TEXT NOT NULL, `draftAttachments` TEXT, PRIMARY KEY(`id`));\n"
"\n"
"CREATE TABLE IF NOT EXISTS `conversation_settings_transfer` (`conversationId` TEXT NOT NULL, `settingsJson` TEXT, PRIMARY KEY(`conversationId`), FOREIGN KEY(`conversationId`) REFERENCES `conversations`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"\n"
"CREATE TABLE IF NOT EXISTS `conversation_settings_import_transfer` (`id` INTEGER NOT NULL, `transferId` TEXT NOT NULL, `settingsJson` TEXT NOT NULL, `mode` TEXT NOT NULL, PRIMARY KEY(`id`));\n"
"\n"
"CREATE TABLE IF NOT EXISTS `maintenance_debt` (`kind` TEXT NOT NULL, `identity` TEXT NOT NULL, `state` TEXT NOT NULL, `revision` INTEGER NOT NULL, `updatedAt` INTEGER NOT NULL, `claimId` TEXT, `claimedAt` INTEGER, PRIMARY KEY(`kind`, `identity`));\n"
"CREATE INDEX IF NOT EXISTS `index_maintenance_debt_state_updatedAt_kind_identity` ON `maintenance_debt` (`state`, `updatedAt`, `kind`, `identity`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `semantic_index_ledger` (`modelId` TEXT NOT NULL, `state` TEXT NOT NULL, `sourceRevision` INTEGER NOT NULL, `completedRevision` INTEGER NOT NULL, `reconcileRevision` INTEGER NOT NULL, `updatedAt` INTEGER NOT NULL, `generation` INTEGER NOT NULL DEFAULT 0, PRIMARY KEY(`modelId`));\n"
"\n"
"CREATE TABLE IF NOT EXISTS `semantic_index_work` (`modelId` TEXT NOT NULL, `messageId` TEXT NOT NULL, `sourceFingerprint` TEXT, `sourceRevision` INTEGER NOT NULL, `updatedAt` INTEGER NOT NULL, PRIMARY KEY(`modelId`, `messageId`), FOREIGN KEY(`modelId`) REFERENCES `semantic_index_ledger`(`modelId`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"CREATE INDEX IF NOT EXISTS `index_semantic_index_work_modelId_sourceRevision_messageId` ON `semantic_index_work` (`modelId`, `sourceRevision`, `messageId`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `semantic_sources` (`messageId` TEXT NOT NULL, `conversationId` TEXT NOT NULL, `eligible` INTEGER NOT NULL, `sourcePrefix` BLOB NOT NULL, `sourceFingerprint` TEXT, `searchLength` INTEGER NOT NULL, `historyEligible` INTEGER NOT NULL, PRIMARY KEY(`messageId`), FOREIGN KEY(`conversationId`) REFERENCES `conversations`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"CREATE INDEX IF NOT EXISTS `index_semantic_sources_conversationId_messageId` ON `semantic_sources` (`conversationId`, `messageId`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `semantic_conversations` (`conversationId` TEXT NOT NULL, `sourceRevision` INTEGER NOT NULL, `eligibleCount` INTEGER NOT NULL, `cacheEligible` INTEGER NOT NULL, PRIMARY KEY(`conversationId`), FOREIGN KEY(`conversationId`) REFERENCES `conversations`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"\n"
"CREATE TABLE IF NOT EXISTS `semantic_model_conversations` (`modelId` TEXT NOT NULL, `conversationId` TEXT NOT NULL, `cachedCount` INTEGER NOT NULL, `indexedRevision` INTEGER NOT NULL, PRIMARY KEY(`modelId`, `conversationId`), FOREIGN KEY(`conversationId`) REFERENCES `conversations`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"CREATE INDEX IF NOT EXISTS `index_semantic_model_conversations_conversationId` ON `semantic_model_conversations` (`conversationId`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `semantic_metadata_state` (`id` INTEGER NOT NULL, `complete` INTEGER NOT NULL, `afterMessageId` TEXT, PRIMARY KEY(`id`));\n"
"\n"
"CREATE TABLE IF NOT EXISTS `attachment_reference_owners` (`kind` INTEGER NOT NULL, `ownerId` TEXT NOT NULL, `token` TEXT NOT NULL, `dirty` INTEGER NOT NULL, `invalid` INTEGER NOT NULL, PRIMARY KEY(`kind`, `ownerId`));\n"
"CREATE INDEX IF NOT EXISTS `index_attachment_reference_owners_dirty_kind_ownerId` ON `attachment_reference_owners` (`dirty`, `kind`, `ownerId`);\n"
"CREATE INDEX IF NOT EXISTS `index_attachment_reference_owners_invalid_kind_ownerId` ON `attachment_reference_owners` (`invalid`, `kind`, `ownerId`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `attachment_references` (`kind` INTEGER NOT NULL, `ownerId` TEXT NOT NULL, `path` TEXT NOT NULL, PRIMARY KEY(`kind`, `ownerId`, `path`), FOREIGN KEY(`kind`, `ownerId`) REFERENCES `attachment_reference_owners`(`kind`, `ownerId`) ON UPDATE NO ACTION ON DELETE CASCADE );\n"
"CREATE INDEX IF NOT EXISTS `index_attachment_references_path` ON `attachment_references` (`path`);\n"
"\n"
"CREATE TABLE IF NOT EXISTS `attachment_reference_state` (`id` INTEGER NOT NULL, `phase` INTEGER NOT NULL, `afterId` TEXT, `revision` INTEGER NOT NULL, PRIMARY KEY(`id`));\n"
"";

std::unique_ptr<agora::core::SqliteExecutor> the_store;
agora::core::chat_dao::ChatDao *the_dao = nullptr;

}  // namespace

namespace pchat::keep {

bool open() {
  if (the_store)
    return true;
  // An empty store of this process: no path is asked of it, so nothing can be
  // lost and nothing else can be read. The schema is applied whole, the way
  // the ported stand applies it before its own tests write a row.
  auto made = std::make_unique<agora::core::SqliteExecutor>(":memory:");
  char *error = nullptr;
  if (sqlite3_exec(made->handle(), kSchema, nullptr, nullptr, &error) !=
      SQLITE_OK) {
    sqlite3_free(error);
    return false;
  }
  the_dao = new agora::core::chat_dao::ChatDao(*made);
  the_store = std::move(made);
  return true;
}

bool is_open() { return the_dao != nullptr; }

void close() {
  delete the_dao;
  the_dao = nullptr;
  the_store.reset();
}

void keep_conversation(conversation_row one) {
  if (!the_dao)
    return;
  agora::core::ChatEntity entity;
  entity.id = one.id;
  entity.title = one.title;
  entity.last_updated = one.last_updated;
  the_dao->upsert_conversation(entity);
}

void keep_message(message_row one) {
  if (!the_dao)
    return;
  agora::core::MessageEntity entity;
  entity.id = one.id;
  entity.conversation_id = one.conversation_id;
  entity.text = one.text;
  entity.token_count = one.token_count;
  entity.timestamp = one.timestamp;
  the_dao->upsert_message(entity);
}

std::vector<conversation_row> conversations() {
  std::vector<conversation_row> out;
  if (!the_dao)
    return out;
  for (const agora::core::chat_dao::ChatConversation &each : the_dao->get_all_conversations())
    out.push_back(conversation_row{each.id, each.title, 0});
  return out;
}

std::vector<message_row> messages_of(const std::string &conversation_id) {
  std::vector<message_row> out;
  if (!the_dao)
    return out;
  for (const agora::core::MessageEntity &each :
       the_dao->get_conversation_messages_page(conversation_id, std::nullopt,
                                               1000))
    out.push_back(message_row{each.id, each.conversation_id, each.text,
                              each.token_count, each.timestamp});
  return out;
}

}  // namespace pchat::keep

#else  // AGORA_CORE_HAVE_SQLITE3

// No ported database layer in this build: the store says it is closed.
namespace pchat::keep {

bool open() { return false; }

bool is_open() { return false; }

void close() {}

void keep_conversation(conversation_row) {}

void keep_message(message_row) {}

std::vector<conversation_row> conversations() { return {}; }

std::vector<message_row> messages_of(const std::string &) { return {}; }

}  // namespace pchat::keep

#endif  // AGORA_CORE_HAVE_SQLITE3
