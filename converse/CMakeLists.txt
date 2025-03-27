cmake_minimum_required(VERSION 3.30)

project(Converse LANGUAGES CXX)

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
find_package(
  Boost CONFIG REQUIRED
  COMPONENTS algorithm
             date_time
             json
             log
             log_setup
             program_options
             unit_test_framework)
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Adds subdirectories
add_subdirectory(src/exe_client)
add_subdirectory(src/exe_server)
add_subdirectory(src/lib_logging)
add_subdirectory(src/lib_proto)
