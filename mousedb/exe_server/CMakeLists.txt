cmake_minimum_required(VERSION 3.31)

project(exe_server LANGUAGES CXX)

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

add_executable(exe_server src/main.cpp)
set_target_properties(
  exe_server
  PROPERTIES CXX_EXTENSIONS ON
             CXX_STANDARD 23
             CXX_STANDARD_REQUIRED OFF
             EXPORT_COMPILE_COMMANDS ON
             OUTPUT_NAME exe)
target_include_directories(exe_server PRIVATE src)
target_link_libraries(exe_server)
