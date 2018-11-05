pybind11_add_module(ecl_pb11 bind_ecl.cpp)
target_link_libraries(ecl_pb11 PRIVATE ecl)
install(TARGETS ecl_pb11 DESTINATION ${PYTHON_INSTALL_PREFIX})
