add_library(lib_proto STATIC)
target_compile_options(lib_proto PRIVATE ${CXX_FLAGS})
target_sources(
  lib_proto
  PRIVATE src/converse/service/main/main.grpc.pb.cc
          src/converse/service/main/main.pb.cc
          src/converse/service/link/link.grpc.pb.cc
          src/converse/service/link/link.pb.cc)
target_include_directories(lib_proto PUBLIC src)
target_link_libraries(
  lib_proto
  PUBLIC gRPC::grpc++
  PRIVATE protobuf::libprotobuf)
