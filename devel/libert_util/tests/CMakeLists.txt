link_directories( ${ERT_BINARY_DIR}/libert_util/src )

add_executable( ert_util_type_vector_test ert_util_type_vector_test.c )
target_link_libraries( ert_util_type_vector_test ert_util )
add_test( ert_util_type_vector_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_type_vector_test )

add_executable( ert_util_vector_test ert_util_vector_test.c )
target_link_libraries( ert_util_vector_test ert_util )
add_test( ert_util_vector_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_vector_test )

add_executable( ert_util_sprintf_escape ert_util_sprintf_escape.c )
target_link_libraries( ert_util_sprintf_escape ert_util )
add_test( ert_util_sprintf_escape ${EXECUTABLE_OUTPUT_PATH}/ert_util_sprintf_escape )

add_executable( ert_util_stringlist_test ert_util_stringlist_test.c )
target_link_libraries( ert_util_stringlist_test ert_util )
add_test( ert_util_stringlist_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_stringlist_test )

add_executable( ert_util_time_interval ert_util_time_interval.c )
target_link_libraries( ert_util_time_interval ert_util )
add_test( ert_util_time_interval ${EXECUTABLE_OUTPUT_PATH}/ert_util_time_interval )

add_executable( ert_util_before_after ert_util_before_after.c )
target_link_libraries( ert_util_before_after ert_util )
add_test( ert_util_before_after ${EXECUTABLE_OUTPUT_PATH}/ert_util_before_after )


add_executable( ert_util_approx_equal ert_util_approx_equal.c )
target_link_libraries( ert_util_approx_equal ert_util )
add_test( ert_util_approx_equal ${EXECUTABLE_OUTPUT_PATH}/ert_util_approx_equal )


if (WITH_LATEX)
   add_executable( ert_util_latex_test ert_util_latex_test.c )
   target_link_libraries(ert_util_latex_test ert_util )
   add_test( ert_util_latex_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_latex_test ${CMAKE_CURRENT_SOURCE_DIR}/data/latex_OK.tex )
endif()
