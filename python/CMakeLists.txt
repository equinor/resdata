include(cmake/init_python.cmake)
include(cmake/python_package_versions.cmake) 
include(cmake/python.cmake)


init_python( 2.7 )

python_package(numpy 1.7.1 ${PYTHON_INSTALL_PREFIX})
if (NOT DEFINED PY_numpy)
   message(WARNING "numpy module not found - Python wrappers not enabled")
   set( BUILD_PYTHON OFF PARENT_SCOPE )
   return()
endif()



# add_python_test is a macro which uses the variable
# PYTHON_INSTALL_PREFIX, the macro defintion must therefor come
# *after* the PYTHON_INSTALL_PREFIX variable has been defined.
include(cmake/add_python_test.cmake)

if (BUILD_TESTS)
   add_subdirectory( tests )
endif()
add_subdirectory( python )
