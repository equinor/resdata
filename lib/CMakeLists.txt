if(ERT_HAVE_GETUID AND ERT_HAVE_OPENDIR)
  list(APPEND opt_srcs util/test_work_area.cpp)
endif()

if(ERT_HAVE_OPENDIR)
  list(APPEND opt_srcs util/util_opendir.cpp)
endif()

if(ERT_HAVE_UNISTD)
  list(APPEND opt_srcs util/path_stack.cpp)
endif()

# Make cmath have defines like M_PI
add_compile_definitions(_USE_MATH_DEFINES)

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
  util/util_lfs.cpp
  util/util_unlink.cpp
  util/vector.cpp
  util/parser.cpp
  util/stringlist.cpp
  util/string_util.cpp
  util/type_vector_functions.cpp
  util/perm_vector.cpp
  util/cxx_string_util.cpp
  ${opt_srcs}
  resdata/rd_sum_tstep.cpp
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
  resdata/rd_grid.cpp
  resdata/rd_coarse_cell.cpp
  resdata/rd_box.cpp
  resdata/rd_file.cpp
  resdata/rd_region.cpp
  resdata/rd_subsidence.cpp
  resdata/nnc_info.cpp
  resdata/rd_grav_common.cpp
  resdata/nnc_vector.cpp
  resdata/layer.cpp
  resdata/fault_block.cpp
  resdata/fault_block_layer.cpp
  resdata/rd_type.cpp
  resdata/well_state.cpp
  resdata/well_conn.cpp
  resdata/well_info.cpp
  resdata/well_segment.cpp
  resdata/well_segment_collection.cpp
  resdata/well_branch_collection.cpp
  resdata/well_rseg_loader.cpp
  resdata/FortIO.cpp
  geometry/geo_surface.cpp
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
  PRIVATE fmt::fmt Backward::Backward)

target_include_directories(
  resdata
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include>
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

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

# Helper to register a pybind11 extension module that wraps a single
# <classname>_pybind.cpp together with the shared cwrap_pybind.cpp glue.
# module_name : name of the python extension (e.g. _grid) source_file : the
# <classname>_pybind.cpp in lib/resdata/ py_pkg_path : install/output package
# path under python/ (e.g. resdata/grid)
function(add_resdata_pybind_module module_name source_file py_pkg_path)
  pybind11_add_module(${module_name} ${source_file} resdata/cwrap_pybind.cpp)
  target_link_libraries(${module_name} PRIVATE resdata fmt::fmt)
  target_include_directories(
    ${module_name}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_BINARY_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
  if(SKBUILD)
    set(${module_name}_install_dir "python/${py_pkg_path}")
    # libresdata.so.<major> is installed at python/resdata/.libs/. The extension
    # is installed at python/${py_pkg_path}/, which may be at any depth. Compute
    # how many directories we must climb to get back to python/resdata/ so the
    # RPATH is correct regardless of the module's package depth.
    string(REPLACE "/" ";" _pkg_parts "${py_pkg_path}")
    list(LENGTH _pkg_parts _pkg_depth)
    math(EXPR _levels_up "${_pkg_depth} - 1")

    if(APPLE)
      set(_origin "@loader_path")
    else()
      set(_origin "$ORIGIN")
    endif()

    set(_rpath "${_origin}")
    if(_levels_up GREATER 0)
      string(REPEAT "/.." ${_levels_up} _up)
      string(APPEND _rpath "${_up}")
    endif()
    set(_rpath "${_rpath}/.libs")
    set_target_properties(${module_name} PROPERTIES INSTALL_RPATH "${_rpath}")
  else()
    set(${module_name}_install_dir "${CMAKE_INSTALL_LIBDIR}")
  endif()
  install(TARGETS ${module_name}
          LIBRARY DESTINATION ${${module_name}_install_dir})
  # When running tests against the build tree (editable install), put the
  # extension in the source package directory so it can be imported without
  # installing.
  set_target_properties(
    ${module_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                              ${PROJECT_SOURCE_DIR}/python/${py_pkg_path})
endfunction()

add_resdata_pybind_module(_grid resdata/rd_grid_pybind.cpp resdata/grid)
add_resdata_pybind_module(_kw resdata/rd_kw_pybind.cpp resdata/resfile)
add_resdata_pybind_module(_rsthead resdata/rsthead_pybind.cpp resdata/resfile)
add_resdata_pybind_module(rd_file resdata/rd_file_pybind.cpp resdata/resfile)
add_resdata_pybind_module(rd_file_view resdata/rd_file_view_pybind.cpp
                          resdata/resfile)
add_resdata_pybind_module(fortio resdata/fortio_pybind.cpp resdata/resfile)
add_resdata_pybind_module(well resdata/well_pybind.cpp resdata)
add_resdata_pybind_module(_subsidence resdata/rd_subsidence_pybind.cpp
                          resdata/gravimetry)
add_resdata_pybind_module(_grav resdata/rd_grav_pybind.cpp resdata/gravimetry)
add_resdata_pybind_module(_rd_sum resdata/rd_sum_pybind.cpp resdata/summary)
add_resdata_pybind_module(_type resdata/rd_type_pybind.cpp resdata/types)
add_resdata_pybind_module(_rd_util resdata/rd_util_pybind.cpp resdata)

add_resdata_pybind_module(_cpolyline geometry/geo_polygon_pybind.cpp
                          resdata/geometry)
add_resdata_pybind_module(
  _cpolyline_collection geometry/geo_polygon_collection_pybind.cpp
  resdata/geometry)
add_resdata_pybind_module(_geo_pointset geometry/geo_pointset_pybind.cpp
                          resdata/geometry)
add_resdata_pybind_module(_geo_region geometry/geo_region_pybind.cpp
                          resdata/geometry)
add_resdata_pybind_module(_surface geometry/geo_surface_pybind.cpp
                          resdata/geometry)
add_resdata_pybind_module(_fault_block resdata/fault_block_pybind.cpp
                          resdata/grid/faults)
add_resdata_pybind_module(
  _fault_block_layer resdata/fault_block_layer_pybind.cpp resdata/grid/faults)
add_resdata_pybind_module(_layer resdata/layer_pybind.cpp resdata/grid/faults)

if(NOT BUILD_TESTS)
  return()
endif()

add_executable(
  rd_test_suite
  tests/test_rd_grid_regular.cpp
  tests/test_nnc_info.cpp
  tests/test_rd_grid_nnc_lgr.cpp
  tests/test_rd_grid_coarse.cpp
  tests/test_rd_grid_xyz_lookup.cpp
  tests/test_rd_grid_compare.cpp
  tests/test_rd_grid_misc.cpp
  tests/test_rd_grid_bounds_checking.cpp
  tests/test_rd_sum.cpp
  tests/test_rd_type.cpp
  tests/test_time_index.cpp
  tests/test_rd_kw.cpp
  tests/test_well_info.cpp
  tests/test_well_keyword_validation.cpp
  tests/test_rd_util.cpp
  tests/test_fortio.cpp
  tests/test_geometry.cpp
  tests/test_rd_file_kw.cpp
  tests/testsuite.cpp
  tests/test_util.cpp
  tests/test_rd_region.cpp
  tests/test_geertsma.cpp
  tests/test_layer.cpp
  tests/test_fault_block_layer.cpp)
target_compile_features(rd_test_suite PUBLIC cxx_std_17)
target_include_directories(rd_test_suite
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
target_link_libraries(rd_test_suite resdata Catch2::Catch2)
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

  add_executable(${name} util/tests/${name}.cpp util/test_util.cpp)
  target_include_directories(
    ${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
  target_link_libraries(${name} resdata)
  add_test(NAME ${name} COMMAND ${name})
endforeach()

add_executable(rd_smspec_node resdata/tests/rd_smspec_node.cpp
                              util/test_util.cpp)
target_link_libraries(rd_smspec_node resdata)
add_test(NAME rd_smspec_node COMMAND rd_smspec_node)

add_executable(ert_util_work_area util/tests/ert_util_work_area.cpp
                                  util/test_util.cpp)
target_link_libraries(ert_util_work_area resdata)
add_test(
  NAME ert_util_work_area
  COMMAND ert_util_work_area data2/file1
          ${CMAKE_CURRENT_SOURCE_DIR}/util/tests/data2/file2 data2
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/util/tests)

add_executable(ert_util_cwd_test util/tests/ert_util_cwd_test.cpp
                                 util/test_util.cpp)
target_link_libraries(ert_util_cwd_test resdata)
add_test(NAME ert_util_cwd_test COMMAND ert_util_cwd_test
                                        ${CMAKE_CURRENT_BINARY_DIR})

add_executable(ert_util_copy_file util/tests/ert_util_copy_file.cpp
                                  util/test_util.cpp)
target_link_libraries(ert_util_copy_file resdata)
add_test(NAME ert_util_copy_file COMMAND ert_util_copy_file
                                         $<TARGET_FILE:ert_util_copy_file>)

add_executable(ert_util_path_stack_test util/tests/ert_util_path_stack_test.cpp
                                        util/test_util.cpp)
target_link_libraries(ert_util_path_stack_test resdata)
add_test(NAME ert_util_path_stack_test
         COMMAND ert_util_path_stack_test ${CMAKE_CURRENT_BINARY_DIR}
                 ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ert_util_abort_gnu_tests util/tests/ert_util_abort_gnu_tests.cpp
                                        util/test_util.cpp)
target_link_libraries(ert_util_abort_gnu_tests resdata)
add_test(NAME ert_util_abort_gnu_tests COMMAND ert_util_abort_gnu_tests)

add_executable(ert_util_type_vector_test
               util/tests/ert_util_type_vector_test.cpp util/test_util.cpp)
target_link_libraries(ert_util_type_vector_test resdata)
add_test(NAME ert_util_type_vector_test COMMAND ert_util_type_vector_test)

foreach(
  name
  rd_fault_block_layer
  rd_path_access
  rd_grid_add_nnc
  rd_grid_copy
  rd_grid_unit_system
  rd_grid_export
  rd_grid_init_fwrite
  rd_grid_reset_actnum
  rd_kw_space_pad
  rd_kw_cmp_string
  rd_kw_equal
  rd_kw_fread
  rd_kw_grdecl
  rd_kw_init
  rd_layer
  rd_nnc_info_test
  rd_nnc_vector
  test_rd_file_index
  rd_filenames
  rd_make_date_no_shift
  rd_make_date_shift
  well_branch_collection
  test_well_conn
  test_well_state
  test_well_segment
  well_segment_conn
  well_segment_collection
  test_rd_file)
  add_executable(${name} resdata/tests/${name}.cpp util/test_util.cpp)
  target_link_libraries(${name} resdata)
  target_include_directories(
    ${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private-include)
  add_test(NAME ${name} COMMAND ${name})
endforeach()

if(HAVE_UTIL_ABORT_INTERCEPT)
  add_executable(rd_grid_corner resdata/tests/rd_grid_corner.cpp
                                util/test_util.cpp)
  target_link_libraries(rd_grid_corner resdata)
  add_test(NAME rd_grid_corner COMMAND rd_grid_corner)
endif()

add_executable(rd_get_num_cpu resdata/tests/rd_get_num_cpu_test.cpp
                              util/test_util.cpp)
target_link_libraries(rd_get_num_cpu resdata)
add_test(
  NAME rd_get_num_cpu
  COMMAND
    rd_get_num_cpu ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu1
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu2
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu3
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu4
    ${CMAKE_CURRENT_SOURCE_DIR}/resdata/tests/data/num_cpu5)

foreach(test rdxx_kw rdxx_types)
  add_executable(${test} resdata/tests/${test}.cpp util/test_util.cpp)
  target_link_libraries(${test} resdata)
  add_test(NAME ${test} COMMAND ${test})
endforeach()

foreach(
  name
  rd_coarse_test
  rd_grid_layer_contains
  rd_restart_test
  rd_grid_lgr_name
  rd_region
  rd_grid_cell_contains_wellpath
  rd_region2region
  rd_grid_volume
  rd_nnc_test
  rd_lgr_test
  rd_dualp
  rd_grid_dx_dy_dz
  rd_sum_test
  rd_file_equinor
  rd_fmt
  rd_rsthead
  well_state_load
  well_state_load_missing_RSEG
  well_segment_load
  test_well_info
  well_conn_CF
  well_ts
  well_dualp
  well_lgr_load)

  add_executable(${name} resdata/tests/${name}.cpp util/test_util.cpp)
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

add_test(NAME rd_dualp COMMAND rd_dualp ${_resdatapath}/LGCcase/LGC_TESTCASE2)

add_test(NAME rd_sum_test COMMAND rd_sum_test ${_resdatapath}/Gurbat/ECLIPSE)

add_test(NAME rd_file_equinor
         COMMAND rd_file_equinor ${_resdatapath}/Gurbat/ECLIPSE.UNRST
                 ECLIPSE.UNRST)

add_test(NAME rd_rsthead
         COMMAND rd_rsthead ${_resdatapath}/Gurbat/ECLIPSE.UNRST
                 ${_resdatapath}/DualPoro/DUALPORO.X0005)

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

add_test(NAME well_info1 COMMAND test_well_info
                                 ${_resdatapath}/Gurbat/ECLIPSE.EGRID)
add_test(
  NAME well_info2
  COMMAND
    test_well_info
    ${_resdatapath}/well_info_rio/BMS8_TMPL_1-BMS8_105DST_EMBED_T0_1.EGRID
    ${_resdatapath}/well_info_rio/BMS8_TMPL_1-BMS8_105DST_EMBED_T0_1.UNRST)

add_test(NAME well_conn_CF COMMAND well_conn_CF
                                   ${_resdatapath}/Gurbat/ECLIPSE.X0060)

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
