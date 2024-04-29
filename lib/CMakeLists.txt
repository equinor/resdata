if(ERT_HAVE_GETUID AND ERT_HAVE_OPENDIR)
  list(APPEND opt_srcs util/test_work_area.cpp util/util_getuid.cpp)
endif()

if(ERT_HAVE_OPENDIR)
  list(APPEND opt_srcs util/util_opendir.cpp)
endif()

if(ERT_HAVE_SPAWN)
  list(APPEND opt_srcs util/util_spawn.cpp)
endif()

if(ERT_HAVE_LOCKF)
  list(APPEND opt_srcs util/util_lockf.cpp)
endif()

if(ERT_HAVE_UNISTD)
  list(APPEND opt_srcs util/path_stack.cpp)
endif()

if(MSVC)
  configure_file(include/ert/util/msvc_stdbool.h ert/util/stdbool.h)
endif()

foreach(type int double long time_t float)
  set(TYPE ${type})
  set(SIGNED_TYPE true)
  configure_file(vector_template.h.in include/ert/util/${type}_vector.h)
  configure_file(vector_template.hpp.in include/ert/util/${type}_vector.hpp)
  configure_file(util/vector_template.cpp.in ${type}_vector.cpp)
  list(APPEND opt_srcs ${CMAKE_CURRENT_BINARY_DIR}/${type}_vector.cpp)
endforeach()

foreach(type bool size_t)
  set(TYPE ${type})
  set(SIGNED_TYPE false)
  configure_file(vector_template.h.in include/ert/util/${type}_vector.h)
  configure_file(vector_template.hpp.in include/ert/util/${type}_vector.hpp)
  configure_file(util/vector_template.cpp.in ${type}_vector.cpp)
  list(APPEND opt_srcs ${CMAKE_CURRENT_BINARY_DIR}/${type}_vector.cpp)
endforeach()

if(ZLIB_FOUND)
  list(APPEND opt_srcs util/util_zlib.cpp)
endif()

configure_file(build_config.h.in include/ert/util/build_config.h)
configure_file(ert_api_config.h.in include/ert/util/ert_api_config.h)
configure_file(build_config.hpp.in include/ert/util/build_config.hpp)
configure_file(ert_api_config.hpp.in include/ert/util/ert_api_config.hpp)

add_library(
  resdata
  util/rng.cpp
  util/lookup_table.cpp
  util/statistics.cpp
  util/mzran.cpp
  util/hash_node.cpp
  util/hash_sll.cpp
  util/path.cpp
  util/hash.cpp
  util/node_data.cpp
  util/node_ctype.cpp
  util/util.cpp
  util/util_abort.cpp
  util/util_symlink.cpp
  util/util_lfs.c
  util/util_unlink.cpp
  util/vector.cpp
  util/parser.cpp
  util/stringlist.cpp
  util/buffer.cpp
  util/string_util.cpp
  util/type_vector_functions.cpp
  util/rd_version.cpp
  util/perm_vector.cpp
  util/test_util.cpp
  util/cxx_string_util.cpp
  ${opt_srcs}
  resdata/rd_rsthead.cpp
  resdata/rd_sum_tstep.cpp
  resdata/rd_rst_file.cpp
  resdata/rd_init_file.cpp
  resdata/rd_grid_cache.cpp
  resdata/smspec_node.cpp
  resdata/rd_kw_grdecl.cpp
  resdata/rd_file_kw.cpp
  resdata/rd_file_view.cpp
  resdata/rd_grav.cpp
  resdata/rd_smspec.cpp
  resdata/rd_unsmry_loader.cpp
  resdata/rd_sum_data.cpp
  resdata/rd_sum_file_data.cpp
  resdata/rd_util.cpp
  resdata/rd_kw.cpp
  resdata/rd_sum.cpp
  resdata/rd_sum_vector.cpp
  resdata/fortio.c
  resdata/rd_rft_file.cpp
  resdata/rd_rft_node.cpp
  resdata/rd_rft_cell.cpp
  resdata/rd_grid.cpp
  resdata/rd_coarse_cell.cpp
  resdata/rd_box.cpp
  resdata/rd_io_config.cpp
  resdata/rd_file.cpp
  resdata/rd_region.cpp
  resdata/rd_subsidence.cpp
  resdata/rd_grid_dims.cpp
  resdata/grid_dims.cpp
  resdata/nnc_info.cpp
  resdata/rd_grav_common.cpp
  resdata/nnc_vector.cpp
  resdata/rd_nnc_export.cpp
  resdata/rd_nnc_data.cpp
  resdata/rd_nnc_geometry.cpp
  resdata/layer.cpp
  resdata/fault_block.cpp
  resdata/fault_block_layer.cpp
  resdata/rd_type.cpp
  resdata/rd_type_python.cpp
  resdata/well_state.cpp
  resdata/well_conn.cpp
  resdata/well_info.cpp
  resdata/well_ts.cpp
  resdata/well_conn_collection.cpp
  resdata/well_segment.cpp
  resdata/well_segment_collection.cpp
  resdata/well_branch_collection.cpp
  resdata/well_rseg_loader.cpp
  resdata/FortIO.cpp
  resdata/RDFilename.cpp
  geometry/geo_surface.cpp
  geometry/geo_util.cpp
  geometry/geo_pointset.cpp
  geometry/geo_region.cpp
  geometry/geo_polygon.cpp
  geometry/geo_polygon_collection.cpp)

if(ERT_WINDOWS)
  set_target_properties(resdata PROPERTIES PREFIX "lib")
  if(MSVC)
    set_target_properties(resdata PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()
endif()

target_link_libraries(
  resdata
  PUBLIC ${m} ${dl} ${pthread} ${blas} ${zlib} ${shlwapi}
  PRIVATE fmt::fmt)

target_include_directories(
  resdata
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include> ${Backward_INCLUDES}
  PRIVATE ${ZLIB_INCLUDE_DIRS} util include
          ${CMAKE_CURRENT_SOURCE_DIR}/private-include
          ${CMAKE_CURRENT_BINARY_DIR}/include)

target_compile_definitions(
  resdata
  PRIVATE -DGIT_COMMIT=${GIT_COMMIT}
          -DGIT_COMMIT_SHORT=${GIT_COMMIT_SHORT}
          -DRD_VERSION_MAJOR=${RD_VERSION_MAJOR}
          -DRD_VERSION_MINOR=${RD_VERSION_MINOR}
          -DRD_VERSION_MICRO=${RD_VERSION_MICRO}
          $<$<BOOL:${BIG_ENDIAN}>:HOST_BIG_ENDIAN>)

target_compile_options(resdata PUBLIC ${pthreadarg})

if(ERT_USE_OPENMP)
  target_compile_options(resdata PUBLIC ${OpenMP_CXX_FLAGS})
  set_property(
    TARGET resdata
    APPEND
    PROPERTY LINK_FLAGS ${OpenMP_CXX_FLAGS})
  target_link_libraries(resdata PUBLIC ${OpenMP_EXE_LINKER_FLAGS})
endif()

set_target_properties(
  resdata PROPERTIES VERSION ${RD_VERSION_MAJOR}.${RD_VERSION_MINOR}
                     SOVERSION ${RD_VERSION_MAJOR})

install(
  TARGETS resdata
  EXPORT resdata-config
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(NOT BUILD_TESTS)
  return()
endif()

add_executable(rd_test_suite tests/test_rd_grid.cpp tests/test_rd_util.cpp
                             tests/testsuite.cpp tests/test_util.cpp)
if(CMAKE_COMPILER_IS_GNUCC)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
    target_compile_options(rd_test_suite PRIVATE -DFS_EXPERIMENTAL)
  else()
    target_compile_options(rd_test_suite PRIVATE "--std=c++17")
  endif()
  target_link_libraries(rd_test_suite resdata Catch2::Catch2 stdc++fs)
else()
  target_compile_options(rd_test_suite PRIVATE "--std=c++17")
  target_link_libraries(rd_test_suite resdata Catch2::Catch2)
endif()
add_test(NAME rd_test_suite COMMAND rd_test_suite)

foreach(
  name
  ert_util_alloc_file_components
  ert_util_split_path
  ert_util_approx_equal
  ert_util_before_after
  ert_util_binary_split
  ert_util_buffer
  ert_util_clamp
  ert_util_chdir
  ert_util_filename
  ert_util_hash_test
  ert_util_parent_path
  ert_util_realpath
  ert_util_relpath_test
  ert_util_rng
  ert_util_sscan_test
  ert_util_statistics
  ert_util_strcat_test
  ert_util_stringlist_test
  ert_util_string_util
  ert_util_strstr_int_format
  ert_util_type_vector_functions
  ert_util_vector_test
  ert_util_datetime
  ert_util_normal_path
  ert_util_mkdir_p
  test_area)

  add_executable(${name} util/tests/${name}.cpp)
  target_include_directories(
    ${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
  target_link_libraries(${name} resdata)
  add_test(NAME ${name} COMMAND ${name})
endforeach()

add_executable(rd_smspec_node resdata/tests/rd_smspec_node.cpp)
target_link_libraries(rd_smspec_node resdata)
add_test(NAME rd_smspec_node COMMAND rd_smspec_node)

add_executable(ert_util_work_area util/tests/ert_util_work_area.cpp)
target_link_libraries(ert_util_work_area resdata)
add_test(
  NAME ert_util_work_area
  COMMAND ert_util_work_area data2/file1
          ${CMAKE_CURRENT_SOURCE_DIR}/util/tests/data2/file2 data2
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/util/tests)

add_executable(ert_util_cwd_test util/tests/ert_util_cwd_test.cpp)
target_link_libraries(ert_util_cwd_test resdata)
add_test(NAME ert_util_cwd_test COMMAND ert_util_cwd_test
                                        ${CMAKE_CURRENT_BINARY_DIR})

add_executable(ert_util_copy_file util/tests/ert_util_copy_file.cpp)
target_link_libraries(ert_util_copy_file resdata)
add_test(NAME ert_util_copy_file COMMAND ert_util_copy_file
                                         $<TARGET_FILE:ert_util_copy_file>)

add_executable(ert_util_file_readable util/tests/ert_util_file_readable.cpp)
target_link_libraries(ert_util_file_readable resdata)
add_test(NAME ert_util_file_readable COMMAND ert_util_file_readable)

add_executable(ert_util_path_stack_test util/tests/ert_util_path_stack_test.cpp)
target_link_libraries(ert_util_path_stack_test resdata)
add_test(NAME ert_util_path_stack_test
         COMMAND ert_util_path_stack_test ${CMAKE_CURRENT_BINARY_DIR}
                 ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ert_util_abort_gnu_tests util/tests/ert_util_abort_gnu_tests.cpp)
target_link_libraries(ert_util_abort_gnu_tests resdata)
add_test(NAME ert_util_abort_gnu_tests COMMAND ert_util_abort_gnu_tests)

add_executable(ert_util_type_vector_test
               util/tests/ert_util_type_vector_test.cpp)
target_link_libraries(ert_util_type_vector_test resdata)
add_test(NAME ert_util_type_vector_test COMMAND ert_util_type_vector_test)

if(ERT_HAVE_SPAWN)
  add_executable(ert_util_spawn util/tests/ert_util_spawn.cpp)
  target_link_libraries(ert_util_spawn resdata)
  add_test(NAME ert_util_spawn COMMAND ert_util_spawn)
endif()

foreach(
  name
  rd_fault_block_layer
  rd_path_access
  rd_grid_add_nnc
  rd_grid_copy
  rd_grid_create
  rd_grid_unit_system
  rd_grid_export
  rd_grid_init_fwrite
  rd_grid_reset_actnum
  rd_grid_ext_actnum
  rd_nnc_export_intersect
  rd_sum_restart
  rd_sum_data_intermediate_test
  rd_grid_cell_contains
  rd_unsmry_loader_test
  rd_init_file
  rd_kw_space_pad
  rd_kw_cmp_string
  rd_kw_equal
  rd_kw_fread
  rd_kw_grdecl
  rd_kw_init
  rd_nnc_geometry
  rd_nnc_info_test
  rd_nnc_vector
  rd_rft_cell
  rd_sum_alloc_resampled_test
  rd_file_view
  test_rd_file_index
  test_transactions
  rd_rst_file
  rd_sum_writer
  rd_filenames
  rd_make_date_no_shift
  rd_make_date_shift
  rd_month_range
  rd_valid_basename
  test_rd_nnc_data
  rd_nnc_pair
  well_conn_collection
  well_branch_collection
  well_conn
  well_state
  well_segment
  well_segment_conn
  well_segment_collection
  rd_file)
  add_executable(${name} resdata/tests/${name}.cpp)
  target_link_libraries(${name} resdata)
  target_include_directories(
    ${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
  add_test(NAME ${name} COMMAND ${name})
endforeach()

if(HAVE_UTIL_ABORT_INTERCEPT)
  add_executable(rd_grid_corner resdata/tests/rd_grid_corner.cpp)
  target_link_libraries(rd_grid_corner resdata)
  add_test(NAME rd_grid_corner COMMAND rd_grid_corner)

  add_executable(rd_layer resdata/tests/rd_layer.cpp)
  target_link_libraries(rd_layer resdata)
  target_include_directories(
    rd_layer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
  add_test(NAME rd_layer COMMAND rd_layer)
endif()

add_executable(rd_get_num_cpu resdata/tests/rd_get_num_cpu_test.cpp)
target_link_libraries(rd_get_num_cpu resdata)
add_test(
  NAME rd_get_num_cpu
  COMMAND
    rd_get_num_cpu ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu1
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu2
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu3
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu4
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu5)

# The rd_win64 application is not built as a proper test integrated into the
# CTEST system. Should be invoked manually on Windows.
if(ERT_WINDOWS)
  add_executable(rd_lfs resdata/tests/rd_lfs.cpp)
  target_link_libraries(rd_lfs resdata)
endif()

#
# geometry
#

foreach(name geo_util_xlines geo_polygon geo_polygon_collection)
  add_executable(${name} geometry/tests/${name}.cpp)
  target_link_libraries(${name} resdata)
  add_test(NAME ${name} COMMAND ${name})
endforeach()

foreach(test ert_util_unique_ptr)
  add_executable(${test} util/tests/${test}.cpp)
  target_link_libraries(${test} resdata)
  add_test(NAME ${test} COMMAND ${test})
endforeach()

foreach(test rdxx_kw rdxx_fortio rdxx_filename rdxx_types)
  add_executable(${test} resdata/tests/${test}.cpp)
  target_link_libraries(${test} resdata)
  add_test(NAME ${test} COMMAND ${test})
endforeach()

foreach(name rd_grid_layer_contains rd_region)

  add_executable(${name} resdata/tests/${name}.cpp)
  target_link_libraries(${name} resdata)
  target_include_directories(
    ${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
endforeach()

foreach(name rd_grid_layer_contains rd_region rd_grid_copy)
  foreach(i RANGE 1 7)
    add_test(NAME ${name}_faarikaal${i}
             COMMAND ${name}
                     ${_local_resdatapath}/faarikaal/faarikaal${i}.EGRID)
  endforeach()
endforeach()
