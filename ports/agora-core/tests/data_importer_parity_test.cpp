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
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <sqlite3.h>

#include "agora/core/chat_dao.hpp"
#include "agora/core/data_importer.hpp"
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
namespace backup = agora::core::native_backup_archive;
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

    void begin() override { exec_raw("BEGIN"); }
    void commit() override { exec_raw("COMMIT"); }
    void rollback() override { exec_raw("ROLLBACK"); }

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
        const Value* text = field(entry, "text");
        if (text == nullptr || text->kind != Kind::String)
            scenario_fail("archive entry needs a text");
        writer.put_entry(name, text->text);
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
    data_import::ConversationSettingsTransfer transfers(
        chat, [] {}, [](const std::vector<std::pair<std::string,
                        agora::core::export_extra_settings::ConversationSettings>>&, bool) {});
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
    std::size_t preview_index = 0;
    std::size_t font_index = 0;

    // What one step says, asked of the port: a preview reads an archive it is
    // given, a font reads the family out of the bytes of a TTF. (Import needs
    // the whole import harness and is driven by its own test.)
    auto actual_for = [&](const Value& step, std::string_view op) -> Value {
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
        const std::filesystem::path temp =
            std::filesystem::temp_directory_path() / (stem + "-font.ttf");
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
        if (op == "import") continue;  // driven by the import harness test

        const std::size_t index = op == "preview" ? preview_index++ : font_index++;
        const std::vector<const Value*>& golden_list =
            op == "preview" ? golden_previews : golden_fonts;
        if (index >= golden_list.size())
            scenario_fail(stem + ": golden has fewer " + op + " steps than the scenario");
        const Value* golden_result = golden_list[index];
        if (golden_result == nullptr) scenario_fail(stem + ": golden has no result");

        const Value actual = actual_for(step, op);
        if (!json_equal(actual, *golden_result))
            scenario_fail(stem + "\n  actual:   " + excerpt(actual) +
                          "\n  expected: " + excerpt(*golden_result));
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
