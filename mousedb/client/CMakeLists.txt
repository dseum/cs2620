cmake_minimum_required(VERSION 3.31)

project(exe_client LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Boost with asio and program_options
find_package(Boost CONFIG REQUIRED COMPONENTS asio program_options)
find_package(GTest CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)

enable_testing()

# Client executable
set(EXE_CLIENT_SRC src/client_main.cpp)
add_executable(exe_client ${EXE_CLIENT_SRC})
set_target_properties(exe_client PROPERTIES OUTPUT_NAME exe/client_main)
target_include_directories(exe_client PRIVATE src)
target_link_libraries(exe_client PRIVATE Boost::asio Boost::program_options quill::quill)

# Client tests
file(GLOB_RECURSE EXE_CLIENT_TEST_SRC CONFIGURE_DEPENDS *.test.cpp)
if(EXE_CLIENT_TEST_SRC)
  add_executable(exe_client_test ${EXE_CLIENT_TEST_SRC})
  set_target_properties(exe_client_test PROPERTIES OUTPUT_NAME exe/test)
  target_include_directories(exe_client_test PRIVATE src)
  target_link_libraries(exe_client_test PRIVATE GTest::gtest_main exe_client)

  include(GoogleTest)
  gtest_discover_tests(exe_client_test)
endif()
