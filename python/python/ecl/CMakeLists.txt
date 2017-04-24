set(PYTHON_SOURCES
    __init__.py
)
add_python_package("python.ecl"  ${PYTHON_INSTALL_PREFIX}/ecl "${PYTHON_SOURCES}" True)

add_subdirectory(ecl)
add_subdirectory(geo)
add_subdirectory(test)
add_subdirectory(util)
add_subdirectory(well)

configure_file(ert_lib_info_build.py.in   ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}/ecl/__ert_lib_info.py )
configure_file(ert_lib_info_install.py.in ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}/ert_lib_info_install.py )
install(FILES ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}/ert_lib_info_install.py DESTINATION ${PYTHON_INSTALL_PREFIX}/ecl RENAME __ert_lib_info.py)
