project(libecl-util C)

if (HAVE_PTHREAD)
    # The block_fs filesystem is so heavily dependant on pthreads that it is
    # not built if de not have pthreads.
    list(APPEND thread_srcs src/thread_pool.c src/block_fs.c)
endif ()

if (LAPACK_FOUND)
    list(APPEND lapack_srcs src/matrix_lapack.c
                            src/matrix_blas.c
                            src/matrix_stat.c
                            src/regression.c
                            src/lars.c
                            src/stepwise.c
        )
endif ()

if (ERT_HAVE_GETUID AND ERT_HAVE_OPENDIR)
    list(APPEND test_area src/test_work_area.c)
endif()

if (ERT_HAVE_UNISTD)
   list(APPEND stack_srcs src/path_stack.c)
endif ()

if (MSVC)
    configure_file(include/ert/util/msvc_stdbool.h ert/util/stdbool.h)
endif ()

foreach (type int double bool long time_t size_t float)
    set(TYPE ${type})
    configure_file(include/ert/util/vector_template.h
                   include/ert/util/${type}_vector.h)
    configure_file(src/vector_template.c ${type}_vector.c)
    list(APPEND vector_srcs ${CMAKE_CURRENT_BINARY_DIR}/${type}_vector.c)
endforeach ()

configure_file(build_config.h.in   include/ert/util/build_config.h)
configure_file(ert_api_config.h.in include/ert/util/ert_api_config.h)

add_library(ert_util src/rng.c
                     src/lookup_table.c
                     src/statistics.c
                     src/mzran.c
                     src/set.c
                     src/hash_node.c
                     src/hash_sll.c
                     src/hash.c
                     src/node_data.c
                     src/node_ctype.c
                     src/util.c
                     src/msg.c
                     src/arg_pack.c
                     src/path_fmt.c
                     src/menu.c
                     src/subst_list.c
                     src/subst_func.c
                     src/vector.c
                     src/parser.c
                     src/stringlist.c
                     src/matrix.c
                     src/buffer.c
                     src/log.c
                     src/template.c
                     src/timer.c
                     src/time_interval.c
                     src/string_util.c
                     src/type_vector_functions.c
                     src/ui_return.c
                     src/ert_version.c
                     src/struct_vector.c
                     src/perm_vector.c
                     src/test_util.c
                     ${vector_srcs}
                     ${thread_srcs}
                     ${test_area}
                     ${lapack_srcs}
                     ${stack_srcs}
           )

target_include_directories(ert_util
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
           $<INSTALL_INTERFACE:include>
    PRIVATE ${ZLIB_INCLUDE_DIRS}
    )

target_compile_definitions(ert_util PRIVATE
            -DGIT_COMMIT=${GIT_COMMIT}
            -DGIT_COMMIT_SHORT=${GIT_COMMIT_SHORT}
            -DERT_VERSION_MAJOR=${ERT_VERSION_MAJOR}
            -DERT_VERSION_MINOR=${ERT_VERSION_MINOR}
            -DERT_VERSION_MICRO=${ERT_VERSION_MICRO}
        )

if (PING_PATH)
    target_compile_definitions(ert_util PRIVATE -DPING_CMD=${PING_PATH})
endif()

target_link_libraries(ert_util PUBLIC ${m}
                                      ${dl}
                                      ${pthread}
                                      ${blas}
                                      ${lapack}
                                      ${zlib}
                                      ${shlwapi}
                     )

target_compile_options(ert_util PUBLIC ${pthreadarg})

set_target_properties(ert_util PROPERTIES
                        VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR}
                        SOVERSION ${ERT_VERSION_MAJOR}
                     )

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
        DESTINATION include
        PATTERN *.h
)
install(TARGETS ert_util
        EXPORT  ecl-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (NOT BUILD_TESTS)
    return ()
endif()

# Add simple tests that have no input arguments or are dependent on config
foreach (name   ert_util_alloc_file_components
                ert_util_approx_equal
                ert_util_arg_pack
                ert_util_before_after
                ert_util_binary_split
                ert_util_buffer
                ert_util_clamp
                ert_util_filename
                ert_util_hash_test
                ert_util_logh
                ert_util_matrix
                ert_util_parent_path
                ert_util_PATH_test
                ert_util_realpath
                ert_util_relpath_test
                ert_util_rng
                ert_util_sprintf_escape
                ert_util_sscan_test
                ert_util_statistics
                ert_util_strcat_test
                ert_util_stringlist_test
                ert_util_string_util
                ert_util_strstr_int_format
                ert_util_subst_list
                ert_util_time_interval
                ert_util_type_vector_functions
                ert_util_ui_return
                ert_util_vector_test
        )

    add_executable(${name} tests/${name}.c)
    target_link_libraries(${name} ert_util)
    add_test(NAME ${name} COMMAND ${name})
endforeach ()

add_executable(ert_util_work_area tests/ert_util_work_area.c)
target_link_libraries(ert_util_work_area ert_util)
add_test(NAME ert_util_work_area
    COMMAND ert_util_work_area  data2/file1
                                ${CMAKE_CURRENT_SOURCE_DIR}/tests/data2/file2
                                data2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

find_library( VALGRIND NAMES valgr )
if (VALGRIND)
    set(valgrind_cmd valgrind --error-exitcode=1 --tool=memcheck)
endif ()

add_executable(test_thread_pool tests/test_thread_pool.c)
target_link_libraries(test_thread_pool ert_util)
add_test(NAME test_thread_pool COMMAND ${valgrind_cmd} test_thread_pool)

add_executable(ert_util_cwd_test tests/ert_util_cwd_test.c)
target_link_libraries(ert_util_cwd_test ert_util)
add_test(NAME ert_util_cwd_test COMMAND ert_util_cwd_test ${CMAKE_CURRENT_BINARY_DIR})

add_executable(ert_util_copy_file tests/ert_util_copy_file.c)
target_link_libraries(ert_util_copy_file ert_util)
add_test(NAME ert_util_copy_file
        COMMAND ert_util_copy_file
        $<TARGET_FILE:ert_util_copy_file>)

add_executable(ert_util_file_readable tests/ert_util_file_readable.c)
target_link_libraries(ert_util_file_readable ert_util)
add_test(NAME ert_util_file_readable COMMAND ert_util_file_readable)

add_executable(ert_util_path_stack_test tests/ert_util_path_stack_test.c)
target_link_libraries(ert_util_path_stack_test ert_util)
add_test(NAME ert_util_path_stack_test COMMAND ert_util_path_stack_test
    ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if (LAPACK_FOUND)
   add_executable(ert_util_matrix_lapack tests/ert_util_matrix_lapack.c)
   target_link_libraries(ert_util_matrix_lapack ert_util)
   add_test(NAME ert_util_matrix_lapack COMMAND ert_util_matrix_lapack)

   add_executable(ert_util_matrix_stat tests/ert_util_matrix_stat.c)
   target_link_libraries(ert_util_matrix_stat ert_util)
   add_test(NAME ert_util_matrix_stat COMMAND ert_util_matrix_stat)
endif()

if (HAVE_BACKTRACE)
   add_executable(ert_util_abort_gnu_tests tests/ert_util_abort_gnu_tests.c)
   target_link_libraries(ert_util_abort_gnu_tests ert_util)
   add_test(NAME ert_util_abort_gnu_tests COMMAND ert_util_abort_gnu_tests)

   add_executable(ert_util_addr2line tests/ert_util_addr2line.c)
   target_link_libraries(ert_util_addr2line ert_util)
   add_test(NAME ert_util_addr2line COMMAND ert_util_addr2line)
endif()

if (PING_PATH)
    add_executable(ert_util_ping tests/ert_util_ping.c)
    target_link_libraries(ert_util_ping ert_util)
    add_test(NAME ert_util_ping COMMAND ert_util_ping)
endif ()

if (HAVE_UTIL_ABORT_INTERCEPT)
   add_executable(ert_util_block_fs tests/ert_util_block_fs.c)
   target_link_libraries(ert_util_block_fs ert_util)
   add_test(NAME ert_util_block_fs COMMAND ert_util_block_fs)

   add_executable(ert_util_struct_vector tests/ert_util_struct_vector.c)
   target_link_libraries(ert_util_struct_vector ert_util)
   add_test(NAME ert_util_struct_vector COMMAND ert_util_struct_vector)

   add_executable(ert_util_type_vector_test tests/ert_util_type_vector_test.c)
   target_link_libraries(ert_util_type_vector_test ert_util)
   add_test(NAME ert_util_type_vector_test COMMAND ert_util_type_vector_test)
endif()

if (ERT_HAVE_SPAWN)
   add_executable(ert_util_spawn tests/ert_util_spawn.c)
   target_link_libraries(ert_util_spawn ert_util)
   add_test(NAME ert_util_spawn COMMAND ert_util_spawn)
endif()
