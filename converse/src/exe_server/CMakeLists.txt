set(SQL_HPP src/sql.hpp)
file(READ src/main.sql MAIN_SQL)
file(WRITE ${SQL_HPP} "#pragma once\n")
file(APPEND ${SQL_HPP} "namespace converse {\n")
file(APPEND ${SQL_HPP} "namespace sql {\n")
file(APPEND ${SQL_HPP} "constexpr char MAIN[] = \"")
string(REGEX REPLACE "[ \t\r\n]+" " " MAIN_SQL "${MAIN_SQL}") # Converts all
# whitespace to a single space
string(REPLACE "\"" "\\\"" MAIN_SQL "${MAIN_SQL}")
file(APPEND ${SQL_HPP} "${MAIN_SQL}")
file(APPEND ${SQL_HPP} "\";\n")
file(APPEND ${SQL_HPP} "} // namespace sql\n")
file(APPEND ${SQL_HPP} "} // namespace converse\n")
add_custom_target(sql_hpp DEPENDS ${SQL_HPP})

add_executable(exe_server)
set_target_properties(exe_server PROPERTIES OUTPUT_NAME exe)
target_sources(exe_server PRIVATE src/main.cpp src/database.cpp src/server.cpp)
target_include_directories(exe_server PRIVATE src)
target_link_libraries(
  exe_server
  PRIVATE Boost::algorithm Boost::program_options lib_logging lib_proto
          unofficial::argon2::libargon2 unofficial::sqlite3::sqlite3)
add_dependencies(exe_server sql_hpp)
