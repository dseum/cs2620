cmake_minimum_required(VERSION 3.30)

project(tempus LANGUAGES CXX)

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
set(CXX_FLAGS -Wall -Wextra -Wpedantic -Wshadow -Wunused)
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(CXX_FLAGS ${CXX_FLAGS} -g -O0)
endif()
if(CMAKE_BUILD_TYPE STREQUAL Release)
  set(CXX_FLAGS ${CXX_FLAGS} -DNDEBUG -O2)
endif()

find_package(Boost CONFIG REQUIRED COMPONENTS unit_test_framework)

# Adds executable
add_executable(exe)
target_compile_options(exe PRIVATE ${CXX_FLAGS})
target_sources(exe PRIVATE src/main.cpp src/logger.cpp src/logic.cpp)
target_include_directories(exe PRIVATE src)

# Adds test executable
add_executable(exe_test)
target_compile_options(exe_test PRIVATE ${CXX_FLAGS})
target_sources(exe_test PRIVATE src/main_test.cpp src/logger.cpp src/logic.cpp)
target_include_directories(exe_test PRIVATE src)
target_link_libraries(exe_test PRIVATE Boost::unit_test_framework)
