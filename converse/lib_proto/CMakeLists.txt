add_library(lib_proto STATIC)
target_sources(
  lib_proto
  PRIVATE src/converse/service/health/health.grpc.pb.cc
          src/converse/service/health/health.pb.cc
          src/converse/service/link/link.grpc.pb.cc
          src/converse/service/link/link.pb.cc
          src/converse/service/main/main.grpc.pb.cc
          src/converse/service/main/main.pb.cc)
target_include_directories(lib_proto PUBLIC src)
target_link_libraries(lib_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)
