cmake_minimum_required(VERSION 3.30)

project(converse_client)

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
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_compile_options(-g -O0)
  set(CMAKE_BUILD_TYPE Debug)
elseif(CMAKE_BUILD_TYPE STREQUAL Release)
  add_compile_options(-DNDEBUG -O3)
else()
  message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif()
# add_compile_options(-Wall -Wconversion -Wextra -Wpedantic -Wshadow -Wunused)

# Finds packages
find_package(Boost CONFIG REQUIRED COMPONENTS date_time log log_setup)
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# Adds subdirectories
add_subdirectory(../lib_logging ${CMAKE_CURRENT_BINARY_DIR}/lib_logging)
add_subdirectory(../lib_proto ${CMAKE_CURRENT_BINARY_DIR}/lib_proto)

qt_standard_project_setup(REQUIRES 6.8.2)
qt_add_executable(exe_client)
qt_add_qml_module(
  exe_client
  URI
  Converse
  VERSION
  1.0
  QML_FILES
  src/main.qml
  SOURCES
  src/backend.hpp
  src/backend.cpp)
set_target_properties(exe_client PROPERTIES OUTPUT_NAME exe)
target_sources(exe_client PRIVATE src/backend.cpp src/main.cpp src/reader.cpp)
target_include_directories(exe_client PRIVATE src)
target_link_libraries(exe_client PRIVATE lib_logging lib_proto Qt6::Gui
                                         Qt6::Qml)
