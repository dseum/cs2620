#pragma once

#include <boost/log/trivial.hpp>
#include <format>

namespace converse {
namespace logging {

enum class sink_type { null, file, console };

enum class level {
    trace = boost::log::trivial::trace,
    debug = boost::log::trivial::debug,
    info = boost::log::trivial::info,
    error = boost::log::trivial::error
};

void init(level lvl, sink_type t, const std::string &name = "");

void write(level lvl, std::string_view message);

template <typename... Args>
void write(level lvl, std::format_string<Args...> fmt, Args &&...args) {
    std::string formatted_message =
        std::format(fmt, std::forward<Args>(args)...);
    write(lvl, formatted_message);
}
}  // namespace logging
}  // namespace converse
