execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c
          "import pybind11; print(pybind11.get_cmake_dir())"
  OUTPUT_VARIABLE _tmp_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ECHO STDOUT)
list(APPEND CMAKE_PREFIX_PATH "${_tmp_dir}")

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_native MODULE init.cpp summary.cpp)
target_link_libraries(_native PRIVATE ecl-static)
target_include_directories(
  _native
  PRIVATE "${CMAKE_SOURCE_DIR}/lib/include"
          "${CMAKE_SOURCE_DIR}/lib/private-include"
          "${CMAKE_BINARY_DIR}/lib/include")
target_compile_definitions(_native PRIVATE VERSION_INFO=${PROJECT_VERSION})
