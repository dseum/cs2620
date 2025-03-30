#include "converse/logging/core.hpp"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

const auto format = "[%TimeStamp%] [%Severity%] %Message%";

namespace converse {
namespace logging {
void init(level lvl, sink_type t, const std::string &name) {
    boost::log::core::get()->remove_all_sinks();
    boost::log::add_common_attributes();
    boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                        std::to_underlying(lvl));
    switch (t) {
        case sink_type::null:
            break;
        case sink_type::file: {
            boost::log::add_file_log(boost::log::keywords::file_name = name,
                                     boost::log::keywords::auto_flush = true,
                                     boost::log::keywords::format = format);
            break;
        }
        case sink_type::console: {
            boost::log::add_console_log(std::cout,
                                        boost::log::keywords::auto_flush = true,
                                        boost::log::keywords::format = format);
            break;
        }
    }
}

void write(level lvl, std::string_view message) {
    switch (lvl) {
        case level::trace:
            BOOST_LOG_TRIVIAL(trace) << message << "\n";
            break;
        case level::debug:
            BOOST_LOG_TRIVIAL(debug) << message << "\n";
            break;
        case level::info:
            BOOST_LOG_TRIVIAL(info) << message << "\n";
            break;
        case level::error:
            BOOST_LOG_TRIVIAL(error) << message << "\n";
            break;
    }
}
}  // namespace logging
}  // namespace converse
