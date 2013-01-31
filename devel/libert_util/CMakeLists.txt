add_subdirectory( include/ert/util )

add_subdirectory( src )
if (BUILD_APPLICATONS)
   add_subdirectory( applications )
endif()

if (BUILD_TESTS)
   add_subdirectory( tests )
endif()

if (MSVC)
  include_directories( ${CMAKE_CURRENT_BINARY_DIR}/include/ert/util )
endif()
