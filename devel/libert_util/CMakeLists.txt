add_subdirectory( include/ert/util )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/include/ert/util/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
add_subdirectory( src )
add_subdirectory( applications )
add_subdirectory( tests )
