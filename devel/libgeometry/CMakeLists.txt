set( GEOMETRY_INSTALL_PREFIX "" CACHE STRING "Prefix for installation of libgeometry")
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src )
add_subdirectory( src )

