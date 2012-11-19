link_directories( ${ERT_BINARY_DIR}/libert_util/src )

add_executable( type_vector_test type_vector_test.c )
target_link_libraries( type_vector_test ert_util )

add_executable( vector_test vector_test.c )
target_link_libraries( vector_test ert_util )



add_test( type_vector_test ${EXECUTABLE_OUTPUT_PATH}/type_vector_test )
add_test( vector_test      ${EXECUTABLE_OUTPUT_PATH}/vector_test )
