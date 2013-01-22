link_directories( ${ERT_BINARY_DIR}/libert_util/src )

add_executable( ert_util_type_vector_test ert_util_type_vector_test.c )
target_link_libraries( ert_util_type_vector_test ert_util )
add_test( ert_util_type_vector_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_type_vector_test )

add_executable( ert_util_vector_test ert_util_vector_test.c )
target_link_libraries( ert_util_vector_test ert_util )
add_test( ert_util_vector_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_vector_test )

add_executable( ert_util_cwd_test ert_util_cwd_test.c )
target_link_libraries( ert_util_cwd_test ert_util )
add_test( ert_util_cwd_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_cwd_test ${CMAKE_CURRENT_BINARY_DIR})

add_executable( ert_util_relpath_test ert_util_relpath_test.c )
target_link_libraries( ert_util_relpath_test ert_util )
add_test( ert_util_relpath_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_relpath_test )

add_executable( ert_util_path_stack_test ert_util_path_stack_test.c )
target_link_libraries( ert_util_path_stack_test ert_util )
add_test( ert_util_path_stack_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_path_stack_test ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_executable( ert_util_PATH_test ert_util_PATH_test.c )
target_link_libraries( ert_util_PATH_test ert_util )
add_test( ert_util_PATH_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_PATH_test )

add_executable( ert_util_strcat_test ert_util_strcat_test.c )
target_link_libraries( ert_util_strcat_test ert_util )
add_test( ert_util_strcat_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_strcat_test )

add_executable( ert_util_stringlist_test ert_util_stringlist_test.c )
target_link_libraries( ert_util_stringlist_test ert_util )
add_test( ert_util_stringlist_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_stringlist_test )

add_executable( ert_util_realpath ert_util_realpath.c )
target_link_libraries( ert_util_realpath ert_util )
add_test( ert_util_realpath ${EXECUTABLE_OUTPUT_PATH}/ert_util_realpath )

add_executable( ert_util_hash_test ert_util_hash_test.c )
target_link_libraries( ert_util_hash_test ert_util )
add_test( ert_util_hash_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_hash_test )

add_executable( ert_util_binary_split ert_util_binary_split.c )
target_link_libraries( ert_util_binary_split ert_util )
add_test( ert_util_binary_split ${EXECUTABLE_OUTPUT_PATH}/ert_util_binary_split )

if (WITH_LATEX)
   add_executable( ert_util_latex_test ert_util_latex_test.c )
   target_link_libraries(ert_util_latex_test ert_util )
   add_test( ert_util_latex_test ${EXECUTABLE_OUTPUT_PATH}/ert_util_latex_test ${CMAKE_CURRENT_SOURCE_DIR}/data/latex_OK.tex )
endif()
