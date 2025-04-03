add_library(lib_logging STATIC)
target_sources(lib_logging PRIVATE src/converse/logging/core.cpp)
target_include_directories(lib_logging PUBLIC src)
target_link_libraries(lib_logging PRIVATE Boost::date_time Boost::log
                                          Boost::log_setup)
