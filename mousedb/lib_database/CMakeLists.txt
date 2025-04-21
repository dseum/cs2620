cmake_minimum_required(VERSION 3.31)

project(lib_database LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(
    FATAL_ERROR "Linux is required. ${CMAKE_SYSTEM_NAME} is not supported.")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "Clang is required.")
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 20.1)
  message(FATAL_ERROR "Clang 20.1 or higher is required.")
endif()

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost CONFIG REQUIRED COMPONENTS dynamic_bitset)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)

enable_testing()

set(LIB_DATABASE_SRC src/arena.cpp src/memtable.cpp)
add_library(lib_database ${LIB_DATABASE_SRC})
target_include_directories(lib_database PRIVATE src)

file(GLOB_RECURSE LIB_DATABASE_TEST_SRC CONFIGURE_DEPENDS *.test.cpp)
if(LIB_DATABASE_TEST_SRC)
  add_executable(lib_database_test ${LIB_DATABASE_TEST_SRC})
  set_target_properties(lib_database_test PROPERTIES OUTPUT_NAME exe/test)
  target_include_directories(lib_database_test PRIVATE src)
  target_link_libraries(lib_database_test PRIVATE GTest::gtest_main
                                                  lib_database)

  include(GoogleTest)
  gtest_discover_tests(lib_database_test)
endif()

file(GLOB_RECURSE LIB_DATABASE_BENCH_SRC CONFIGURE_DEPENDS *.bench.cpp)
if(LIB_DATABASE_BENCH_SRC)
  add_executable(lib_database_bench ${LIB_DATABASE_BENCH_SRC})
  set_target_properties(lib_database_bench PROPERTIES OUTPUT_NAME exe/bench)
  target_include_directories(lib_database_bench PRIVATE src)
  target_link_libraries(lib_database_bench PRIVATE benchmark::benchmark_main
                                                   lib_database)
endif()
