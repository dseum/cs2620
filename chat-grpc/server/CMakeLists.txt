cmake_minimum_required(VERSION 3.10)

project(chat)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Flags
set(CXX_FLAGS -Wall)
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(CXX_FLAGS ${CXX_FLAGS} -fexperimental-library)
endif()
if(CMAKE_BUILD_TYPE STREQUAL Release)
  set(CXX_FLAGS ${CXX_FLAGS} -DNDEBUG -O3 -march=native)
else()
  set(CXX_FLAGS ${CXX_FLAGS} -g -O0)
endif()

# Sources
set(SRCS
    src/database.cpp src/operations.cpp src/server.cpp
    src/gen/rpc/converse/converse.pb.cc
    src/gen/rpc/converse/converse.grpc.pb.cc)

set(DIRS src src/gen/rpc/converse)

# Find required packages
find_package(Boost CONFIG REQUIRED COMPONENTS algorithm json program_options
                                              unit_test_framework)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Libraries to link
set(LIBS Boost::algorithm Boost::json Boost::program_options
         unofficial::argon2::libargon2 unofficial::sqlite3::sqlite3)

# Original executable (using main.cpp)
set(EXE exe)
add_executable(${EXE} src/main.cpp ${SRCS})
target_include_directories(${EXE} PRIVATE ${DIRS})
target_compile_options(${EXE} PRIVATE ${CXX_FLAGS})
target_link_libraries(${EXE} PRIVATE ${LIBS})

# Test executable (using main_test.cpp)
set(EXE_TEST ${EXE}_test)
add_executable(${EXE_TEST} src/main_test.cpp ${SRCS})
target_include_directories(${EXE_TEST} PRIVATE ${DIRS})
target_compile_options(${EXE_TEST} PRIVATE ${CXX_FLAGS})
target_link_libraries(${EXE_TEST} PRIVATE ${LIBS} Boost::unit_test_framework)
