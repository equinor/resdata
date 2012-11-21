add_executable( coarse_test coarse_test.c )
target_link_libraries( coarse_test ecl )


add_test( coarse_test ${EXECUTABLE_OUTPUT_PATH}/coarse_test ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/LGCcase/LGC_TESTCASE2.EGRID )
set_property( TEST coarse_test PROPERTY LABELS Statoil )
