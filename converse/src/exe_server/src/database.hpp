#pragma once

#include <sqlite3.h>

#include <format>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

void bind_arg(sqlite3_stmt *stmt, int &index, int value);

void bind_arg(sqlite3_stmt *stmt, int &index, sqlite3_int64 value);

void bind_arg(sqlite3_stmt *stmt, int &index, unsigned long long value);

void bind_arg(sqlite3_stmt *stmt, int &index, double value);

void bind_arg(sqlite3_stmt *stmt, int &index, std::string_view value);

void bind_arg(sqlite3_stmt *stmt, int &index, std::nullptr_t);

template <typename T>
    requires(std::ranges::range<T> && !std::convertible_to<T, std::string_view>)

void bind_arg(sqlite3_stmt *stmt, int &index, const T &range) {
    for (const auto &value : range) {
        bind_arg(stmt, index, value);
    }
}

template <typename T>
inline T get_column_value(sqlite3_stmt *stmt, int i);

template <>
inline bool get_column_value<bool>(sqlite3_stmt *stmt, int i) {
    return sqlite3_column_int(stmt, i) != 0;
}

template <>
inline int get_column_value<int>(sqlite3_stmt *stmt, int i) {
    return sqlite3_column_int(stmt, i);
}

template <>
inline sqlite3_int64 get_column_value<sqlite3_int64>(sqlite3_stmt *stmt,
                                                     int i) {
    return sqlite3_column_int64(stmt, i);
}

template <>
inline double get_column_value<double>(sqlite3_stmt *stmt, int i) {
    return sqlite3_column_double(stmt, i);
}

template <>
inline std::string_view get_column_value<std::string_view>(sqlite3_stmt *stmt,
                                                           int i) {
    std::string_view text(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)),
        sqlite3_column_bytes(stmt, i));
    return text;
}

template <>
inline std::string get_column_value<std::string>(sqlite3_stmt *stmt, int i) {
    std::string text;
    text.assign(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)),
                sqlite3_column_bytes(stmt, i));
    return text;
}

template <typename... Ts, std::size_t... Is>
std::tuple<Ts...> fetch_tuple_impl(sqlite3_stmt *stmt, int i,
                                   std::index_sequence<Is...>) {
    return std::tuple<Ts...>(get_column_value<Ts>(stmt, i + Is)...);
}

// Wrapper around the helper function to generate the index sequence
template <typename... Ts>
std::tuple<Ts...> fetch_tuple(sqlite3_stmt *stmt, int i) {
    return fetch_tuple_impl<Ts...>(stmt, i, std::index_sequence_for<Ts...>{});
}

class Db {
    std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db;

   public:
    Db(std::string &name);
    virtual ~Db() = default;

    template <typename... Ts, typename... Args>
    std::vector<std::tuple<Ts...>> execute(std::string_view sql, Args... args) {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db.get(), sql.data(), sql.size(), &stmt,
                               nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::format(
                "sqlite3_prepare_v2 failed with {}", sqlite3_errmsg(db.get())));
        }

        int index = 0;
        (bind_arg(stmt, index, args), ...);

        std::vector<std::tuple<Ts...>> rows;

        int rc;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            rows.push_back(fetch_tuple<Ts...>(stmt, 0));
        }
        sqlite3_finalize(stmt);
        switch (rc) {
            case SQLITE_DONE: {
                break;
            }
            case SQLITE_ERROR: {
                throw std::runtime_error(std::format("sqlite3_step failed: {}",
                                                     sqlite3_errmsg(db.get())));
            }
        }

        return rows;
    }
};
