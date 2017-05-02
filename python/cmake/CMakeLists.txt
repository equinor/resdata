set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/Modules")

include(init_python)
include(add_python_test)
include(find_python_package) 
include(add_python_package)

install( DIRECTORY Modules DESTINATION share/cmake )
