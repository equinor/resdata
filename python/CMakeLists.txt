add_subdirectory( cmake )

init_python( 2.7 )

python_package(numpy 1.7.1 ${PYTHON_INSTALL_PREFIX})
if (NOT DEFINED PY_numpy)
   message(WARNING "numpy module not found - Python wrappers not enabled")
   set( BUILD_PYTHON OFF PARENT_SCOPE )
   return()
endif()



#include( cmake/Modules/add_python_test.cmake )
if (BUILD_TESTS)
   add_subdirectory( tests )
endif()
add_subdirectory( python )


