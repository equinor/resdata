configure_file(test_env.py.in   ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}/test_env.py )

if (NOT PY_cwrap)
    add_subdirectory(cwrap)
endif ()

add_subdirectory( ecl )

if (INSTALL_ERT_LEGACY)
   add_subdirectory( legacy/ert )
endif()
