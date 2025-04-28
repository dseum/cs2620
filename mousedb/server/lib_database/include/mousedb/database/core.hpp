#pragma once

#include <optional>
#include <string>

namespace mousedb {
namespace database {

class Database {
   public:
    Database(const std::string &path);

    auto find(const std::string &key) -> std::optional<std::string_view>;
    auto insert(const std::string &key, const std::string &value) -> void;
    auto erase(const std::string &key) -> void;

   private:
    std::string path_;
};
}  // namespace database
}  // namespace mousedb
