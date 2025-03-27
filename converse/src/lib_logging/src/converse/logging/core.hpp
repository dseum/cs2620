#pragma once

#include <boost/log/trivial.hpp>
#include <format>

namespace converse {
namespace logging {

enum class sink_type { null, file, console };

void init(sink_type t, const std::string &name = "");

enum class level { trace, debug, info, error };

template <typename... Args>
void write(level lvl, std::format_string<Args...> fmt, Args &&...args) {
    std::string formatted_message =
        std::format(fmt, std::forward<Args>(args)...);

    switch (lvl) {
        case level::trace:
            BOOST_LOG_TRIVIAL(trace) << formatted_message << "\n";
            break;
        case level::debug:
            BOOST_LOG_TRIVIAL(debug) << formatted_message << "\n";
            break;
        case level::info:
            BOOST_LOG_TRIVIAL(info) << formatted_message << "\n";
            break;
        case level::error:
            BOOST_LOG_TRIVIAL(error) << formatted_message << "\n";
            break;
    }
}
}  // namespace logging
}  // namespace converse
