# The bridge of names

Agora and this tree call the same things by different names often enough
that counting coverage by spelling alone calls a carried thing missing. This
table says, once, what each side calls the forty-odd things both have; the
count tool (`tools/agora_coverage.py`) reads it and counts a name carried
where either side's word stands in `src/` or `test/`.

The left column is the name as it stands in the port of Agora
(`ports/agora-core`), the right as it stands here. Where one name here answers
several there, each of them says the same right-hand side.

| Agora | here |
| --- | --- |
| `ChatEntity` | `conversation` |
| `ConversationEntry` | `conversation` |
| `DrawerConversationRow` | `conversation` |
| `SearchableConversation` | `conversation` |
| `MessageEntity` | `message` |
| `MessageRecord` | `message` |
| `MessageInfo` | `message` |
| `IndexableMessage` | `message` |
| `ContextWindowUsage` | `context_used` |
| `ContextCostModel` | `budget` |
| `FixedContextComposition` | `budget` |
| `ContextRetentionSplit` | `wire_view` |
| `ContextCompactDao` | `capsule` |
| `ChatDao` | `prgrssv` |
| `FileStore` | `storage` |
| `SettingsStore` | `settings` |
| `SettingsSnapshot` | `settings` |
| `SettingsView` | `settings` |
| `SettingsWrites` | `settings` |
| `PreferenceStore` | `looks` |
| `SnapshotStore` | `archive` |
| `NativeBackupArchive` | `archive` |
| `ProviderConfig` | `door` |
| `CustomProviderConfig` | `door` |
| `ApiKeyEntry` | `door_key` |
| `Provider` | `provider` |
| `ModelId` | `model` |
| `SavedSystemPrompt` | `told` |
| `SystemPromptEntry` | `told` |
| `ApiGenerationConfig` | `asking` |
| `ApiToolConfig` | `tools` |
| `RunEntity` | `run` |
| `LoopEntity` | `loop` |
| `AttachmentItem` | `attachment` |
| `ClaudeAttachment` | `attachment` |
| `SelectedAttachment` | `attachment` |
| `ExportRequest` | `save_archive` |
| `ImportConversations` | `load_archive` |
| `DataImporter` | `load_archive` |
| `ChatSearchDao` | `message_matches` |
| `Transport` | `carry` |
| `ProxyConfig` | `proxy` |
| `Member` | `account` |
| `TextCounter` | `tokens` |
| `TokenUsage` | `usage` |
| `ToolProvider` | `tools` |
