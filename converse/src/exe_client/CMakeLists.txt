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
