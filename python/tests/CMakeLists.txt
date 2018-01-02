set(TEST_SOURCES
    __init__.py
)

add_python_package("python.tests" "${PYTHON_INSTALL_PREFIX}/tests" "${TEST_SOURCES}" False)
set(CTEST_PYTHONPATH ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX})

add_subdirectory(geometry_tests)
add_subdirectory(util_tests)
add_subdirectory(ecl_tests)
add_subdirectory(well_tests)
add_subdirectory(global_tests)

if (INSTALL_ERT_LEGACY)
   add_subdirectory(legacy_tests)
endif()

configure_file( test_install.in ${EXECUTABLE_OUTPUT_PATH}/test_install @ONLY )
