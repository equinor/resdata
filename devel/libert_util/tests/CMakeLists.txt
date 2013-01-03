link_directories( ${ERT_BINARY_DIR}/libert_util/src )

add_executable( type_vector_test type_vector_test.c )
target_link_libraries( type_vector_test ert_util )
add_test( type_vector_test ${EXECUTABLE_OUTPUT_PATH}/type_vector_test )

add_executable( vector_test vector_test.c )
target_link_libraries( vector_test ert_util )
add_test( vector_test ${EXECUTABLE_OUTPUT_PATH}/vector_test )

add_executable( ert_util_stringlist_test ert_util_stringlist_test.c )
target_link_libraries( ert_util_stringlist_test ert_util )
add_test( ert_util_stringlist_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_stringlist_test )

if (WITH_LATEX)
   add_executable( latex_test latex_test.c )
   target_link_libraries( latex_test ert_util )
   add_test( latex_test ${EXECUTABLE_OUTPUT_PATH}/latex_test ${CMAKE_CURRENT_SOURCE_DIR}/data/latex_OK.tex )
endif()
