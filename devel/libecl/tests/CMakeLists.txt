if (BUILD_TESTS)
   include( tests.cmake ) 
endif()

# The ecl_win64 application is not built as a proper test integrated
# into the CTEST system. Should be invoked manually on Windows.
if (ERT_WINDOWS) 
   add_executable( ecl_win64 ecl_win64.c ) 
   target_link_libraries( ecl_win64 ecl util ) 
endif()

