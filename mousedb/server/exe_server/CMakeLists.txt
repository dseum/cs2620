cmake_minimum_required(VERSION 3.31)

project(exe_server LANGUAGES CXX)

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

find_package(mousedb REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS asio)
find_package(GTest CONFIG REQUIRED)

enable_testing()

set(EXE_SERVER_SRC src/main.cpp src/server.cpp)
add_executable(exe_server ${EXE_SERVER_SRC})
set_target_properties(exe_server PROPERTIES OUTPUT_NAME exe/main)
target_include_directories(exe_server PRIVATE src)
target_link_libraries(exe_server PRIVATE mousedb::database Boost::asio)

file(GLOB_RECURSE EXE_SERVER_TEST_SRC CONFIGURE_DEPENDS *.test.cpp)
if(EXE_SERVER_TEST_SRC)
  add_executable(exe_server_test ${EXE_SERVER_TEST_SRC})
  set_target_properties(exe_server_test PROPERTIES OUTPUT_NAME exe/test)
  target_include_directories(exe_server_test PRIVATE src)
  target_link_libraries(exe_server_test PRIVATE GTest::gtest_main exe_server)

  include(GoogleTest)
  gtest_discover_tests(exe_server_test)
endif()
