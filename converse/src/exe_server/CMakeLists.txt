add_executable(exe_server)
set_target_properties(exe_server PROPERTIES OUTPUT_NAME exe)
target_sources(exe_server PRIVATE src/main.cpp src/database.cpp src/server.cpp)
target_include_directories(exe_server PRIVATE src)
target_link_libraries(
  exe_server
  PRIVATE Boost::algorithm Boost::program_options lib_logging lib_proto
          unofficial::argon2::libargon2 unofficial::sqlite3::sqlite3)
