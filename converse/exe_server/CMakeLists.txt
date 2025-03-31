cmake_minimum_required(VERSION 3.30)

project(converse_server)

# Sets CMake flags
set(CMAKE_BUILD_TYPE
    "Debug"
    CACHE STRING "choose the build type: Debug Release")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")

# Sets compiler definitions
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sets compiler flags
set(CXX_FLAGS
    -Wall
    -Wconversion
    -Wextra
    -Wpedantic
    -Wshadow
    -Wunused
    -march=native)
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(CXX_FLAGS ${CXX_FLAGS} -g -O0)
  set(CMAKE_BUILD_TYPE Release)
elseif(CMAKE_BUILD_TYPE STREQUAL Release)
  set(CXX_FLAGS ${CXX_FLAGS} -DNDEBUG -O3)
else()
  message(FATAL_ERROR "unknown build type: ${CMAKE_BUILD_TYPE}")
endif()

# Finds packages
find_package(Boost CONFIG REQUIRED COMPONENTS algorithm date_time log log_setup
                                              program_options)
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Adds subdirectories
add_subdirectory(../lib_logging ${CMAKE_BINARY_DIR}/lib_logging)
add_subdirectory(../lib_proto ${CMAKE_BINARY_DIR}/lib_proto)

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
target_sources(exe_server PRIVATE src/main.cpp src/database.cpp src/server.cpp src/link.cpp)
target_include_directories(exe_server PRIVATE src)
target_link_libraries(
  exe_server
  PRIVATE Boost::algorithm Boost::program_options lib_logging lib_proto
          unofficial::argon2::libargon2 unofficial::sqlite3::sqlite3)
add_dependencies(exe_server sql_hpp)
