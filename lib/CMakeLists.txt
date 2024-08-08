if(ERT_HAVE_GETUID AND ERT_HAVE_OPENDIR)
  list(APPEND opt_srcs util/test_work_area.cpp util/util_getuid.cpp)
endif()

if(ERT_HAVE_OPENDIR)
  list(APPEND opt_srcs util/util_opendir.cpp)
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
  configure_file(vector_template.hpp.in include/ert/util/${type}_vector.hpp)
  configure_file(util/vector_template.cpp.in ${type}_vector.cpp)
  list(APPEND opt_srcs ${CMAKE_CURRENT_BINARY_DIR}/${type}_vector.cpp)
endforeach()

foreach(type bool size_t)
  set(TYPE ${type})
  set(SIGNED_TYPE false)
  configure_file(vector_template.hpp.in include/ert/util/${type}_vector.hpp)
  configure_file(util/vector_template.cpp.in ${type}_vector.cpp)
  list(APPEND opt_srcs ${CMAKE_CURRENT_BINARY_DIR}/${type}_vector.cpp)
endforeach()

configure_file(build_config.hpp.in include/ert/util/build_config.hpp)
configure_file(ert_api_config.hpp.in include/ert/util/ert_api_config.hpp)

add_library(
  resdata
  util/rng.cpp
  util/lookup_table.cpp
  util/mzran.cpp
  util/hash_node.cpp
  util/hash_sll.cpp
  util/path.cpp
  util/hash.cpp
  util/node_data.cpp
  util/util.cpp
  util/util_abort.cpp
  util/util_symlink.cpp
  util/util_lfs.c
  util/util_unlink.cpp
  util/vector.cpp
  util/parser.cpp
  util/stringlist.cpp
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
  resdata/rd_file.cpp
  resdata/rd_region.cpp
  resdata/rd_subsidence.cpp
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
  ert_util_binary_split
  ert_util_filename
  ert_util_parent_path
  ert_util_realpath
  ert_util_relpath_test
  ert_util_rng
  ert_util_strcat_test
  ert_util_stringlist_test
  ert_util_string_util
  ert_util_type_vector_functions
  ert_util_vector_test
  ert_util_normal_path
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

foreach(
  name
  rd_coarse_test
  rd_grid_layer_contains
  rd_restart_test
  rd_nnc_export
  rd_nnc_export_get_tran
  rd_nnc_data_equinor_root
  rd_sum_case_exists
  rd_grid_lgr_name
  rd_region
  rd_grid_cell_contains_wellpath
  rd_region2region
  rd_grid_volume
  rd_nnc_test
  rd_lgr_test
  rd_layer_equinor
  rd_dualp
  rd_grid_dx_dy_dz
  rd_sum_test
  rd_sum_report_step_equal
  rd_sum_report_step_compatible
  rd_file_equinor
  rd_fmt
  rd_rsthead
  rd_smspec
  rd_rft
  rd_fault_block_layer_equinor
  well_state_load
  well_state_load_missing_RSEG
  well_segment_load
  well_segment_branch_conn_load
  well_info
  well_conn_CF
  well_conn_load
  well_ts
  well_dualp
  well_lgr_load)

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

if(NOT EQUINOR_TESTDATA_ROOT)
  return()
endif()

add_test(NAME rd_grid_dx_dy_dz_gurbat COMMAND rd_grid_dx_dy_dz
                                              ${_resdatapath}/Gurbat/ECLIPSE)
add_test(NAME rd_grid_dx_dy_dz_troll
         COMMAND rd_grid_dx_dy_dz ${_resdatapath}/Troll/Ref2014/T07-4A-W2014-06)

add_test(NAME rd_coarse_test COMMAND rd_coarse_test
                                     ${_resdatapath}/LGCcase/LGC_TESTCASE2)

add_test(NAME rd_restart_test COMMAND rd_restart_test
                                      ${_resdatapath}/Gurbat/ECLIPSE.UNRST)

add_test(NAME rd_nnc_export_gurbat COMMAND rd_nnc_export
                                           ${_resdatapath}/Gurbat/ECLIPSE TRUE)
add_test(NAME rd_nnc_export_10kcase
         COMMAND rd_nnc_export ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC TRUE)
add_test(NAME rd_nnc_export_dual_poro_diff
         COMMAND rd_nnc_export ${_resdatapath}/DualPoro/DUAL_DIFF TRUE)
add_test(NAME rd_nnc_export_dual_poro
         COMMAND rd_nnc_export ${_resdatapath}/DualPoro/DUALPORO TRUE)
add_test(NAME rd_nnc_export_nested_lgr_case
         COMMAND rd_nnc_export ${_resdatapath}/nestedLGRcase/TESTCASE_NESTEDLGR
                 TRUE)
add_test(NAME rd_nnc_export_tyrihans
         COMMAND rd_nnc_export ${_resdatapath}/TYRIHANS/BASE20150218_MULTFLT
                 FALSE)

add_test(
  NAME rd_nnc_export_get_tran
  COMMAND rd_nnc_export_get_tran
          ${_resdatapath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3)

add_test(
  NAME rd_nnc_data_equinor_root
  COMMAND
    rd_nnc_data_equinor_root
    ${_resdatapath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3
    ${_resdatapath}/flow-nnc/Simple4/SIMPLE_SUMMARY4)

add_test(
  NAME rd_sum_case_exists
  COMMAND
    rd_sum_case_exists ${_resdatapath}/Gurbat/ECLIPSE
    ${_resdatapath}/GurbatSummary/missingHeader/ECLIPSE
    ${_resdatapath}/GurbatSummary/missingData/ECLIPSE)

add_test(
  NAME rd_grid_lgr_name
  COMMAND
    rd_grid_lgr_name
    ${_resdatapath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3.EGRID)

add_test(
  NAME rd_grid_cell_contains_wellpath1
  COMMAND
    rd_grid_cell_contains_wellpath
    ${_resdatapath}/CellContains/model/SMS-0.EGRID
    ${_resdatapath}/CellContains/R_PB-4H.jira)

add_test(NAME rd_region2region COMMAND rd_region2region
                                       ${_resdatapath}/R2R/R2R.SMSPEC)

add_test(NAME rd_lgr_test1
         COMMAND rd_lgr_test ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID)
add_test(NAME rd_lgr_test2
         COMMAND rd_lgr_test ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.GRID)
add_test(
  NAME rd_lgr_test3
  COMMAND
    rd_lgr_test
    ${_resdatapath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3.EGRID)

add_test(NAME rd_grid_volume1 COMMAND rd_grid_volume
                                      ${_resdatapath}/Gurbat/ECLIPSE)
add_test(NAME rd_grid_volume2 COMMAND rd_grid_volume
                                      ${_resdatapath}/VolumeTest/TEST1)
add_test(NAME rd_grid_volume3 COMMAND rd_grid_volume
                                      ${_resdatapath}/OsebergSyd/Omega/OMEGA-0)
add_test(
  NAME rd_grid_volume4
  COMMAND rd_grid_volume
          ${_resdatapath}/Norne/reservoir_models/Norne_ATW2013/NORNE_ATW2013)

# The grid volume test fails miserably on the test case given as example five;
# looking at the failures one could actually suspect that the ECLIPSE algorithm
# for PORV calculations has been different in this file - i.e. that the absolute
# value of the individual tetrahedron parts have been taken during the sum, and
# not at the end. At least the ert algorithm gets volumes ~ 0 whereas ECLIPSE
# reports ~10^9 for the same cell. add_test( rd_grid_volume5
# ${EXECUTABLE_OUTPUT_PATH}/rd_grid_volume
# ${_resdatapath}/Heidrun/Summary/FF12_2013B3_CLEAN_RS)

add_test(NAME rd_nnc_test1 COMMAND rd_nnc_test
                                   ${_resdatapath}/Gurbat/ECLIPSE.EGRID)
add_test(NAME rd_nnc_test2
         COMMAND rd_nnc_test ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID)
add_test(
  NAME rd_nnc_test3
  COMMAND
    rd_nnc_test
    ${_resdatapath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3.EGRID)
add_test(NAME rd_nnc_test4 COMMAND rd_nnc_test
                                   ${_resdatapath}/DualPoro/DUAL_DIFF.EGRID)
add_test(NAME rd_nnc_test5
         COMMAND rd_nnc_test
                 ${_resdatapath}/nestedLGRcase/TESTCASE_NESTEDLGR.EGRID)

add_test(NAME rd_layer_equinor
         COMMAND rd_layer_equinor ${_resdatapath}/Mariner/MARINER.EGRID
                 ${_resdatapath}/Mariner/faultblock.grdecl)

add_test(NAME rd_dualp COMMAND rd_dualp ${_resdatapath}/LGCcase/LGC_TESTCASE2)

add_test(NAME rd_sum_test COMMAND rd_sum_test ${_resdatapath}/Gurbat/ECLIPSE)

add_test(NAME rd_sum_report_step_equal1
         COMMAND rd_sum_report_step_equal ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/Snorre/SNORRE FALSE)
add_test(NAME rd_sum_report_step_equal2
         COMMAND rd_sum_report_step_equal ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/Gurbat/ECLIPSE TRUE)
add_test(NAME rd_sum_report_step_equal3
         COMMAND rd_sum_report_step_equal ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/modGurbat/extraMinistep/ECLIPSE TRUE)
add_test(NAME rd_sum_report_step_equal4
         COMMAND rd_sum_report_step_equal ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/modGurbat/short/ECLIPSE FALSE)
add_test(NAME rd_sum_report_step_equal5
         COMMAND rd_sum_report_step_equal ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/modGurbat/enkf/ECLIPSE FALSE)
add_test(NAME rd_sum_report_step_equal6
         COMMAND rd_sum_report_step_equal ${_resdatapath}/Snorre/SNORRE
                 ${_resdatapath}/Snorre2/SNORRE2 FALSE)

add_test(NAME rd_sum_report_step_compatible1
         COMMAND rd_sum_report_step_compatible ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/Snorre/SNORRE FALSE)
add_test(NAME rd_sum_report_step_compatible2
         COMMAND rd_sum_report_step_compatible ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/Gurbat/ECLIPSE TRUE)
add_test(NAME rd_sum_report_step_compatible3
         COMMAND rd_sum_report_step_compatible ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/modGurbat/extraMinistep/ECLIPSE TRUE)
add_test(NAME rd_sum_report_step_compatible4
         COMMAND rd_sum_report_step_compatible ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/modGurbat/short/ECLIPSE TRUE)
add_test(NAME rd_sum_report_step_compatible5
         COMMAND rd_sum_report_step_compatible ${_resdatapath}/Gurbat/ECLIPSE
                 ${_resdatapath}/modGurbat/enkf/ECLIPSE TRUE)
add_test(NAME rd_sum_report_step_compatible6
         COMMAND rd_sum_report_step_equal ${_resdatapath}/Snorre/SNORRE
                 ${_resdatapath}/Snorre2/SNORRE2 FALSE)

add_test(NAME rd_file_equinor
         COMMAND rd_file_equinor ${_resdatapath}/Gurbat/ECLIPSE.UNRST
                 ECLIPSE.UNRST)

add_test(NAME rd_rsthead
         COMMAND rd_rsthead ${_resdatapath}/Gurbat/ECLIPSE.UNRST
                 ${_resdatapath}/DualPoro/DUALPORO.X0005)

add_test(NAME rd_smspec
         COMMAND rd_smspec ${_resdatapath}/Gurbat/ECLIPSE.SMSPEC
                 ${_resdatapath}/Heidrun/Summary/FF12_2013B3_CLEAN_RS.SMSPEC)

add_test(NAME rd_rft_rft COMMAND rd_rft ${_resdatapath}/Gurbat/ECLIPSE.RFT RFT)
add_test(NAME rd_rft_rft_rw COMMAND rd_rft ${_resdatapath}/Gurbat/ECLIPSE.RFT
                                    RFT_RW)
add_test(NAME rd_rft_plt COMMAND rd_rft ${_resdatapath}/RFT/TEST1_1A.RFT PLT)
add_test(NAME rd_rft_mswplt COMMAND rd_rft ${_resdatapath}/RFT/RFT2.RFT MSW-PLT)
add_test(NAME rd_rft_alloc
         COMMAND rd_rft ${_resdatapath}/RFT/NORNE_ATW2013_RFTPLT_V2.RFT SIMPLE)

add_test(
  NAME rd_fault_block_layer_equinor
  COMMAND rd_fault_block_layer_equinor ${_resdatapath}/Mariner/MARINER.EGRID
          ${_resdatapath}/Mariner/faultblock.grdecl)

if(HAVE_UTIL_ABORT_INTERCEPT)
  add_executable(rd_fortio resdata/tests/rd_fortio.cpp)
  target_link_libraries(rd_fortio resdata)
  add_test(NAME rd_fortio COMMAND rd_fortio
                                  ${_resdatapath}/Gurbat/ECLIPSE.UNRST)
endif()

add_test(NAME well_state_load1
         COMMAND well_state_load ${_resdatapath}/Gurbat/ECLIPSE.EGRID
                 ${_resdatapath}/Gurbat/ECLIPSE.X0030)
add_test(NAME well_state_load2
         COMMAND well_state_load ${_resdatapath}/MSWcase/MSW_CASE.EGRID
                 ${_resdatapath}/MSWcase/MSW_CASE.X0021)
add_test(NAME well_state_load3
         COMMAND well_state_load ${_resdatapath}/Troll/MSW/MSW.EGRID
                 ${_resdatapath}/Troll/MSW/MSW.X0123)
add_test(NAME well_state_load4
         COMMAND well_state_load ${_resdatapath}/Troll/MSW_LGR/LGR.EGRID
                 ${_resdatapath}/Troll/MSW_LGR/LGR.X0095)
add_test(
  NAME well_state_load5
  COMMAND well_state_load ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID
          ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.X0061)
add_test(
  NAME well_state_load_missing_RSEG1
  COMMAND
    well_state_load_missing_RSEG
    ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID
    ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.X0061)
add_test(
  NAME well_state_load_missing_RSEG2
  COMMAND well_state_load_missing_RSEG ${_resdatapath}/Troll/MSW/MSW.EGRID
          ${_resdatapath}/Troll/MSW/MSW.X0123)

add_test(NAME well_segment_load COMMAND well_segment_load
                                        ${_resdatapath}/MSWcase/MSW_CASE.X0021)

add_test(NAME well_segment_branch_conn_load
         COMMAND well_segment_branch_conn_load
                 ${_resdatapath}/MSWcase/MSW_CASE.X0021)

add_test(NAME well_info1 COMMAND well_info ${_resdatapath}/Gurbat/ECLIPSE.EGRID)
add_test(
  NAME well_info2
  COMMAND
    well_info
    ${_resdatapath}/well_info_rio/BMS8_TMPL_1-BMS8_105DST_EMBED_T0_1.EGRID
    ${_resdatapath}/well_info_rio/BMS8_TMPL_1-BMS8_105DST_EMBED_T0_1.UNRST)

add_test(NAME well_conn_CF COMMAND well_conn_CF
                                   ${_resdatapath}/Gurbat/ECLIPSE.X0060)

add_test(NAME well_conn_load1 COMMAND well_conn_load
                                      ${_resdatapath}/Gurbat/ECLIPSE.X0030 F)
add_test(NAME well_conn_load2
         COMMAND well_conn_load
                 ${_resdatapath}/10kcase/TEST10K_FLT_LGR_NNC.X0021 F)
add_test(NAME well_conn_load3 COMMAND well_conn_load
                                      ${_resdatapath}/MSWcase/MSW_CASE.X0021 T)
add_test(NAME well_conn_load4
         COMMAND well_conn_load
                 ${_resdatapath}/AmalgLGRcase/TESTCASE_AMALG_LGR.X0021 F)
add_test(NAME well_conn_load5 COMMAND well_conn_load
                                      ${_resdatapath}/DualPoro/DUALPORO.X0009 F)
add_test(NAME well_conn_load6
         COMMAND well_conn_load
                 ${_resdatapath}/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.X0003 F)

add_test(NAME well_ts COMMAND well_ts ${_resdatapath}/CO2case/BASE_CASE)

add_test(NAME well_dualp
         COMMAND well_dualp ${_resdatapath}/Gurbat/ECLIPSE.UNRST
                 ${_resdatapath}/DualPoro/DUALPORO.X0005)

add_test(
  NAME well_lgr_load1
  COMMAND well_lgr_load ${_resdatapath}/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.EGRID
          ${_resdatapath}/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.X0003)
add_test(
  NAME well_lgr_load2
  COMMAND well_lgr_load ${_resdatapath}/AmalgLGRcase/TESTCASE_AMALG_LGR.EGRID
          ${_resdatapath}/AmalgLGRcase/TESTCASE_AMALG_LGR.X0016)

#
# geometry
#
add_executable(geo_surface geometry/tests/geo_surface.cpp)
target_link_libraries(geo_surface resdata)
add_test(NAME geo_surface COMMAND geo_surface ${_geopath}/Surface.irap
                                  ${_geopath}/Surface_incompatible.irap)
