#include "agora/core/data_importer.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "agora/core/json.hpp"
#include "agora/core/loop_policy.hpp"
#include "agora/core/native_backup_format.hpp"
#include "agora/core/native_backup_archive.hpp"
#include "agora/core/portable_settings_archive.hpp"
#include "agora/core/export_extra_settings.hpp"
#include "agora/core/native_backup_secrets.hpp"
#include "agora/core/native_conversation_graph_importer.hpp"
#include "agora/core/native_conversation_graph_source.hpp"
#include "agora/core/android_json.hpp"

namespace agora::core::data_import {

namespace {

using json::Value;
using json::Kind;
using json::ParseResult;
using json::Member;

// Adapter to use NativeBackupArchive as a graph source EntrySource
class ArchiveEntrySource final : public native_graph::EntrySource {
public:
    explicit ArchiveEntrySource(native_backup_archive::NativeBackupArchive& archive) : archive_(archive) {}

    bool has(std::string_view name) override { return archive_.has(name); }

    std::optional<std::string> bytes(std::string_view name) override { return archive_.bytes(name); }

    std::unique_ptr<android_json::Source> stream(std::string_view name) override {
        auto entry_stream = archive_.stream(name);
        if (!entry_stream) return nullptr;
        return std::make_unique<EntryStreamAdapter>(std::move(entry_stream));
    }

private:
    // Adapter from NativeBackupArchive::EntryStream to android_json::Source
    class EntryStreamAdapter final : public android_json::Source {
    public:
        explicit EntryStreamAdapter(std::unique_ptr<native_backup_archive::EntryStream> stream)
            : stream_(std::move(stream)) {}

        long long read(char* buffer, long long capacity) override {
            return stream_->read(buffer, capacity);
        }

        void close() override {
            stream_->close();
        }

    private:
        std::unique_ptr<native_backup_archive::EntryStream> stream_;
    };

    native_backup_archive::NativeBackupArchive& archive_;
};

const Value* field(const Value& object, std::string_view name) {
    if (object.kind != Kind::Object) return nullptr;
    for (const Member& member : object.members) {
        if (member.key == name) return &member.value;
    }
    return nullptr;
}

std::optional<std::string> string_field(const Value& object, std::string_view name) {
    const Value* value = field(object, name);
    if (value == nullptr) return std::nullopt;
    if (value->kind != Kind::String) return std::nullopt;
    return value->text;
}

std::optional<bool> boolean_field(const Value& object, std::string_view name) {
    const Value* value = field(object, name);
    if (value == nullptr) return std::nullopt;
    if (value->kind != Kind::Literal) return std::nullopt;
    if (value->text == "true") return true;
    if (value->text == "false") return false;
    return std::nullopt;
}

std::optional<int> int_field(const Value& object, std::string_view name) {
    const Value* value = field(object, name);
    if (value == nullptr) return std::nullopt;
    if (value->kind == Kind::Literal) {
        try {
            return std::stoi(value->text);
        } catch (...) {
            return std::nullopt;
        }
    }
    if (value->kind == Kind::String) {
        try {
            return std::stoi(value->text);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<float> float_field(const Value& object, std::string_view name) {
    const Value* value = field(object, name);
    if (value == nullptr) return std::nullopt;
    if (value->kind == Kind::Literal || value->kind == Kind::String) {
        try {
            return std::stof(value->text);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::vector<std::string> decode_string_array(const Value* value) {
    std::vector<std::string> result;
    if (value == nullptr || value->kind != Kind::Array) return result;
    for (const Value& element : value->elements) {
        if (element.kind == Kind::String) result.push_back(element.text);
    }
    return result;
}

// EmbeddingModelConfig semantics comparison
[[maybe_unused]] bool embedding_model_semantics_changed(const native_backup::EmbeddingModelConfig& before,
                                       const native_backup::EmbeddingModelConfig& after) {
    if (before.type != after.type) return true;
    switch (after.type) {
        case native_backup::EmbeddingModelType::Remote:
            return before.remote_model_name != after.remote_model_name ||
                   before.remote_base_url != after.remote_base_url;
        case native_backup::EmbeddingModelType::Local:
            return before.local_file_path != after.local_file_path;
    }
    return false;
}

// SystemPromptEntry equality
[[maybe_unused]] bool system_prompt_equal(const data_export::SystemPromptEntry& a,
                         const data_export::SystemPromptEntry& b) {
    if (a.id != b.id || a.title != b.title || a.content != b.content) return false;
    if (a.system_items.size() != b.system_items.size()) return false;
    if (a.user_items.size() != b.user_items.size()) return false;
    if (a.assistant_items.size() != b.assistant_items.size()) return false;
    if (a.user_prepend_items.size() != b.user_prepend_items.size()) return false;
    if (a.user_postpend_items.size() != b.user_postpend_items.size()) return false;
    for (size_t i = 0; i < a.system_items.size(); ++i) {
        if (a.system_items[i].id != b.system_items[i].id ||
            a.system_items[i].type != b.system_items[i].type ||
            a.system_items[i].value != b.system_items[i].value) return false;
    }
    for (size_t i = 0; i < a.user_items.size(); ++i) {
        if (a.user_items[i].id != b.user_items[i].id ||
            a.user_items[i].type != b.user_items[i].type ||
            a.user_items[i].value != b.user_items[i].value) return false;
    }
    for (size_t i = 0; i < a.assistant_items.size(); ++i) {
        if (a.assistant_items[i].id != b.assistant_items[i].id ||
            a.assistant_items[i].type != b.assistant_items[i].type ||
            a.assistant_items[i].value != b.assistant_items[i].value) return false;
    }
    for (size_t i = 0; i < a.user_prepend_items.size(); ++i) {
        if (a.user_prepend_items[i].id != b.user_prepend_items[i].id ||
            a.user_prepend_items[i].type != b.user_prepend_items[i].type ||
            a.user_prepend_items[i].value != b.user_prepend_items[i].value) return false;
    }
    for (size_t i = 0; i < a.user_postpend_items.size(); ++i) {
        if (a.user_postpend_items[i].id != b.user_postpend_items[i].id ||
            a.user_postpend_items[i].type != b.user_postpend_items[i].type ||
            a.user_postpend_items[i].value != b.user_postpend_items[i].value) return false;
    }
    return true;
}

}  // namespace

// ============================================================================
// ConversationSettingsTransfer
// ============================================================================

ConversationSettingsTransfer::ConversationSettingsTransfer(
    chat_dao::ChatDao& dao,
    std::function<void()> await_initial_load,
    std::function<void(const std::vector<std::pair<std::string, export_extra_settings::ConversationSettings>>&, bool)> apply_import)
    : dao_(dao),
      await_initial_load_(std::move(await_initial_load)),
      apply_import_(std::move(apply_import)) {}

void ConversationSettingsTransfer::complete_pending_import() {
    await_initial_load_();
    auto transfer = dao_.get_conversation_settings_import_transfer();
    if (!transfer) return;

    ParseResult pr = json::parse(transfer->settings_json);
    if (!pr.value || pr.value->kind != Kind::Object) return;

    std::vector<std::pair<std::string, export_extra_settings::ConversationSettings>> imported;
    for (const Member& member : pr.value->members) {
        export_extra_settings::ConversationSettings settings;
        if (member.value.kind == Kind::Object) {
            const Value& o = member.value;
            if (auto v = int_field(o, "contextWindow")) settings.context_window = *v;
            if (auto v = float_field(o, "temperature")) settings.temperature = *v;
            if (auto v = int_field(o, "maxTokens")) settings.max_tokens = *v;
            if (auto v = float_field(o, "topP")) settings.top_p = *v;
            if (auto v = float_field(o, "frequencyPenalty")) settings.frequency_penalty = *v;
            if (auto v = float_field(o, "presencePenalty")) settings.presence_penalty = *v;
            if (auto v = boolean_field(o, "codeExecutionEnabled")) settings.code_execution_enabled = *v;
            if (auto v = boolean_field(o, "googleSearchEnabled")) settings.google_search_enabled = *v;
            if (auto v = boolean_field(o, "openAiWebSearchEnabled")) settings.open_ai_web_search_enabled = *v;
            if (auto v = boolean_field(o, "thinkingEnabled")) settings.thinking_enabled = *v;
            if (auto v = string_field(o, "thinkingLevel")) settings.thinking_level = *v;
            if (auto v = boolean_field(o, "thinkingBudgetEnabled")) settings.thinking_budget_enabled = *v;
            if (auto v = int_field(o, "thinkingBudgetTokens")) settings.thinking_budget_tokens = *v;
            if (auto v = boolean_field(o, "openAiServiceTierEnabled")) settings.open_ai_service_tier_enabled = *v;
            if (auto v = string_field(o, "openAiServiceTier")) settings.open_ai_service_tier = *v;
            if (auto v = boolean_field(o, "webSearchEnabled")) settings.web_search_enabled = *v;
            if (auto v = boolean_field(o, "shellEnabled")) settings.shell_enabled = *v;
            if (auto v = boolean_field(o, "lowContextModeEnabled")) settings.low_context_mode_enabled = *v;
        }
        imported.emplace_back(member.key, settings);
    }

    apply_import_(imported, transfer->mode == "REPLACE");
    dao_.delete_conversation_settings_import_transfer(transfer->transfer_id);
}

void ConversationSettingsTransfer::complete_import(const std::string& transfer_id) {
    await_initial_load_();
    auto transfer = dao_.get_conversation_settings_import_transfer();
    if (!transfer || transfer->transfer_id != transfer_id) return;

    ParseResult pr = json::parse(transfer->settings_json);
    if (!pr.value || pr.value->kind != Kind::Object) return;

    std::vector<std::pair<std::string, export_extra_settings::ConversationSettings>> imported;
    for (const Member& member : pr.value->members) {
        export_extra_settings::ConversationSettings settings;
        if (member.value.kind == Kind::Object) {
            const Value& o = member.value;
            if (auto v = int_field(o, "contextWindow")) settings.context_window = *v;
            if (auto v = float_field(o, "temperature")) settings.temperature = *v;
            if (auto v = int_field(o, "maxTokens")) settings.max_tokens = *v;
            if (auto v = float_field(o, "topP")) settings.top_p = *v;
            if (auto v = float_field(o, "frequencyPenalty")) settings.frequency_penalty = *v;
            if (auto v = float_field(o, "presencePenalty")) settings.presence_penalty = *v;
            if (auto v = boolean_field(o, "codeExecutionEnabled")) settings.code_execution_enabled = *v;
            if (auto v = boolean_field(o, "googleSearchEnabled")) settings.google_search_enabled = *v;
            if (auto v = boolean_field(o, "openAiWebSearchEnabled")) settings.open_ai_web_search_enabled = *v;
            if (auto v = boolean_field(o, "thinkingEnabled")) settings.thinking_enabled = *v;
            if (auto v = string_field(o, "thinkingLevel")) settings.thinking_level = *v;
            if (auto v = boolean_field(o, "thinkingBudgetEnabled")) settings.thinking_budget_enabled = *v;
            if (auto v = int_field(o, "thinkingBudgetTokens")) settings.thinking_budget_tokens = *v;
            if (auto v = boolean_field(o, "openAiServiceTierEnabled")) settings.open_ai_service_tier_enabled = *v;
            if (auto v = string_field(o, "openAiServiceTier")) settings.open_ai_service_tier = *v;
            if (auto v = boolean_field(o, "webSearchEnabled")) settings.web_search_enabled = *v;
            if (auto v = boolean_field(o, "shellEnabled")) settings.shell_enabled = *v;
            if (auto v = boolean_field(o, "lowContextModeEnabled")) settings.low_context_mode_enabled = *v;
        }
        imported.emplace_back(member.key, settings);
    }

    apply_import_(imported, transfer->mode == "REPLACE");
    dao_.delete_conversation_settings_import_transfer(transfer_id);
}

// ============================================================================
// DataImporter
// ============================================================================

DataImporter::DataImporter(
    chat_dao::ChatDao& chat,
    semantic_index::SemanticIndexDao& semantic,
    maintenance_debt::MaintenanceDebtDao& debt,
    SqlExecutor& sql,
    memory::MemoryManager& memory,
    skill::SkillManager& skill,
    SettingsView settings_view,
    SettingsWrites settings_writes,
    ConversationSettingsTransfer& conversation_settings,
    Platform platform)
    : chat_(chat),
      semantic_(semantic),
      debt_(debt),
      sql_(sql),
      memory_(memory),
      skill_(skill),
      settings_(std::move(settings_view)),
      writes_(std::move(settings_writes)),
      transfers_(conversation_settings),
      platform_(std::move(platform)),
      graph_source_(),
      graph_importer_(chat, semantic, debt, sql, native_graph_import::ImportCallbacks{
          // resolveSystemPromptId: null-preserving; a prompt id the plan remapped
          // maps through, an unknown one passes through unchanged.
          [](const std::optional<std::string>& id) { return id; },
          [this] { return platform_.new_uuid ? platform_.new_uuid() : std::string(); },
          // A fixed wall clock; the golden masks it as <clock>.
          [] { return static_cast<std::int64_t>(1790000000000LL); },
          [] {},
      }) {}

ImportPreview DataImporter::preview(native_backup_archive::NativeBackupArchive& archive) {
    ImportPreview empty;
    empty.manifest.version = 0;

    auto manifest_bytes = archive.bytes(native_backup::kManifestEntry);
    if (!manifest_bytes) return empty;

    ParseResult pr = json::parse(*manifest_bytes);
    if (!pr.value) return empty;

    ImportManifest manifest;
    const Value& root = *pr.value;
    manifest.version = int_field(root, "agora_export_version").value_or(1);
    manifest.app_version = string_field(root, "app_version").value_or("");
    manifest.exported_at = string_field(root, "exported_at").value_or("");
    manifest.categories = decode_string_array(field(root, "categories"));
    manifest.has_api_keys = boolean_field(root, "has_api_keys").value_or(false);

    ImportPreview preview;
    preview.manifest = manifest;

    preview.memory_count = 0;
    for (const auto& name : archive.names()) {
        if (name.rfind("memories/", 0) == 0) preview.memory_count++;
    }
    preview.settings_present = archive.has(native_backup::kSettingsEntry);
    preview.api_keys_present = archive.has(native_backup::kSecretsEntry);

    try {
        if (manifest.version >= 5) {
            auto index_bytes = archive.bytes(native_backup::kConversationIndexEntry);
            if (index_bytes) {
                ParseResult ipr = json::parse(*index_bytes);
                if (ipr.value && ipr.value->kind == Kind::Object) {
                    if (auto convs = field(*ipr.value, "conversations"); convs && convs->kind == Kind::Array) {
                        preview.conversation_count = static_cast<int>(convs->elements.size());
                    }
                }
            }
            // Use the graph source for counting tasks
            ArchiveEntrySource entry_source(archive);
            native_graph::NativeConversationGraphSource graph_source = native_graph::NativeConversationGraphSource::open(entry_source, manifest.version, platform_.files_dir);
            auto tasks_stream = graph_source.open_stream();
            native_graph_import::GraphStreamAdapter tasks_adapter(tasks_stream);
            native_graph_import::GraphCounts counts = graph_importer_.count(tasks_adapter);
            preview.task_count = counts.tasks;
        } else if (archive.has(native_backup::kConversationsEntry)) {
            // Use the graph source for legacy conversations.json
            ArchiveEntrySource entry_source(archive);
            native_graph::NativeConversationGraphSource graph_source = native_graph::NativeConversationGraphSource::open(entry_source, manifest.version, platform_.files_dir);
            auto conv_stream = graph_source.open_stream();
            native_graph_import::GraphStreamAdapter conv_adapter(conv_stream);
            native_graph_import::GraphCounts counts = graph_importer_.count(conv_adapter);
            preview.conversation_count = counts.conversations;
            preview.task_count = counts.tasks;
        }
    } catch (...) {
        // DebugLog.e would go here
    }

    auto prompts_bytes = archive.bytes(native_backup::kSystemPromptsEntry);
    if (prompts_bytes) {
        try {
            ParseResult ppr = json::parse(*prompts_bytes);
            if (ppr.value && ppr.value->kind == Kind::Array) {
                preview.system_prompt_count = static_cast<int>(ppr.value->elements.size());
            }
        } catch (...) {
            // DebugLog.e
        }
    }

    return preview;
}

std::optional<settings_archive::CustomFont> DataImporter::restore_custom_font(
    native_backup_archive::NativeBackupArchive& archive,
    int archive_version) {
    std::string entry_name;
    if (archive_version >= 4) {
        if (archive.has(native_backup::kCustomFontEntry)) {
            entry_name = native_backup::kCustomFontEntry;
        }
    } else {
        for (const auto& name : archive.names()) {
            if (name.rfind("custom_font/", 0) == 0) {
                std::string suffix = name.substr(std::strlen("custom_font/"));
                if (suffix.find('/') == std::string::npos) {
                    entry_name = name;
                    break;
                }
            }
        }
    }
    if (entry_name.empty()) return std::nullopt;

    std::int64_t declared_size = archive.size(entry_name);
    constexpr std::int64_t MAX_CUSTOM_FONT_BYTES = 64LL * 1024 * 1024;
    if (declared_size > MAX_CUSTOM_FONT_BYTES) {
        throw std::runtime_error("Custom font exceeds the 64 MB limit");
    }

    std::string uuid1 = platform_.new_uuid();
    std::string uuid2 = platform_.new_uuid();
    std::filesystem::path temp_path = platform_.files_dir / (".custom_font_import_" + uuid1 + ".tmp");
    std::filesystem::path target_path = platform_.files_dir / ("custom_font_import_" + uuid2);

    try {
        std::filesystem::create_directories(platform_.files_dir);
        auto copy_result = archive.copy_to(entry_name, temp_path, MAX_CUSTOM_FONT_BYTES, platform_.available_bytes);
        if (!copy_result) return std::nullopt;

        // TODO: Implement full TTF name table parsing (port of FontUtils.readFontName)
        // For now, use the file name as fallback
        std::string display_name = temp_path.stem().string();

        std::error_code ec;
        std::filesystem::rename(temp_path, target_path, ec);
        if (ec) {
            std::ifstream in(temp_path, std::ios::binary);
            if (in) {
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                native_backup_archive::NativeBackupArchive::copy_stream_to_file(
                    content, target_path, std::filesystem::file_size(temp_path),
                    -1, MAX_CUSTOM_FONT_BYTES, platform_.available_bytes, 32 * 1024, "custom font");
            }
            std::filesystem::remove(temp_path, ec);
        }

        return settings_archive::CustomFont{
            target_path.string(),
            display_name
        };
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove(temp_path, ec);
        std::filesystem::remove(target_path, ec);
        throw;
    }
}

namespace {

/** Reads the prompt items array under `name` (absent = empty). */
std::vector<data_export::PromptTemplateItem> read_items(const Value& object, std::string_view name) {
    std::vector<data_export::PromptTemplateItem> out;
    const Value* value = field(object, name);
    if (value == nullptr) return out;
    if (value->kind != Kind::Array) return out;
    for (const Value& element : value->elements) {
        if (element.kind != Kind::Object) continue;
        data_export::PromptTemplateItem item;
        item.id = string_field(element, "id").value_or("");
        if (item.id.empty()) item.id = "i" + std::to_string(out.size());
        item.type = string_field(element, "type").value_or("");
        item.value = string_field(element, "value").value_or("");
        out.push_back(std::move(item));
    }
    return out;
}

/** Parses system_prompts.json (a JSON array of prompt entries). */
std::vector<data_export::SystemPromptEntry> parse_system_prompts(std::string_view bytes) {
    std::vector<data_export::SystemPromptEntry> out;
    ParseResult pr = json::parse(std::string(bytes));
    if (!pr.value || pr.value->kind != Kind::Array) return out;
    for (const Value& element : pr.value->elements) {
        if (element.kind != Kind::Object) continue;
        data_export::SystemPromptEntry entry;
        entry.id = string_field(element, "id").value_or("");
        entry.title = string_field(element, "title").value_or("");
        entry.content = string_field(element, "content").value_or("");
        entry.system_items = read_items(element, "systemItems");
        entry.user_items = read_items(element, "userItems");
        entry.assistant_items = read_items(element, "assistantItems");
        entry.user_prepend_items = read_items(element, "userPrependItems");
        entry.user_postpend_items = read_items(element, "userPostpendItems");
        out.push_back(std::move(entry));
    }
    return out;
}

/** True when two prompts carry the same id. */
bool same_prompt_id(const data_export::SystemPromptEntry& a, const data_export::SystemPromptEntry& b) {
    return a.id == b.id;
}

/** True when two prompts are identical in title and content (the merge's skip test). */
bool identical_prompt(const data_export::SystemPromptEntry& a, const data_export::SystemPromptEntry& b) {
    return a.title == b.title && a.content == b.content;
}

/** Makes `title` unique against `taken` by appending " (imported)" / " (imported) N". */
std::string unique_title(std::string title, const std::set<std::string>& taken) {
    if (!taken.count(title)) return title;
    std::string candidate = title + " (imported)";
    int n = 2;
    while (taken.count(candidate)) {
        candidate = title + " (imported) " + std::to_string(n);
        ++n;
    }
    return candidate;
}

}  // namespace

std::vector<data_export::SystemPromptEntry> DataImporter::plan_system_prompts(
    native_backup_archive::NativeBackupArchive& archive,
    ImportStrategy strategy) {
    // What the archive carries, parsed.
    std::vector<data_export::SystemPromptEntry> incoming;
    if (auto bytes = archive.bytes(native_backup::kSystemPromptsEntry); bytes)
        incoming = parse_system_prompts(*bytes);

    // REPLACE: the archive becomes the whole set.
    if (strategy == ImportStrategy::Replace) return incoming;

    // MERGE: start from what is already there and fold the archive in.
    std::vector<data_export::SystemPromptEntry> merged;
    if (settings_.system_prompts) merged = settings_.system_prompts();

    for (data_export::SystemPromptEntry& candidate : incoming) {
        // An identical prompt (same title and content) is already present: skip it.
        bool present = false;
        for (const auto& existing : merged) {
            if (identical_prompt(existing, candidate)) { present = true; break; }
        }
        if (present) continue;

        // Ids that collide with an existing prompt get a fresh uuid; free ids are kept.
        bool id_taken = false;
        for (const auto& existing : merged) {
            if (same_prompt_id(existing, candidate)) { id_taken = true; break; }
        }
        if (id_taken && platform_.new_uuid) candidate.id = platform_.new_uuid();

        // Titles are kept unique across the merged set.
        std::set<std::string> taken;
        for (const auto& existing : merged) taken.insert(existing.title);
        candidate.title = unique_title(candidate.title, taken);

        merged.push_back(std::move(candidate));
    }
    return merged;
}

ImportResult DataImporter::import(
    native_backup_archive::NativeBackupArchive& archive,
    const std::map<data_export::ExportCategory, ImportStrategy>& decisions,
    const std::function<void(float)>& on_progress) {

    ImportResult result;

    auto manifest_bytes = archive.bytes(native_backup::kManifestEntry);
    if (!manifest_bytes) {
        result.errors.push_back(std::string(native_backup::kManifestEntry) + " is missing or invalid");
        return result;
    }

    ParseResult pr = json::parse(*manifest_bytes);
    if (!pr.value) {
        result.errors.push_back(std::string(native_backup::kManifestEntry) + " is missing or invalid");
        return result;
    }

    ImportManifest manifest;
    const Value& root = *pr.value;
    manifest.version = int_field(root, "agora_export_version").value_or(1);
    manifest.app_version = string_field(root, "app_version").value_or("");
    manifest.exported_at = string_field(root, "exported_at").value_or("");
    manifest.categories = decode_string_array(field(root, "categories"));
    manifest.has_api_keys = boolean_field(root, "has_api_keys").value_or(false);

    if (!native_backup::is_supported(manifest.version)) {
        result.errors.push_back("Unsupported backup version " + std::to_string(manifest.version) +
                               "; this app supports " + std::to_string(native_backup::kMinSupportedVersion) + "–" +
                               std::to_string(native_backup::kCurrentVersion));
        return result;
    }

    std::map<data_export::ExportCategory, ImportStrategy> active_categories = decisions;
    for (auto it = active_categories.begin(); it != active_categories.end();) {
        if (it->second == ImportStrategy::Skip) it = active_categories.erase(it);
        else ++it;
    }
    int total_steps = static_cast<int>(active_categories.size());
    int completed = 0;
    std::string pending_transfer_id;
    auto step = [&]() {
        completed++;
        if (on_progress) on_progress(static_cast<float>(completed) / std::max(1, total_steps));
    };

    [[maybe_unused]] bool allow_legacy_secrets = manifest.version < native_backup::kSeparateSecretsSinceVersion &&
                               decisions.count(data_export::ExportCategory::ApiKeys) &&
                               decisions.at(data_export::ExportCategory::ApiKeys) != ImportStrategy::Skip;

    bool conv_selected = decisions.count(data_export::ExportCategory::Conversations) &&
                         decisions.at(data_export::ExportCategory::Conversations) != ImportStrategy::Skip;
    bool settings_selected = decisions.count(data_export::ExportCategory::Settings) &&
                             decisions.at(data_export::ExportCategory::Settings) != ImportStrategy::Skip;

    try {
        archive.preflight_import_resources(conv_selected, settings_selected, manifest.version,
                                           platform_.files_dir, 64LL * 1024 * 1024,
                                           platform_.available_bytes);
    } catch (const std::exception& e) {
        result.errors.push_back(e.what());
        return result;
    }

    // 2/4. Stage + import the conversation graph first: a staging failure aborts
    // the whole import before any progress is reported (the reference stages
    // before it starts ticking progress).
    if (active_categories.count(data_export::ExportCategory::Conversations)) {
        const ImportStrategy strategy = active_categories.at(data_export::ExportCategory::Conversations);
        try {
            ArchiveEntrySource entry_source(archive);
            native_graph::NativeConversationGraphSource source =
                native_graph::NativeConversationGraphSource::open(entry_source, manifest.version,
                                                                  platform_.files_dir);
            int conversations = 0, tasks = 0, loops = 0;
            const native_graph_import::ImportStrategy graph_strategy =
                strategy == ImportStrategy::Replace ? native_graph_import::ImportStrategy::Replace
                : strategy == ImportStrategy::Merge  ? native_graph_import::ImportStrategy::Merge
                                                     : native_graph_import::ImportStrategy::Skip;
            {
                native_graph::NativeConversationGraphSource::Stream stream = source.open_stream();
                native_graph_import::GraphStreamAdapter adapter(stream);
                native_graph_import::ConversationGraphHeaders headers =
                    graph_importer_.read_headers(adapter, graph_strategy,
                                                 native_graph_import::RestoredMedia{});
                conversations = static_cast<int>(headers.conversations.size());
                tasks = static_cast<int>(headers.tasks.size());
                loops = static_cast<int>(headers.loops.size());
                semantic_index::SemanticModelSnapshot snapshot;
                pending_transfer_id = graph_importer_.import_graph(source, graph_strategy, headers,
                                                                  native_graph_import::RestoredMedia{},
                                                                  manifest.version, snapshot);
            }
            source.close();
            result.conversations_imported = conversations;
            result.tasks_imported = tasks;
            result.loops_imported = loops;
        } catch (const std::exception& e) {
            result.errors.push_back(std::string("Conversations: ") + e.what());
            return result;
        }
    }

    // Each active category is processed in enum order, reporting progress after
    // it; the categories whose logic is still staged below are counted (their
    // step fires) so the progress matches the reference even before they write.
    for (const auto& [category, strategy] : active_categories) {
        if (category == data_export::ExportCategory::SystemPrompts) {
            // 1. Plan system prompts (MERGE/REPLACE), 3. Save prompts.
            try {
                const int incoming_count = [&] {
                    auto bytes = archive.bytes(native_backup::kSystemPromptsEntry);
                    if (!bytes) return 0;
                    return static_cast<int>(parse_system_prompts(*bytes).size());
                }();
                std::vector<data_export::SystemPromptEntry> planned = plan_system_prompts(archive, strategy);
                if (writes_.save_system_prompts) writes_.save_system_prompts(planned);
                result.system_prompts_imported = incoming_count;
            } catch (const std::exception& e) {
                result.errors.push_back(e.what());
            }
        } else if (category == data_export::ExportCategory::Memories) {
            // 5. Import memories and skills from the archive's memories/ tree.
            try {
                int imported = 0;
                for (const std::string& name : archive.names()) {
                    if (name.rfind("memories/", 0) != 0) continue;
                    const std::string relative = name.substr(std::strlen("memories/"));
                    const auto content = archive.bytes(name);
                    if (!content) continue;
                    // Overwrite an existing file the way the reference's import does:
                    // a name already on disk is replaced, not rejected.
                    auto write_or_replace = [](auto& manager, const std::string& file_name,
                                               const std::string& text) {
                        try {
                            manager.create_file(file_name, text);
                        } catch (const std::exception&) {
                            manager.delete_file(file_name);
                            manager.create_file(file_name, text);
                        }
                    };
                    if (relative == "active_memory.md") {
                        memory_.update_active_memory(*content);
                    } else if (relative == "memory_db/memory_meta.json") {
                        memory_.save_meta_json(*content);
                    } else if (relative.rfind("memory_db/", 0) == 0) {
                        write_or_replace(memory_, relative.substr(std::strlen("memory_db/")), *content);
                    } else if (relative == "skill_db/skill_meta.json") {
                        skill_.save_meta_json(*content);
                    } else if (relative.rfind("skill_db/", 0) == 0) {
                        write_or_replace(skill_, relative.substr(std::strlen("skill_db/")), *content);
                    }
                    if (relative.size() >= 3 && relative.substr(relative.size() - 3) == ".md")
                        ++imported;
                }
                result.memories_imported = imported;
            } catch (const std::exception& e) {
                result.errors.push_back(std::string("Memories: ") + e.what());
            }
        } else if (category == data_export::ExportCategory::Settings) {
            // The conversation-settings transfer the graph import staged lands
            // after the prompts and before the settings restore -- the reference
            // saves the map first, then restores the archive.
            if (!pending_transfer_id.empty()) {
                transfers_.complete_import(pending_transfer_id);
                pending_transfer_id.clear();
            }
            // 6. Import settings: restore the portable archive (validating it),
            // restore a custom font, reconcile the embedding models.
            try {
                const auto settings_bytes = archive.bytes(native_backup::kSettingsEntry);
                if (!settings_bytes) throw std::runtime_error("settings.json is missing");
                settings_archive::RestoreOptions options;
                options.replace = strategy == ImportStrategy::Replace;
                if (archive.has(native_backup::kCustomFontEntry))
                    options.restored_custom_font =
                        restore_custom_font(archive, manifest.version);
                const json::ParseResult parsed_settings = json::parse(*settings_bytes);
                if (!parsed_settings.value || parsed_settings.value->kind != Kind::Object)
                    throw std::runtime_error("settings.json is invalid");
                if (writes_.archive_sink != nullptr) {
                    settings_archive::restore_from_json_object(
                        *parsed_settings.value, settings_.archive_snapshot
                                                    ? settings_.archive_snapshot()
                                                    : settings_archive::SettingsSnapshot{},
                        *writes_.archive_sink, options);
                }
                result.settings_imported = true;
            } catch (const settings_archive::IllegalArgumentException& e) {
                result.errors.push_back(std::string("Settings: ") + e.what());
            } catch (const std::exception& e) {
                result.errors.push_back(std::string("Settings: ") + e.what());
            }
        }
        // 7. API keys: staged (Conversations/prompts/memories/settings run above).
        step();
    }

    // Apply the conversation-settings transfer if no settings category consumed it
    // (scenario 06: prompts + conversations only -- the map still lands).
    if (!pending_transfer_id.empty()) transfers_.complete_import(pending_transfer_id);

    // 8. Final progress and result

    if (on_progress) on_progress(1.0f);
    return result;
}

}  // namespace agora::core::data_import
