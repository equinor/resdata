# This will macro will initialize the current cmake session for
# Python. The macro starts by looking for the Python interpreter of
# correct version. Then afterwards the variables;
#
#
#  PYTHON_INSTALL_PREFIX: All python packages will be located in
#        ${GLOBAL_PREFIX}/${PYTHON_INSTALL_PREFIX} - this applies both
#        when searching for dependencies and when installing.
#
# CTEST_PYTHONPATH: Normal ':' separated path variables which is
#        passed to the test runner. Should contain the PYTHONPATH to
#        all third party packages which are not in the default search
#        path. The CTEST_PYTHONPATH variable will be updated by the
#        python_package( ) function when searching for third party
#        packages.
#
# will be set.

macro(init_python target_version)

   FIND_PACKAGE(PythonInterp)
   if (NOT DEFINED PYTHON_EXECUTABLE)
      message(WARNING "Python interpreter not found - Python wrappers not enabled")
      set( BUILD_PYTHON OFF PARENT_SCOPE )
      return()
   endif()

   if (NOT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" STREQUAL "${target_version}")
      message(WARNING "Need Python version ${target_version}, found version: ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} - Python wrappers not enabled")
      set( BUILD_PYTHON OFF PARENT_SCOPE )
      return()
   endif()

   if (EXISTS "/etc/debian_version")
      set( PYTHON_PACKAGE_PATH "dist-packages")
   else()
      set( PYTHON_PACKAGE_PATH "site-packages")
   endif()
   set(PYTHON_INSTALL_PREFIX  "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/${PYTHON_PACKAGE_PATH}"  CACHE STRING "Subdirectory to install Python modules in")
   set(CTEST_PYTHONPATH ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX})

endmacro()