#include "converse/logging/core.hpp"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

const auto format = "[%TimeStamp%] [%Severity%] %Message%";

namespace converse {
namespace logging {
void init(sink_type t, const std::string &name) {
    boost::log::core::get()->remove_all_sinks();
    boost::log::add_common_attributes();
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
}  // namespace logging
}  // namespace converse
