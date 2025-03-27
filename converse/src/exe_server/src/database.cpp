#include "database.hpp"

#include <format>
#include <memory>
#include <stdexcept>

void bind_arg(sqlite3_stmt *stmt, int &index, int value) {
    index++;
    if (sqlite3_bind_int(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_int failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, sqlite3_int64 value) {
    index++;
    if (sqlite3_bind_int64(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_int64 failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, unsigned long long value) {
    index++;
    int value_ = static_cast<sqlite3_int64>(value);
    bind_arg(stmt, index, value_);
}

void bind_arg(sqlite3_stmt *stmt, int &index, double value) {
    index++;
    if (sqlite3_bind_double(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_double failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, std::string_view value) {
    index++;
    if (sqlite3_bind_text(stmt, index, value.data(), value.size(),
                          SQLITE_STATIC) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_text failed");
    }
}

void bind_arg(sqlite3_stmt *stmt, int &index, std::nullptr_t) {
    index++;
    if (sqlite3_bind_null(stmt, index) != SQLITE_OK) {
        throw std::runtime_error("sqlite3_bind_null failed");
    }
}

Db::Db() : db(nullptr, &sqlite3_close) {
    sqlite3 *db_;
    if (sqlite3_open("main.db", &db_) != SQLITE_OK) {
        throw std::runtime_error(
            std::format("can't open database: {}", sqlite3_errmsg(db.get())));
    }
    db.reset(db_);
}
