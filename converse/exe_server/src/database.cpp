#include "database.hpp"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <format>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "sql.hpp"

namespace lg = converse::logging;

void bind_arg(sqlite3_stmt *stmt, int &index, int value) {
    index++;
    lg::write(lg::level::trace, "bind_arg<int>({},{})", index, value);
    if (sqlite3_bind_int(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_int failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, sqlite3_int64 value) {
    index++;
    lg::write(lg::level::trace, "bind_arg<sqlite3_int64>({},{})", index, value);
    if (sqlite3_bind_int64(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_int64 failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, unsigned long value) {
    int value_ = static_cast<int>(value);
    bind_arg(stmt, index, value_);
}

void bind_arg(sqlite3_stmt *stmt, int &index, unsigned long long value) {
    int value_ = static_cast<int>(value);
    bind_arg(stmt, index, value_);
}

void bind_arg(sqlite3_stmt *stmt, int &index, double value) {
    index++;
    lg::write(lg::level::trace, "bind_arg<double>({},{})", index, value);
    if (sqlite3_bind_double(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_double failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, std::string_view value) {
    index++;
    lg::write(lg::level::trace, "bind_arg<std::string_view>({},{})", index,
              value);
    if (sqlite3_bind_text(stmt, index, value.data(),
                          static_cast<int>(value.size()),
                          SQLITE_STATIC) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_text failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, std::nullptr_t) {
    index++;
    lg::write(lg::level::trace, "bind_arg<nullptr_t>({},nullptr)", index);
    if (sqlite3_bind_null(stmt, index) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_null failed");
    }
}

Db::Db(const std::string &name)
    : db_(nullptr, &sqlite3_close), filename_(std::format("{}.db", name)) {
    sqlite3 *db;
    std::filesystem::path path(filename_);
    bool exists = std::filesystem::exists(path);
    if (sqlite3_open(filename_.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error(
            std::format("can't open database: {}", sqlite3_errmsg(db)));
    }
    db_.reset(db);
    if (!exists) {
        std::stringstream buf(converse::sql::MAIN);
        std::string stmt;
        while (std::getline(buf, stmt, ';')) {
            boost::trim_right(stmt);
            if (!stmt.empty()) {
                execute(stmt + ";");
            }
        }
    }
}

std::string Db::get_bytes() {
    sqlite3_int64 size;
    unsigned char *bytes = sqlite3_serialize(db_.get(), nullptr, &size, 0);
    return std::string(reinterpret_cast<char *>(bytes),
                       static_cast<size_t>(size));
}

void Db::set_bytes(std::string &bytes) {
    sqlite3 *mem_db = nullptr;
    sqlite3 *file_db = nullptr;
    unsigned char *buffer = reinterpret_cast<unsigned char *>(bytes.data());
    sqlite3_int64 size = bytes.size();
    if (sqlite3_open(":memory:", &mem_db) != SQLITE_OK) {
        lg::write(lg::level::error, "Failed to open memory DB: {}",
                  sqlite3_errmsg(mem_db));
        return;
    }

    if (sqlite3_deserialize(mem_db, "main", buffer, size, size,
                            SQLITE_DESERIALIZE_READONLY |
                                SQLITE_DESERIALIZE_RESIZEABLE) != SQLITE_OK) {
        lg::write(lg::level::error, "Failed to deserialize: {}",
                  sqlite3_errmsg(mem_db));
        sqlite3_close(mem_db);
        return;
    }

    if (sqlite3_open(filename_.c_str(), &file_db) != SQLITE_OK) {
        lg::write(lg::level::error, "Failed to open file DB: {}",
                  sqlite3_errmsg(file_db));
        sqlite3_close(mem_db);
        return;
    }

    // Step 4: Use backup API to copy memory DB → file DB
    sqlite3_backup *backup =
        sqlite3_backup_init(file_db, "main", mem_db, "main");
    if (backup) {
        sqlite3_backup_step(backup, -1);  // copy everything
        sqlite3_backup_finish(backup);
    }

    if (sqlite3_errcode(file_db) != SQLITE_OK) {
        lg::write(lg::level::error, "Failed to backup: {}",
                  sqlite3_errmsg(file_db));
        sqlite3_close(file_db);
        sqlite3_close(mem_db);
        return;
    }

    sqlite3_close(mem_db);
    sqlite3_close(file_db);
}
