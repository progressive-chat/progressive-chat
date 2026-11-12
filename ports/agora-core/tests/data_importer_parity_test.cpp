// Golden parity test for the C++ DataImporter port's preview path.
//
// Replays every `preview` step under core/tools/golden/data-importer/scenarios:
// the step's ordered archive entries are assembled into a ZIP, opened as a
// NativeBackupArchive, and DataImporter::preview is asked what the archive
// holds. The ImportPreview is built into a JSON Value and compared with the
// frozen Kotlin golden, structurally (key order does not matter), so the port
// must read and count exactly what the reference did.
//
// Only the `preview` steps are driven here; the `import` and `font` steps need
// the full import harness (progress callbacks, decisions, the platform surface)
// and are covered when that lands.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <regex>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <sqlite3.h>

#include "agora/core/chat_dao.hpp"
#include "agora/core/data_exporter.hpp"
#include "agora/core/data_importer.hpp"
#include "agora/core/openai_request.hpp"
#include "agora/core/portable_settings_archive.hpp"
#include "agora/core/font_utils.hpp"
#include "agora/core/json.hpp"
#include "agora/core/maintenance_debt.hpp"
#include "agora/core/memory_manager.hpp"
#include "agora/core/native_backup_archive.hpp"
#include "agora/core/semantic_index_ledger.hpp"
#include "agora/core/skill_manager.hpp"
#include "agora/core/sql_boundary.hpp"
#include "agora/core/zip_writer.hpp"
#include "check.hpp"

#ifndef AGORA_FIXTURE_DATA_IMPORTER_DIR
#error "AGORA_FIXTURE_DATA_IMPORTER_DIR must point at core/tools/golden/data-importer"
#endif

namespace {

namespace json = agora::core::json;
namespace data_import = agora::core::data_import;
namespace data_export = agora::core::data_export;
namespace backup = agora::core::native_backup_archive;
namespace secrets = agora::core::native_backup;
namespace archive = agora::core::settings_archive;
using agora::core::json::Kind;
using agora::core::json::Member;
using agora::core::json::Value;
using agora::core::SqlExecutor;
using agora::core::SqlValue;
using agora::core::SqlRow;
using agora::core::SqlArgs;
using agora::core::SqlError;
using agora::core::SqlConstraintError;

[[noreturn]] void scenario_fail(const std::string& what) {
    agora::core::test::fail(__FILE__, __LINE__, what);
}

// Mismatches are collected rather than aborting the run, so one scenario's failure
// does not hide the rest: every scenario is checked, then all differences report at once.
std::vector<std::string> g_mismatches;

void scenario_diff(const std::string& what) {
    g_mismatches.push_back(what);
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

const Value* field(const Value& object, std::string_view name) {
    if (object.kind != Kind::Object) return nullptr;
    for (const Member& member : object.members)
        if (member.key == name) return &member.value;
    return nullptr;
}

std::string required_text(const Value& object, std::string_view name) {
    const Value* found = field(object, name);
    if (found == nullptr || found->kind != Kind::String)
        scenario_fail("scenario needs a string " + std::string(name));
    return found->text;
}

std::string text_or(const Value& object, std::string_view name, std::string fallback = "") {
    const Value* found = field(object, name);
    if (found == nullptr || found->kind != Kind::String) return fallback;
    return found->text;
}

std::vector<data_export::PromptTemplateItem> read_items(const Value& object, std::string_view name) {
    std::vector<data_export::PromptTemplateItem> out;
    const Value* value = field(object, name);
    if (value == nullptr || value->kind != Kind::Array) return out;
    std::size_t n = 0;
    for (const Value& element : value->elements) {
        if (element.kind != Kind::Object) continue;
        data_export::PromptTemplateItem item;
        item.id = text_or(element, "id", "i" + std::to_string(n));
        item.type = text_or(element, "type");
        item.value = text_or(element, "value");
        out.push_back(std::move(item));
        ++n;
    }
    return out;
}

/** The scenario's seed system prompts (the initial state for a MERGE). */
std::vector<data_export::SystemPromptEntry> read_prompt_seed(const Value& root) {
    std::vector<data_export::SystemPromptEntry> out;
    const Value* value = field(root, "systemPrompts");
    if (value == nullptr || value->kind != Kind::Array) return out;
    for (const Value& element : value->elements) {
        if (element.kind != Kind::Object) continue;
        data_export::SystemPromptEntry entry;
        entry.id = text_or(element, "id");
        entry.title = text_or(element, "title");
        entry.content = text_or(element, "content");
        entry.system_items = read_items(element, "systemItems");
        entry.user_items = read_items(element, "userItems");
        entry.assistant_items = read_items(element, "assistantItems");
        entry.user_prepend_items = read_items(element, "userPrependItems");
        entry.user_postpend_items = read_items(element, "userPostpendItems");
        out.push_back(std::move(entry));
    }
    return out;
}

// ---- masking (identical to the stand driver's two passes) -----------------------------------------

std::string mask_trace(std::string text) {
    // UUIDs: first-appearance ordinal replacement. Fixture content never carries
    // UUID-shaped values; the generated ones are the uuids the import made.
    static const std::regex uuid_pattern(
        "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}");
    std::map<std::string, std::string> ordinals;
    std::string masked;
    auto begin = std::sregex_iterator(text.begin(), text.end(), uuid_pattern);
    std::size_t copied = 0;
    for (auto it = begin; it != std::sregex_iterator(); ++it) {
        const std::smatch& match = *it;
        const std::string uuid = match.str();
        const auto [entry, inserted] =
            ordinals.emplace(uuid, "<uuid:" + std::to_string(ordinals.size()) + ">");
        (void)inserted;
        masked.append(text, copied,
                      static_cast<std::size_t>(match.position(0)) - copied);
        copied = static_cast<std::size_t>(match.position(0) + match.length(0));
        masked += entry->second;
    }
    masked.append(text, copied, std::string::npos);

    // Epochs: any 13-digit 1[7-9] value bounded by non-digits -> "<clock>".
    std::string out;
    out.reserve(masked.size());
    for (std::size_t i = 0; i < masked.size();) {
        const auto digit = [](char c) { return c >= '0' && c <= '9'; };
        if (digit(masked[i]) && i + 13 <= masked.size()) {
            bool candidate = (masked[i + 1] == '7' || masked[i + 1] == '8' ||
                              masked[i + 1] == '9');
            for (std::size_t k = 0; k < 13 && candidate; ++k) candidate = digit(masked[i + k]);
            if (candidate) {
                const bool before_ok = i == 0 || (!digit(masked[i - 1]) && masked[i - 1] != '.');
                const bool after_ok = i + 13 == masked.size() || !digit(masked[i + 13]);
                if (before_ok && after_ok) {
                    out += "\"<clock>\"";
                    i += 13;
                    continue;
                }
            }
        }
        out.push_back(masked[i]);
        ++i;
    }
    return out;
}

/** Renders a Value, masks uuids/clocks, and re-parses it for comparison. */
Value masked(const Value& value) {
    const json::ParseResult parsed = json::parse(mask_trace(json::to_kotlin_string(value)));
    return parsed.value ? *parsed.value : value;
}

// The Value builders the sibling tests use: a string is a String, and a bare
// number or boolean is a Literal, which is how json::parse classifies them, so
// the two sides compare as the same kind.
Value json_string(std::string_view text) {
    Value v;
    v.kind = Kind::String;
    v.text = std::string(text);
    return v;
}

Value json_long(std::int64_t number) {
    Value v;
    v.kind = Kind::Literal;
    v.text = std::to_string(number);
    return v;
}

Value json_bool(bool value) {
    Value v;
    v.kind = Kind::Literal;
    v.text = value ? "true" : "false";
    return v;
}

// A bare token (null, true, a number): json::parse reads it as a Literal, so
// the port must build it as one to compare equal.
Value json_literal(std::string_view token) {
    Value v;
    v.kind = Kind::Literal;
    v.text = std::string(token);
    return v;
}

Value json_array(std::vector<Value> elements) {
    Value v;
    v.kind = Kind::Array;
    v.elements = std::move(elements);
    return v;
}

Value json_object(std::vector<std::pair<std::string, Value>> members) {
    Value v;
    v.kind = Kind::Object;
    for (auto& [key, value] : members) v.members.push_back({std::move(key), std::move(value)});
    return v;
}

// Structural equality: every key found, every value equal; the order the two
// sides list their members in does not matter. (Same rule as the sibling
// tests, so a port that reorders fields still matches.)
bool json_equal(const Value& left, const Value& right) {
    if (left.kind != right.kind) return false;
    if (left.kind == Kind::String || left.kind == Kind::Literal) return left.text == right.text;
    if (left.kind == Kind::Array) {
        if (left.elements.size() != right.elements.size()) return false;
        for (std::size_t i = 0; i < left.elements.size(); ++i)
            if (!json_equal(left.elements[i], right.elements[i])) return false;
        return true;
    }
    if (left.members.size() != right.members.size()) return false;
    for (const Member& member : left.members) {
        const Value* match = field(right, member.key);
        if (match == nullptr || !json_equal(member.value, *match)) return false;
    }
    return true;
}


// base64 (the fixtures mirror java.util.Base64; copied from the exporter test
// so both decode the same bytes).
std::vector<std::uint8_t> base64_decode(std::string_view text) {
    std::vector<std::uint8_t> out;
    int accumulator = 0;
    int bits = 0;
    for (const char ch : text) {
        if (ch == '=' || ch == '\n' || ch == '\r') break;
        int value = -1;
        if (ch >= 'A' && ch <= 'Z') value = ch - 'A';
        else if (ch >= 'a' && ch <= 'z') value = ch - 'a' + 26;
        else if (ch >= '0' && ch <= '9') value = ch - '0' + 52;
        else if (ch == '+') value = 62;
        else if (ch == '/') value = 63;
        if (value < 0) continue;
        accumulator = (accumulator << 6) | value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xFF));
        }
    }
    return out;
}

std::string excerpt(const Value& value, std::size_t limit = 400) {
    std::string text = json::to_kotlin_string(value);
    if (text.size() > limit) return text.substr(0, limit) + "...";
    return text;
}

// An in-memory SQLite over the stand's schema: the DAO dependencies the
// importer holds read it, and a preview never writes to it. (Same double the
// chat-dao parity test uses.)
class SqliteExecutor : public agora::core::SqlExecutor {
public:
    explicit SqliteExecutor(const std::filesystem::path& schema_file) {
        if (sqlite3_open(":memory:", &db_) != SQLITE_OK) {
            scenario_fail(std::string("cannot open sqlite: ") + sqlite3_errmsg(db_));
        }
        exec_raw("PRAGMA foreign_keys = ON");
        const std::string schema = read_file(schema_file);
        char* error = nullptr;
        if (sqlite3_exec(db_, schema.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
            const std::string message = error != nullptr ? error : "schema failed";
            sqlite3_free(error);
            scenario_fail("schema: " + message);
        }
    }

    ~SqliteExecutor() override {
        if (db_ != nullptr) sqlite3_close(db_);
    }

    std::int64_t execute(const std::string& sql, const SqlArgs& args) override {
        sqlite3_stmt* statement = prepare(sql);
        try {
            bind(statement, args);
        } catch (...) {
            sqlite3_finalize(statement);
            throw;
        }
        const int rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE) {
            const std::int64_t changed = sqlite3_changes(db_);
            sqlite3_finalize(statement);
            return changed;
        }
        const std::string message = sqlite3_errmsg(db_);
        sqlite3_finalize(statement);
        if ((rc & 0xFF) == SQLITE_CONSTRAINT) throw SqlConstraintError(message);
        throw SqlError("sqlite: " + message + " in " + sql);
    }

    std::vector<SqlRow> query(const std::string& sql, const SqlArgs& args) override {
        sqlite3_stmt* statement = prepare(sql);
        try {
            bind(statement, args);
        } catch (...) {
            sqlite3_finalize(statement);
            throw;
        }
        std::vector<SqlRow> rows;
        for (;;) {
            const int rc = sqlite3_step(statement);
            if (rc == SQLITE_ROW) {
                rows.push_back(read_row(statement));
                continue;
            }
            if (rc == SQLITE_DONE) break;
            const std::string message = sqlite3_errmsg(db_);
            sqlite3_finalize(statement);
            throw SqlError("sqlite: " + message + " in " + sql);
        }
        sqlite3_finalize(statement);
        return rows;
    }

    std::int64_t last_insert_rowid() override { return sqlite3_last_insert_rowid(db_); }

    // Reference-counted: nested DAO transactions join the outer import transaction, and only the
    // outermost level issues COMMIT/ROLLBACK — the JVM SQLiteSession behavior Room relies on.
    void begin() override {
        if (depth_++ == 0) {
            aborted_ = false;
            exec_raw("BEGIN");
        }
    }

    void commit() override {
        if (depth_ == 0) throw agora::core::SqlError("commit without a transaction");
        if (--depth_ == 0) {
            if (deferred_rollback_) {
                deferred_rollback_ = false;
                exec_raw("ROLLBACK");
            } else {
                exec_raw("COMMIT");
            }
        }
    }

    void rollback() override {
        if (depth_ == 0) throw agora::core::SqlError("rollback without a transaction");
        deferred_rollback_ = true;
        if (--depth_ == 0) {
            deferred_rollback_ = false;
            exec_raw("ROLLBACK");
        }
    }

private:
    void exec_raw(const std::string& sql) {
        char* error = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
            const std::string message = error != nullptr ? error : "exec failed";
            sqlite3_free(error);
            scenario_fail("sqlite: " + message + " in " + sql);
        }
    }

    sqlite3_stmt* prepare(const std::string& sql) {
        sqlite3_stmt* statement = nullptr;
        const int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &statement, nullptr);
        if (rc != SQLITE_OK) {
            throw SqlError(std::string("sqlite prepare: ") + sqlite3_errmsg(db_) + " in " + sql);
        }
        return statement;
    }

    void bind(sqlite3_stmt* statement, const SqlArgs& args) {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const int index = static_cast<int>(i + 1);
            const SqlValue& value = args[i];
            int rc = SQLITE_OK;
            switch (value.kind) {
                case SqlValue::Kind::Null:
                    rc = sqlite3_bind_null(statement, index);
                    break;
                case SqlValue::Kind::Text:
                    rc = sqlite3_bind_text(statement, index, value.text.data(),
                                           static_cast<int>(value.text.size()), SQLITE_TRANSIENT);
                    break;
                case SqlValue::Kind::Integer:
                    rc = sqlite3_bind_int64(statement, index, value.integer);
                    break;
                case SqlValue::Kind::Real:
                    rc = sqlite3_bind_double(statement, index, value.real);
                    break;
                case SqlValue::Kind::Blob: {
                    const void* data = value.blob.empty()
                                           ? static_cast<const void*>("")
                                           : static_cast<const void*>(value.blob.data());
                    rc = sqlite3_bind_blob(statement, index, data,
                                           static_cast<int>(value.blob.size()), SQLITE_TRANSIENT);
                    break;
                }
            }
            if (rc != SQLITE_OK) throw SqlError(std::string("sqlite bind: ") + sqlite3_errmsg(db_));
        }
    }

    static SqlRow read_row(sqlite3_stmt* statement) {
        SqlRow row;
        const int columns = sqlite3_column_count(statement);
        for (int i = 0; i < columns; ++i) {
            SqlValue value;
            switch (sqlite3_column_type(statement, i)) {
                case SQLITE_NULL:
                    value.kind = SqlValue::Kind::Null;
                    break;
                case SQLITE_INTEGER:
                    value.kind = SqlValue::Kind::Integer;
                    value.integer = sqlite3_column_int64(statement, i);
                    break;
                case SQLITE_FLOAT:
                    value.kind = SqlValue::Kind::Real;
                    value.real = sqlite3_column_double(statement, i);
                    break;
                case SQLITE_TEXT: {
                    value.kind = SqlValue::Kind::Text;
                    const auto* text = sqlite3_column_text(statement, i);
                    const int length = sqlite3_column_bytes(statement, i);
                    value.text.assign(reinterpret_cast<const char*>(text),
                                      static_cast<std::size_t>(length));
                    break;
                }
                case SQLITE_BLOB: {
                    value.kind = SqlValue::Kind::Blob;
                    const auto* data = static_cast<const std::uint8_t*>(
                        sqlite3_column_blob(statement, i));
                    const int length = sqlite3_column_bytes(statement, i);
                    value.blob.assign(data, data + length);
                    break;
                }
            }
            row.push_back(std::move(value));
        }
        return row;
    }

    sqlite3* db_ = nullptr;
    int depth_ = 0;
    bool deferred_rollback_ = false;
    bool aborted_ = false;
};

// The step's ordered archive entries, assembled into the bytes of a real ZIP
// the archive reads itself from.
std::string archive_bytes(const Value& step) {
    const Value* entries = field(step, "archive");
    if (entries == nullptr || entries->kind != Kind::Array)
        scenario_fail("preview step needs an archive array");
    agora::core::zip::Writer writer;
    for (const Value& entry : entries->elements) {
        const std::string name = required_text(entry, "name");
        if (const Value* text = field(entry, "text");
            text != nullptr && text->kind == Kind::String) {
            writer.put_entry(name, text->text);
            continue;
        }
        // Media and font entries carry raw bytes, base64-encoded.
        if (const Value* encoded = field(entry, "base64");
            encoded != nullptr && encoded->kind == Kind::String) {
            const std::vector<std::uint8_t> bytes = base64_decode(encoded->text);
            writer.put_entry(name, std::string_view(reinterpret_cast<const char*>(bytes.data()),
                                                    bytes.size()));
            continue;
        }
        scenario_fail("archive entry needs a text");
    }
    return writer.finish();
}

// What preview says of an archive, as the golden says it: the manifest read
// whole, the counts, and the three booleans it derives. Written out field by
// field in the golden's own key names.
Value preview_value(const data_import::ImportPreview& preview) {
    std::vector<Value> categories;
    for (const std::string& one : preview.manifest.categories) categories.push_back(json_string(one));
    return json_object({
        {"manifest",
         json_object({
             {"version", json_long(preview.manifest.version)},
             {"appVersion", json_string(preview.manifest.app_version)},
             {"exportedAt", json_string(preview.manifest.exported_at)},
             {"categories", json_array(std::move(categories))},
             {"hasApiKeys", json_bool(preview.manifest.has_api_keys)},
         })},
        {"conversationCount", json_long(preview.conversation_count)},
        {"taskCount", json_long(preview.task_count)},
        {"memoryCount", json_long(preview.memory_count)},
        {"systemPromptCount", json_long(preview.system_prompt_count)},
        {"settingsPresent", json_bool(preview.settings_present)},
        {"apiKeysPresent", json_bool(preview.api_keys_present)},
        {"hasConversationGraph", json_bool(preview.has_conversation_graph())},
        {"hasImportableData", json_bool(preview.has_importable_data())},
        {"isSupportedVersion", json_bool(preview.is_supported_version())},
    });
}

// One scenario: every preview step in it, run through the port.
// ---- recording SettingsSink + snapshot serializers (copied from the
// portable-settings-archive parity test, which produces the same
// {name,value} call log and the snapshot shapes the importer's golden pins)

void append_quoted(std::string& out, std::string_view text) {
    out.push_back('"');
    json::escape_to(out, text);
    out.push_back('"');
}

std::string quoted(std::string_view text) {
    std::string out;
    append_quoted(out, text);
    return out;
}


class RecordingStore : public archive::SettingsSink {
public:
    std::string calls;
    bool first = true;

    void append(const char* name, const std::string& value) {
        if (!first) calls.push_back(',');
        first = false;
        calls += "{\"name\":";
        append_quoted(calls, name);
        calls += ",\"value\":";
        calls += value;
        calls.push_back('}');
    }

    static std::string boolean_value(bool value) {
        return std::string("{\"bool\":") + (value ? "true" : "false") + "}";
    }
    static std::string int_value(int value) {
        return "{\"int\":" + std::to_string(value) + "}";
    }
    static std::string float_value(const std::optional<float>& value) {
        if (!value.has_value()) return "{\"float\":null}";
        return "{\"float\":\"" + agora::core::openai::format_kotlin_float(*value) + "\"}";
    }
    static std::string string_value(const std::optional<std::string>& value) {
        if (!value.has_value()) return "{\"string\":null}";
        std::string out = "{\"string\":";
        append_quoted(out, *value);
        out.push_back('}');
        return out;
    }
    static std::string strings_value(const std::vector<std::string>& values) {
        std::string out = "{\"strings\":[";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) out.push_back(',');
            append_quoted(out, values[i]);
        }
        out += "]}";
        return out;
    }
    static std::string map_value(const archive::StringMap& values) {
        std::string out = "{\"map\":{";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) out.push_back(',');
            append_quoted(out, values[i].first);
            out.push_back(':');
            append_quoted(out, values[i].second);
        }
        out += "}}";
        return out;
    }
    static std::string embedding_value(const std::vector<secrets::EmbeddingModelConfig>& models) {
        std::string out = "{\"embeddingModels\":[";
        for (std::size_t i = 0; i < models.size(); ++i) {
            if (i != 0) out.push_back(',');
            const auto& model = models[i];
            out += "{\"id\":";
            append_quoted(out, model.id);
            out += ",\"name\":";
            append_quoted(out, model.name);
            out += ",\"type\":";
            append_quoted(out, model.type == secrets::EmbeddingModelType::Remote ? "REMOTE"
                                                                                : "LOCAL");
            out += ",\"remoteModelName\":";
            append_quoted(out, model.remote_model_name);
            out += ",\"remoteBaseUrl\":";
            append_quoted(out, model.remote_base_url);
            out += ",\"remoteApiKey\":";
            append_quoted(out, model.remote_api_key);
            out += ",\"localFilePath\":";
            append_quoted(out, model.local_file_path);
            out += ",\"batchSize\":" + std::to_string(model.batch_size) + "}";
        }
        out += "]}";
        return out;
    }
    static std::string shell_value(const std::vector<secrets::ShellDeviceConfig>& devices) {
        std::string out = "{\"shellDevices\":[";
        for (std::size_t i = 0; i < devices.size(); ++i) {
            if (i != 0) out.push_back(',');
            const auto& device = devices[i];
            out += "{\"id\":";
            append_quoted(out, device.id);
            out += ",\"name\":";
            append_quoted(out, device.name);
            out += ",\"description\":";
            append_quoted(out, device.description);
            out += ",\"type\":";
            append_quoted(out, device.type);
            out += ",\"serverUrl\":";
            append_quoted(out, device.server_url);
            out += ",\"apiKey\":";
            append_quoted(out, device.api_key);
            out += ",\"conchPublicKey\":";
            append_quoted(out, device.conch_public_key);
            out += ",\"sshHost\":";
            append_quoted(out, device.ssh_host);
            out += ",\"sshPort\":" + std::to_string(device.ssh_port);
            out += ",\"sshUser\":";
            append_quoted(out, device.ssh_user);
            out += ",\"sshPassword\":";
            append_quoted(out, device.ssh_password);
            out += ",\"sshHostKey\":";
            append_quoted(out, device.ssh_host_key);
            out += "}";
        }
        out += "]}";
        return out;
    }
    static std::string mcp_value(const std::vector<secrets::McpServerConfig>& servers) {
        std::string out = "{\"mcpServers\":[";
        for (std::size_t i = 0; i < servers.size(); ++i) {
            if (i != 0) out.push_back(',');
            const auto& server = servers[i];
            out += "{\"id\":";
            append_quoted(out, server.id);
            out += ",\"name\":";
            append_quoted(out, server.name);
            out += ",\"enabled\":" + std::string(server.enabled ? "true" : "false");
            out += ",\"url\":";
            append_quoted(out, server.url);
            out += ",\"transport\":";
            append_quoted(out, server.transport == "sse" ? "SSE" : "STREAMABLE_HTTP");
            out += ",\"headers\":{";
            for (std::size_t h = 0; h < server.headers.size(); ++h) {
                if (h != 0) out.push_back(',');
                append_quoted(out, server.headers[h].first);
                out.push_back(':');
                append_quoted(out, server.headers[h].second);
            }
            out += "},\"disabledTools\":[";
            for (std::size_t t = 0; t < server.disabled_tools.size(); ++t) {
                if (t != 0) out.push_back(',');
                append_quoted(out, server.disabled_tools[t]);
            }
            out += "]}";
        }
        out += "]}";
        return out;
    }
    static std::string provider_value(
        const std::vector<archive::CustomProviderConfig>& providers) {
        std::string out = "{\"customProviders\":[";
        for (std::size_t i = 0; i < providers.size(); ++i) {
            if (i != 0) out.push_back(',');
            const auto& provider = providers[i];
            out += "{\"name\":";
            append_quoted(out, provider.name);
            out += ",\"protocol\":";
            append_quoted(out, provider.protocol);
            out += ",\"id\":";
            append_quoted(out, provider.id);
            out += ",\"legacyNames\":[";
            for (std::size_t n = 0; n < provider.legacy_names.size(); ++n) {
                if (n != 0) out.push_back(',');
                append_quoted(out, provider.legacy_names[n]);
            }
            out += "],\"responsesApiEnabled\":";
            out += provider.responses_api_enabled ? "true" : "false";
            out += ",\"anthropicCacheEnabled\":";
            out += provider.anthropic_cache_enabled ? "true" : "false";
            out += ",\"anthropicCacheTtl\":";
            append_quoted(out, provider.anthropic_cache_ttl);
            out += "}";
        }
        out += "]}";
        return out;
    }

    /** saveSystemPrompts: {systemPrompts:[{id,title,content,items...}]} as the reference logs it. */
    static std::string prompts_value(const std::vector<data_export::SystemPromptEntry>& prompts) {
        auto items_json = [](const std::vector<data_export::PromptTemplateItem>& items) {
            std::string out = "[";
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i != 0) out.push_back(',');
                out += "{\"id\":";
                append_quoted(out, items[i].id);
                out += ",\"type\":";
                append_quoted(out, items[i].type);
                out += ",\"value\":";
                append_quoted(out, items[i].value);
                out.push_back('}');
            }
            out.push_back(']');
            return out;
        };
        std::string out = "{\"systemPrompts\":[";
        for (std::size_t i = 0; i < prompts.size(); ++i) {
            if (i != 0) out.push_back(',');
            const auto& prompt = prompts[i];
            out += "{\"id\":";
            append_quoted(out, prompt.id);
            out += ",\"title\":";
            append_quoted(out, prompt.title);
            out += ",\"content\":";
            append_quoted(out, prompt.content);
            out += ",\"systemItems\":" + items_json(prompt.system_items);
            out += ",\"userItems\":" + items_json(prompt.user_items);
            out += ",\"assistantItems\":" + items_json(prompt.assistant_items);
            out += ",\"userPrependItems\":" + items_json(prompt.user_prepend_items);
            out += ",\"userPostpendItems\":" + items_json(prompt.user_postpend_items);
            out.push_back('}');
        }
        out += "]}";
        return out;
    }

    void save_selected_model(const std::string& model) override {
        append("saveSelectedModel", string_value(model));
    }
    void save_custom_models(const std::vector<std::string>& models) override {
        append("saveCustomModels", strings_value(models));
    }
    void save_enabled_models(const std::vector<std::string>& models) override {
        append("saveEnabledModels", strings_value(models));
    }
    void save_model_aliases(const archive::StringMap& aliases) override {
        append("saveModelAliases", map_value(aliases));
    }
    void save_model_provider_names(const archive::BoolMap& values, bool replace) override {
        std::string map = "{\"map\":{";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) map.push_back(',');
            append_quoted(map, values[i].first);
            map.push_back(':');
            map += values[i].second ? "true" : "false";
        }
        map += "},\"bool\":";
        map += replace ? "true" : "false";
        map.push_back('}');
        append("saveModelProviderNames", map);
    }
    void save_max_context_window(int window) override {
        append("saveMaxContextWindow", int_value(window));
    }
    void save_visualize_context_rollout(bool enabled) override {
        append("saveVisualizeContextRollout", boolean_value(enabled));
    }
    void save_context_compact_enabled(bool enabled) override {
        append("saveContextCompactEnabled", boolean_value(enabled));
    }
    void save_context_compact_model(const std::optional<std::string>& model) override {
        append("saveContextCompactModel", string_value(model));
    }
    void save_context_compact_prompt(const std::string& prompt) override {
        append("saveContextCompactPrompt", string_value(std::optional<std::string>(prompt)));
    }
    void save_context_compact_retain_count(int count) override {
        append("saveContextCompactRetainCount", int_value(count));
    }
    void save_context_compact_preserve_system_prompt(bool enabled) override {
        append("saveContextCompactPreserveSystemPrompt", boolean_value(enabled));
    }
    void save_context_compact_threshold_percent(int percent) override {
        append("saveContextCompactThresholdPercent", int_value(percent));
    }
    void save_code_execution_enabled(bool enabled) override {
        append("saveCodeExecutionEnabled", boolean_value(enabled));
    }
    void save_google_search_enabled(bool enabled) override {
        append("saveGoogleSearchEnabled", boolean_value(enabled));
    }
    void save_thinking_enabled(bool enabled) override {
        append("saveThinkingEnabled", boolean_value(enabled));
    }
    void save_thinking_level(const std::string& level) override {
        append("saveThinkingLevel", string_value(std::optional<std::string>(level)));
    }
    void save_thinking_budget_enabled(bool enabled) override {
        append("saveThinkingBudgetEnabled", boolean_value(enabled));
    }
    void save_thinking_budget_tokens(int tokens) override {
        append("saveThinkingBudgetTokens", int_value(tokens));
    }
    void save_open_ai_service_tier_enabled(bool enabled) override {
        append("saveOpenAiServiceTierEnabled", boolean_value(enabled));
    }
    void save_open_ai_service_tier(const std::string& tier) override {
        append("saveOpenAiServiceTier", string_value(std::optional<std::string>(tier)));
    }
    void save_open_ai_responses_api_enabled(bool enabled) override {
        append("saveOpenAiResponsesApiEnabled", boolean_value(enabled));
    }
    void save_anthropic_cache_enabled(bool enabled) override {
        append("saveAnthropicCacheEnabled", boolean_value(enabled));
    }
    void save_anthropic_cache_ttl(const std::string& ttl) override {
        append("saveAnthropicCacheTtl", string_value(std::optional<std::string>(ttl)));
    }
    void save_provider_base_urls(const archive::StringMap& urls) override {
        append("saveProviderBaseUrls", map_value(urls));
    }
    void save_title_generation_enabled(bool enabled) override {
        append("saveTitleGenerationEnabled", boolean_value(enabled));
    }
    void save_title_generation_model(const std::optional<std::string>& model) override {
        append("saveTitleGenerationModel", string_value(model));
    }
    void save_title_generation_prompt(const std::string& prompt) override {
        append("saveTitleGenerationPrompt", string_value(std::optional<std::string>(prompt)));
    }
    void save_title_generation_notifications_enabled(bool enabled) override {
        append("saveTitleGenerationNotificationsEnabled", boolean_value(enabled));
    }
    void save_access_past_conversations(bool enabled) override {
        append("saveAccessPastConversations", boolean_value(enabled));
    }
    void save_access_saved_memories(bool enabled) override {
        append("saveAccessSavedMemories", boolean_value(enabled));
    }
    void save_access_active_memory(bool enabled) override {
        append("saveAccessActiveMemory", boolean_value(enabled));
    }
    void save_access_skills(bool enabled) override { append("saveAccessSkills", boolean_value(enabled)); }
    void save_rag_search_enabled(bool enabled) override {
        append("saveRagSearchEnabled", boolean_value(enabled));
    }
    void save_model_search_method(const std::string& method) override {
        append("saveModelSearchMethod", string_value(std::optional<std::string>(method)));
    }
    void save_manual_search_method(const std::string& method) override {
        append("saveManualSearchMethod", string_value(std::optional<std::string>(method)));
    }
    void save_embedding_models(const std::vector<secrets::EmbeddingModelConfig>& models) override {
        append("saveEmbeddingModels", embedding_value(models));
    }
    void set_active_embedding_model_id(const std::string& id) override {
        append("setActiveEmbeddingModelId", string_value(std::optional<std::string>(id)));
    }
    void save_app_language(const std::string& language) override {
        append("saveAppLanguage", string_value(std::optional<std::string>(language)));
    }
    void save_web_search_enabled(bool enabled) override {
        append("saveWebSearchEnabled", boolean_value(enabled));
    }
    void save_web_search_provider(const std::string& provider) override {
        append("saveWebSearchProvider", string_value(std::optional<std::string>(provider)));
    }
    void save_web_search_num_results(int count) override {
        append("saveWebSearchNumResults", int_value(count));
    }
    void save_web_search_base_url(const std::string& url) override {
        append("saveWebSearchBaseUrl", string_value(std::optional<std::string>(url)));
    }
    void update_image_quality(const std::optional<std::string>& mode,
                              const std::optional<int>& custom_max_long_edge,
                              const std::optional<int>& custom_jpeg_quality) override {
        std::string value = "{\"mode\":";
        if (mode.has_value()) append_quoted(value, *mode);
        else value += "null";
        value += ",\"customMaxLongEdge\":";
        if (custom_max_long_edge.has_value()) value += std::to_string(*custom_max_long_edge);
        else value += "null";
        value += ",\"customJpegQuality\":";
        if (custom_jpeg_quality.has_value()) value += std::to_string(*custom_jpeg_quality);
        else value += "null";
        value.push_back('}');
        append("imageQuality.update", value);
    }
    void save_image_gen_enabled(bool enabled) override {
        append("saveImageGenEnabled", boolean_value(enabled));
    }
    void save_image_gen_model(const std::optional<std::string>& model) override {
        append("saveImageGenModel", string_value(model));
    }
    void save_image_gen_size(const std::string& size) override {
        append("saveImageGenSize", string_value(std::optional<std::string>(size)));
    }
    void save_search_context_window(int window) override {
        append("saveSearchContextWindow", int_value(window));
    }
    void save_search_match_limit(int limit) override {
        append("saveSearchMatchLimit", int_value(limit));
    }
    void save_rag_threshold(float threshold) override {
        append("saveRagThreshold", float_value(threshold));
    }
    void save_auto_cache_enabled(bool enabled) override {
        append("saveAutoCacheEnabled", boolean_value(enabled));
    }
    void save_show_uncached_notification(bool enabled) override {
        append("saveShowUncachedNotification", boolean_value(enabled));
    }
    void save_auto_update_check(bool enabled) override {
        append("saveAutoUpdateCheck", boolean_value(enabled));
    }
    void save_image_transcription_enabled(bool enabled) override {
        append("saveImageTranscriptionEnabled", boolean_value(enabled));
    }
    void save_image_transcription_enabled_models(
        const std::vector<std::string>& models) override {
        append("saveImageTranscriptionEnabledModels", strings_value(models));
    }
    void save_image_transcription_model(const std::optional<std::string>& model) override {
        append("saveImageTranscriptionModel", string_value(model));
    }
    void save_image_transcription_batch_size(int size) override {
        append("saveImageTranscriptionBatchSize", int_value(size));
    }
    void save_image_transcription_prompt(const std::string& prompt) override {
        append("saveImageTranscriptionPrompt", string_value(std::optional<std::string>(prompt)));
    }
    void save_shell_enabled(bool enabled) override { append("saveShellEnabled", boolean_value(enabled)); }
    void save_shell_confirm_enabled(bool enabled) override {
        append("saveShellConfirmEnabled", boolean_value(enabled));
    }
    void save_ask_user_enabled(bool enabled) override {
        append("saveAskUserEnabled", boolean_value(enabled));
    }
    void save_shell_devices(const std::vector<secrets::ShellDeviceConfig>& devices) override {
        append("saveShellDevices", shell_value(devices));
    }
    void save_automation_tools_enabled(bool enabled) override {
        append("saveAutomationToolsEnabled", boolean_value(enabled));
    }
    void save_exact_execution_enabled(bool enabled) override {
        append("saveExactExecutionEnabled", boolean_value(enabled));
    }
    void save_automation_wake_lock_enabled(bool enabled) override {
        append("saveAutomationWakeLockEnabled", boolean_value(enabled));
    }
    void save_custom_providers(const std::vector<archive::CustomProviderConfig>& providers) override {
        append("saveCustomProviders", provider_value(providers));
    }
    void save_mcp_servers(const std::vector<secrets::McpServerConfig>& servers) override {
        append("saveMcpServers", mcp_value(servers));
    }
    void save_proxy_enabled(bool enabled) override { append("saveProxyEnabled", boolean_value(enabled)); }
    void save_proxy_type(const std::string& type) override {
        append("saveProxyType", string_value(std::optional<std::string>(type)));
    }
    void save_proxy_host(const std::string& host) override {
        append("saveProxyHost", string_value(std::optional<std::string>(host)));
    }
    void save_proxy_port(const std::string& port) override {
        append("saveProxyPort", string_value(std::optional<std::string>(port)));
    }
    void save_proxy_username(const std::string& username) override {
        append("saveProxyUsername", string_value(std::optional<std::string>(username)));
    }
    void save_proxy_bypass(const std::string& bypass) override {
        append("saveProxyBypass", string_value(std::optional<std::string>(bypass)));
    }
    void save_show_documentation_fab(bool enabled) override {
        append("saveShowDocumentationFab", boolean_value(enabled));
    }
    void save_theme_mode(const std::string& mode) override {
        append("saveThemeMode", string_value(std::optional<std::string>(mode)));
    }
    void save_amoled_enabled(bool enabled) override { append("saveAmoledEnabled", boolean_value(enabled)); }
    void save_color_scheme(const std::string& scheme) override {
        append("saveColorScheme", string_value(std::optional<std::string>(scheme)));
    }
    void save_dynamic_color(bool enabled) override { append("saveDynamicColor", boolean_value(enabled)); }
    void save_blur_effects_enabled(bool enabled) override {
        append("saveBlurEffectsEnabled", boolean_value(enabled));
    }
    void save_reduce_motion(bool enabled) override { append("saveReduceMotion", boolean_value(enabled)); }
    void save_stick_to_bottom(bool enabled) override {
        append("saveStickToBottom", boolean_value(enabled));
    }
    void save_parse_inline_dollar_math(bool enabled) override {
        append("saveParseInlineDollarMath", boolean_value(enabled));
    }
    void save_auto_wrap_code_blocks(bool enabled) override {
        append("saveAutoWrapCodeBlocks", boolean_value(enabled));
    }
    void save_haptics_enabled(bool enabled) override { append("saveHapticsEnabled", boolean_value(enabled)); }
    void save_detailed_token_usage(bool enabled) override {
        append("saveDetailedTokenUsage", boolean_value(enabled));
    }
    void save_tool_call_display_mode(const std::string& mode) override {
        append("saveToolCallDisplayMode", string_value(std::optional<std::string>(mode)));
    }
    void save_thinking_segment_display_mode(const std::string& mode) override {
        append("saveThinkingSegmentDisplayMode", string_value(std::optional<std::string>(mode)));
    }
    void save_auto_expand_active_group(bool enabled) override {
        append("saveAutoExpandActiveGroup", boolean_value(enabled));
    }
    void save_scheme_style(const std::string& style) override {
        append("saveSchemeStyle", string_value(std::optional<std::string>(style)));
    }
    void save_font_preference(const std::string& preference) override {
        append("saveFontPreference", string_value(std::optional<std::string>(preference)));
    }
    void save_custom_font_path(const std::string& path) override {
        append("saveCustomFontPath", string_value(std::optional<std::string>(path)));
    }
    void save_custom_font_name(const std::string& name) override {
        append("saveCustomFontName", string_value(std::optional<std::string>(name)));
    }
    void save_default_temperature(const std::optional<float>& value) override {
        append("saveDefaultTemperature", float_value(value));
    }
    void save_default_max_tokens(const std::optional<int>& value) override {
        if (value.has_value()) {
            append("saveDefaultMaxTokens", int_value(*value));
        } else {
            append("saveDefaultMaxTokens", "{\"int\":null}");
        }
    }
    void save_default_top_p(const std::optional<float>& value) override {
        append("saveDefaultTopP", float_value(value));
    }
    void save_default_frequency_penalty(const std::optional<float>& value) override {
        append("saveDefaultFrequencyPenalty", float_value(value));
    }
    void save_default_presence_penalty(const std::optional<float>& value) override {
        append("saveDefaultPresencePenalty", float_value(value));
    }
    void set_active_system_prompt_id(const std::optional<std::string>& id) override {
        append("setActiveSystemPromptId", string_value(id));
    }
    void reset_portable_settings_for_import() override {
        append("resetPortableSettingsForImport", "{\"none\":true}");
    }
    void invalidate_portable_model_caches() override {
        append("invalidatePortableModelCaches", "{\"none\":true}");
    }
};

/** The conversation-settings map value the golden's saveConversationSettingsMap carries. */
std::string conversation_settings_value(
    const agora::core::export_extra_settings::ConversationSettings& one) {
    auto opt_int = [](const std::optional<int>& v) {
        return v.has_value() ? std::to_string(*v) : std::string("null");
    };
    auto opt_float = [](const std::optional<float>& v) {
        if (!v.has_value()) return std::string("null");
        return "\"" + agora::core::openai::format_kotlin_float(*v) + "\"";
    };
    auto opt_bool = [](const std::optional<bool>& v) -> std::string {
        if (!v.has_value()) return "null";
        return *v ? "true" : "false";
    };
    auto opt_str = [](const std::optional<std::string>& v) {
        if (!v.has_value()) return std::string("null");
        std::string out;
        append_quoted(out, *v);
        return out;
    };
    std::string out = "{";
    out += "\"contextWindow\":" + opt_int(one.context_window);
    out += ",\"temperature\":" + opt_float(one.temperature);
    out += ",\"maxTokens\":" + opt_int(one.max_tokens);
    out += ",\"topP\":" + opt_float(one.top_p);
    out += ",\"frequencyPenalty\":" + opt_float(one.frequency_penalty);
    out += ",\"presencePenalty\":" + opt_float(one.presence_penalty);
    out += ",\"codeExecutionEnabled\":" + opt_bool(one.code_execution_enabled);
    out += ",\"googleSearchEnabled\":" + opt_bool(one.google_search_enabled);
    out += ",\"openAiWebSearchEnabled\":" + opt_bool(one.open_ai_web_search_enabled);
    out += ",\"thinkingEnabled\":" + opt_bool(one.thinking_enabled);
    out += ",\"thinkingLevel\":" + opt_str(one.thinking_level);
    out += ",\"thinkingBudgetEnabled\":" + opt_bool(one.thinking_budget_enabled);
    out += ",\"thinkingBudgetTokens\":" + opt_int(one.thinking_budget_tokens);
    out += ",\"openAiServiceTierEnabled\":" + opt_bool(one.open_ai_service_tier_enabled);
    out += ",\"openAiServiceTier\":" + opt_str(one.open_ai_service_tier);
    out += ",\"webSearchEnabled\":" + opt_bool(one.web_search_enabled);
    out += ",\"shellEnabled\":" + opt_bool(one.shell_enabled);
    out += ",\"lowContextModeEnabled\":" + opt_bool(one.low_context_mode_enabled);
    out += "}";
    return out;
}

void run_scenario(const std::filesystem::path& path) {
    const std::string stem = path.stem().string();
    const json::ParseResult parsed = json::parse(read_file(path));
    if (!parsed.value || parsed.value->kind != Kind::Object)
        scenario_fail(stem + ": not an object");
    const Value& root = *parsed.value;

    const Value* steps = field(root, "steps");
    if (steps == nullptr || steps->kind != Kind::Array)
        scenario_fail(stem + ": scenario needs steps");

    const std::filesystem::path stand(AGORA_FIXTURE_DATA_IMPORTER_DIR);
    SqliteExecutor sql(stand / "platform" / "schema.sql");

    // The importer's dependencies. A preview reads only the archive and the
    // graph counter, so these stand empty; the import steps drive them for
    // real when their own test lands.
    agora::core::chat_dao::ChatDao chat(sql);
    agora::core::semantic_index::SemanticIndexDao semantic(sql);
    agora::core::maintenance_debt::MaintenanceDebtDao debt(sql);
    agora::core::memory::MemoryManager memory((stand / "platform").string());
    agora::core::skill::SkillManager skill((stand / "platform").string());
    // The recording sink the active import step is logging to; the settings-transfer
    // apply writes saveConversationSettingsMap into it (the reference's SettingsManager).
    RecordingStore* active_sink = nullptr;
    data_import::ConversationSettingsTransfer transfers(
        chat, [] {},
        [&active_sink](const std::vector<std::pair<std::string,
                        agora::core::export_extra_settings::ConversationSettings>>& settings,
                       bool replace) {
            if (active_sink == nullptr) return;
            std::string out = "{\"conversationSettings\":{";
            bool first = true;
            for (const auto& [id, one] : settings) {
                if (!first) out.push_back(',');
                first = false;
                out += "\"" + id + "\":" + conversation_settings_value(one);
            }
            out += "}}";
            active_sink->append("saveConversationSettingsMap", out);
            (void)replace;
        });
    data_import::DataImporter importer(
        chat, semantic, debt, sql, memory, skill, data_import::SettingsView{},
        data_import::SettingsWrites{}, transfers, data_import::Platform{});

    const json::ParseResult golden =
        json::parse(read_file(stand / "golden" / (stem + ".trace")));
    if (!golden.value) scenario_fail(stem + ": golden is not json");
    const Value* golden_steps = field(*golden.value, "steps");
    if (golden_steps == nullptr || golden_steps->kind != Kind::Array)
        scenario_fail(stem + ": golden needs steps");

    // The golden's steps, grouped by op and kept in order: the i-th preview
    // step of the scenario is compared with the i-th preview step of the
    // golden, and the i-th font step with the i-th font step, so a scenario
    // that previews several archives or reads several fonts is checked step
    // for step.
    auto golden_results_for = [&](std::string_view wanted) {
        std::vector<const Value*> found;
        for (const Value& golden_step : golden_steps->elements) {
            if (required_text(golden_step, "op") == wanted)
                found.push_back(field(golden_step, "result"));
        }
        return found;
    };
    const std::vector<const Value*> golden_previews = golden_results_for("preview");
    const std::vector<const Value*> golden_fonts = golden_results_for("font");
    // The golden's import steps, kept whole (not just their result): each
    // carries the result, the settings-call log, and the post-import captures.
    std::vector<const Value*> golden_imports;
    for (const Value& golden_step : golden_steps->elements) {
        if (required_text(golden_step, "op") == "import") golden_imports.push_back(&golden_step);
    }
    std::size_t preview_index = 0;
    std::size_t font_index = 0;
    std::size_t import_index = 0;

    // What one step says, asked of the port: a preview reads an archive it is
    // given, a font reads the family out of the bytes of a TTF. (Import needs
    // the whole import harness and is driven by its own test.)
    auto actual_for = [&](const Value& step, std::string_view op, std::size_t index) -> Value {
        if (op == "preview") {
            const std::string bytes = archive_bytes(step);
            backup::NativeBackupArchive archive = backup::NativeBackupArchive::open_bytes(bytes);
            return preview_value(importer.preview(archive));
        }
        // font: the base64 is the font itself; read its name table from a file.
        const Value* encoded = field(step, "base64");
        if (encoded == nullptr || encoded->kind != Kind::String)
            scenario_fail("font step needs a base64 font");
        const std::vector<std::uint8_t> font = base64_decode(encoded->text);
        // The reference writes each font to a file named for its step, so a
        // font it cannot read falls back to that name ("font-2" in the golden);
        // name ours the same way for the fallback to match.
        const std::filesystem::path temp =
            std::filesystem::temp_directory_path() /
            ("font-" + std::to_string(index) + ".ttf");
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(font.data()),
                      static_cast<std::streamsize>(font.size()));
        }
        const agora::core::font::FontMetadata meta =
            agora::core::font::read_font_meta(temp);
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        return json_object({
            {"family", json_string(meta.family)},
            {"subfamily", meta.subfamily.has_value() ? json_string(*meta.subfamily)
                                                     : json_literal("null")},
        });
    };

    for (const Value& step : steps->elements) {
        const std::string op = required_text(step, "op");
        if (op == "import") {
            // An import: the archive, the decisions the scenario was given, and
            // everything the import left behind -- the call log, the view of the
            // settings it read, the files it wrote, the tables it changed -- which
            // is what the golden pins.
            const std::string bytes = archive_bytes(step);
            backup::NativeBackupArchive archive = backup::NativeBackupArchive::open_bytes(bytes);

            // The decisions, as the enum the importer takes.
            std::map<data_export::ExportCategory, data_import::ImportStrategy> decisions;
            if (const Value* raw = field(step, "decisions");
                raw != nullptr && raw->kind == Kind::Object) {
                static const std::map<std::string, data_export::ExportCategory> kCategories = {
                    {"CONVERSATIONS", data_export::ExportCategory::Conversations},
                    {"MEMORIES", data_export::ExportCategory::Memories},
                    {"SYSTEM_PROMPTS", data_export::ExportCategory::SystemPrompts},
                    {"SETTINGS", data_export::ExportCategory::Settings},
                    {"API_KEYS", data_export::ExportCategory::ApiKeys},
                };
                static const std::map<std::string, data_import::ImportStrategy> kStrategies = {
                    {"MERGE", data_import::ImportStrategy::Merge},
                    {"REPLACE", data_import::ImportStrategy::Replace},
                    {"SKIP", data_import::ImportStrategy::Skip},
                };
                for (const Member& member : raw->members) {
                    const auto category = kCategories.find(member.key);
                    const auto strategy = kStrategies.find(member.value.text);
                    if (category == kCategories.end() || strategy == kStrategies.end())
                        scenario_fail(stem + ": unknown decision " + member.key + "=" + member.value.text);
                    decisions[category->second] = strategy->second;
                }
            }

            // The recording sinks: what the import asked the settings to save.
            // saveSystemPrompts is called directly (it is in no sink); the rest
            // go through the archive sink. Both log with the reference's names.
            RecordingStore sink;
            std::vector<float> progress;
            data_import::SettingsWrites writes;
            writes.archive_sink = &sink;
            writes.save_system_prompts =
                [&sink](const std::vector<data_export::SystemPromptEntry>& prompts) {
                    sink.append("saveSystemPrompts", RecordingStore::prompts_value(prompts));
                };
            // A deterministic platform: uuids numbered from zero (the golden's
            // <uuid:N> masks) and a scratch files dir the import writes media to.
            int uuid_counter = 0;
            const std::filesystem::path files_dir =
                std::filesystem::temp_directory_path() / "agora-import-parity";
            std::filesystem::create_directories(files_dir);
            data_import::Platform platform;
            platform.files_dir = files_dir;
            platform.package_name = "agora.test";
            platform.new_uuid = [&uuid_counter] {
                char buffer[37];
                std::snprintf(buffer, sizeof(buffer), "%08x-0000-4000-8000-%012x", uuid_counter,
                              uuid_counter);
                ++uuid_counter;
                return std::string(buffer);
            };
            platform.file_provider_uri = [](const std::filesystem::path& path) {
                return "content://agora.test.files/" + path.filename().string();
            };
            // The initial settings: the scenario's seed system prompts (a MERGE
            // folds the archive into these).
            const std::vector<data_export::SystemPromptEntry> seed_prompts = read_prompt_seed(root);
            data_import::SettingsView view;
            view.system_prompts = [&seed_prompts] { return seed_prompts; };
            data_import::DataImporter import_importer(
                chat, semantic, debt, sql, memory, skill, view, writes,
                transfers, platform);
            active_sink = &sink;
            const data_import::ImportResult result = import_importer.import(
                archive, decisions, [&](float at) { progress.push_back(at); });
            active_sink = nullptr;

            // The result: the counts and errors, and the progress as the
            // reference writes it -- each float as a string.
            std::string progress_json = "[";
            for (std::size_t i = 0; i < progress.size(); ++i) {
                if (i != 0) progress_json.push_back(',');
                progress_json += "\"" + agora::core::openai::format_kotlin_float(progress[i]) + "\"";
            }
            progress_json += "]";
            Value progress_value;
            {
                const json::ParseResult parsed_progress = json::parse(progress_json);
                progress_value = parsed_progress.value ? *parsed_progress.value : Value{};
                progress_value.kind = Kind::Array;
            }
            std::vector<Value> error_values;
            for (const std::string& message : result.errors) error_values.push_back(json_string(message));
            const Value actual = json_object({
                {"result",
                 json_object({
                     {"conversationsImported", json_long(result.conversations_imported)},
                     {"tasksImported", json_long(result.tasks_imported)},
                     {"loopsImported", json_long(result.loops_imported)},
                     {"memoriesImported", json_long(result.memories_imported)},
                     {"systemPromptsImported", json_long(result.system_prompts_imported)},
                     {"settingsImported", json_bool(result.settings_imported)},
                     {"apiKeysImported", json_bool(result.api_keys_imported)},
                     {"errors", json_array(error_values)},
                 })},
                {"progress", progress_value},
            });

            // The golden's matching import step (i-th import step, in order).
            if (import_index >= golden_imports.size()) {
                scenario_diff(stem + ": golden has fewer import steps than the scenario");
                continue;
            }
            const Value& golden_step = *golden_imports[import_index];
            ++import_index;

            // The golden pins the whole step: result, settingsCalls, settingsView,
            // memory, skills, files, tables, appliedImports. The result is compared
            // here; the rest are compared against what this test captured (empty
            // where the import wrote nothing, as in the no-decisions case).
            const Value* golden_result = field(golden_step, "result");
            if (golden_result == nullptr) {
                scenario_diff(stem + ": golden import has no result");
            } else {
                const Value actual_result = masked(actual);
                if (!json_equal(actual_result, *golden_result))
                    scenario_diff(stem + "\n  actual:   " + excerpt(actual_result) +
                                  "\n  expected: " + excerpt(*golden_result));
            }

            // The call log the recording sink kept: the golden's settingsCalls.
            const Value golden_calls_field = [&] {
                Value v;
                v.kind = Kind::Array;
                const json::ParseResult parsed_calls = json::parse("[" + sink.calls + "]");
                if (parsed_calls.value) v = *parsed_calls.value;
                return v;
            }();
            const Value actual_calls = masked(golden_calls_field);
            const Value* golden_calls = field(golden_step, "settingsCalls");
            if (golden_calls == nullptr) {
                scenario_diff(stem + ": golden has no settingsCalls");
            } else if (!json_equal(actual_calls, *golden_calls)) {
                scenario_diff(stem + "\n  settingsCalls actual:   " + excerpt(actual_calls) +
                              "\n  settingsCalls expected: " + excerpt(*golden_calls));
            }
            continue;
        }

        const std::size_t index = op == "preview" ? preview_index++ : font_index++;
        const std::vector<const Value*>& golden_list =
            op == "preview" ? golden_previews : golden_fonts;
        if (index >= golden_list.size()) {
            scenario_diff(stem + ": golden has fewer " + op + " steps than the scenario");
            continue;
        }
        const Value* preview_font_golden = golden_list[index];
        if (preview_font_golden == nullptr) {
            scenario_diff(stem + ": golden has no result");
            continue;
        }

        const Value actual = actual_for(step, op, index);
        if (!json_equal(actual, *preview_font_golden))
            scenario_diff(stem + "\n  actual:   " + excerpt(actual) +
                          "\n  expected: " + excerpt(*preview_font_golden));
    }
}


}  // namespace

TEST(data_importer_previews_every_archive_as_the_reference_does) {
    const std::filesystem::path root_dir = AGORA_FIXTURE_DATA_IMPORTER_DIR;
    const std::filesystem::path scenarios_dir = root_dir / "scenarios";
    CHECK(std::filesystem::is_directory(scenarios_dir));
    std::vector<std::filesystem::path> scenarios;
    for (const auto& entry : std::filesystem::directory_iterator(scenarios_dir)) {
        if (entry.path().extension() == ".json") scenarios.push_back(entry.path());
    }
    std::sort(scenarios.begin(), scenarios.end());
    CHECK(scenarios.size() >= 4);
    for (const auto& scenario : scenarios) run_scenario(scenario);
}
