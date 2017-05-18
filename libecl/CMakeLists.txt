project(libecl-ecl)

add_library(ecl src/ecl_rsthead.c
                src/ecl_sum_tstep.c
                src/ecl_rst_file.c
                src/ecl_init_file.c
                src/ecl_grid_cache.c
                src/smspec_node.c
                src/ecl_kw_grdecl.c
                src/ecl_file_kw.c
                src/ecl_file_view.c
                src/ecl_grav.c
                src/ecl_grav_calc.c
                src/ecl_smspec.c
                src/ecl_sum_data.c
                src/ecl_util.c
                src/ecl_kw.c
                src/ecl_sum.c
                src/ecl_sum_vector.c
                src/fortio.c
                src/ecl_rft_file.c
                src/ecl_rft_node.c
                src/ecl_rft_cell.c
                src/ecl_grid.c
                src/ecl_coarse_cell.c
                src/ecl_box.c
                src/ecl_io_config.c
                src/ecl_file.c
                src/ecl_region.c
                src/ecl_subsidence.c
                src/ecl_grid_dims.c
                src/grid_dims.c
                src/nnc_info.c
                src/ecl_grav_common.c
                src/nnc_vector.c
                src/ecl_nnc_export.c
                src/ecl_nnc_geometry.c
                src/layer.c
                src/fault_block_layer.c
                src/ecl_type.c
                src/ecl_type_python.c
           )

set_target_properties(ecl PROPERTIES
                          VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR}
                          SOVERSION ${ERT_VERSION_MAJOR})
target_link_libraries(ecl ert_geometry ert_util openmp)
target_include_directories(ecl
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
    )

install(TARGETS ecl
        EXPORT  ecl-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION include)

if (NOT BUILD_TESTS)
    return ()
endif ()

foreach (name   ecl_alloc_cpgrid
                ecl_alloc_grid_dxv_dyv_dzv
                ecl_fault_block_layer
                ecl_grid_add_nnc
                ecl_grid_copy
                ecl_grid_create
                ecl_grid_DEPTHZ
                ecl_grid_export
                ecl_grid_init_fwrite
                ecl_grid_reset_actnum
                ecl_init_file
                ecl_kw_cmp_string
                ecl_kw_equal
                ecl_kw_fread
                ecl_kw_grdecl
                ecl_kw_init
                ecl_nnc_geometry
                ecl_nnc_info_test
                ecl_nnc_vector
                ecl_rft_cell
                ecl_rst_file
                ecl_sum_writer
                ecl_util_make_date_no_shift
                ecl_util_month_range
                ecl_valid_basename
        )
        add_executable(${name} tests/${name}.c)
        target_link_libraries(${name} ecl)
        add_test(NAME ${name} COMMAND ${name})
endforeach ()

add_executable(ecl_grid_cell_contains tests/ecl_grid_cell_contains.c)
target_link_libraries(ecl_grid_cell_contains ecl)
add_test(NAME ecl_grid_cell_contains1 COMMAND ecl_grid_cell_contains)

if (HAVE_UTIL_ABORT_INTERCEPT)
   add_executable(ecl_grid_corner tests/ecl_grid_corner.c)
   target_link_libraries(ecl_grid_corner ecl)
   add_test(NAME ecl_grid_corner COMMAND ecl_grid_corner)

   add_executable(ecl_layer tests/ecl_layer.c)
   target_link_libraries(ecl_layer ecl)
   add_test(NAME ecl_layer COMMAND ecl_layer)
endif()

add_executable(ecl_get_num_cpu tests/ecl_get_num_cpu_test.c)
target_link_libraries(ecl_get_num_cpu ecl)
add_test(NAME ecl_get_num_cpu COMMAND ecl_get_num_cpu
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/num_cpu1
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/num_cpu2
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/num_cpu3
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/num_cpu4)

# The ecl_win64 application is not built as a proper test integrated
# into the CTEST system. Should be invoked manually on Windows.
if (ERT_WINDOWS)
   add_executable(ecl_lfs ecl_lfs.c)
   target_link_libraries(ecl_lfs ecl)
endif()

if (NOT STATOIL_TESTDATA_ROOT)
    return ()
endif()

add_executable(ecl_coarse_test tests/ecl_coarse_test.c)
target_link_libraries(ecl_coarse_test ecl)
add_test(NAME ecl_coarse_test COMMAND ecl_coarse_test
    ${_eclpath}/LGCcase/LGC_TESTCASE2)

add_executable(ecl_grid_layer_contains tests/ecl_grid_layer_contains.c)
target_link_libraries(ecl_grid_layer_contains ecl)
add_test(NAME ecl_grid_layer_contains1 COMMAND ecl_grid_layer_contains
    ${_eclpath}/Gurbat/ECLIPSE.EGRID)
add_test(NAME ecl_grid_layer_contains2 COMMAND ecl_grid_layer_contains
    ${_eclpath}/Mariner/MARINER.EGRID)

add_executable(ecl_restart_test tests/ecl_restart_test.c)
target_link_libraries(ecl_restart_test ecl)
add_test(NAME ecl_restart_test COMMAND ecl_restart_test
    ${_eclpath}/Gurbat/ECLIPSE.UNRST)

add_executable(ecl_nnc_export tests/ecl_nnc_export.c)
target_link_libraries(ecl_nnc_export ecl)
add_test(NAME ecl_nnc_export1 COMMAND ecl_nnc_export ${_eclpath}/Gurbat/ECLIPSE TRUE)
add_test(NAME ecl_nnc_export2 COMMAND ecl_nnc_export ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC TRUE)
add_test(NAME ecl_nnc_export3 COMMAND ecl_nnc_export ${_eclpath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3 TRUE)
add_test(NAME ecl_nnc_export4 COMMAND ecl_nnc_export ${_eclpath}/DualPoro/DUAL_DIFF TRUE)
add_test(NAME ecl_nnc_export5 COMMAND ecl_nnc_export ${_eclpath}/DualPoro/DUALPORO TRUE)
add_test(NAME ecl_nnc_export6 COMMAND ecl_nnc_export ${_eclpath}/nestedLGRcase/TESTCASE_NESTEDLGR TRUE)
add_test(NAME ecl_nnc_export7 COMMAND ecl_nnc_export ${_eclpath}/TYRIHANS/BASE20150218_MULTFLT FALSE)

add_executable(ecl_nnc_export_get_tran tests/ecl_nnc_export_get_tran.c)
target_link_libraries(ecl_nnc_export_get_tran ecl)
add_test(NAME ecl_nnc_export_get_tran COMMAND ecl_nnc_export_get_tran
    ${_eclpath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3)

add_executable(ecl_util_make_date_shift tests/ecl_util_make_date_shift.c)
target_link_libraries(ecl_util_make_date_shift ecl)
add_test(NAME ecl_util_make_date_shift COMMAND ecl_util_make_date_shift)

add_executable(ecl_sum_case_exists tests/ecl_sum_case_exists.c)
target_link_libraries(ecl_sum_case_exists ecl)
add_test(NAME ecl_sum_case_exists COMMAND ecl_sum_case_exists
         ${_eclpath}/Gurbat/ECLIPSE
         ${_eclpath}/GurbatSummary/missingHeader/ECLIPSE
         ${_eclpath}/GurbatSummary/missingData/ECLIPSE)


add_executable(ecl_grid_lgr_name tests/ecl_grid_lgr_name.c)
target_link_libraries(ecl_grid_lgr_name ecl)
add_test(NAME ecl_grid_lgr_name COMMAND ecl_grid_lgr_name
    ${_eclpath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3.EGRID)

add_executable(ecl_region tests/ecl_region.c)
target_link_libraries(ecl_region ecl)
add_test(NAME ecl_region COMMAND ecl_region ${_eclpath}/Gurbat/ECLIPSE.EGRID)

add_test(NAME ecl_grid_cell_contains2 COMMAND ecl_grid_cell_contains ${_eclpath}/Gurbat/ECLIPSE.EGRID)
add_test(NAME ecl_grid_cell_contains3 COMMAND ecl_grid_cell_contains ${_eclpath}/FF12/FF12_2013B2.EGRID)
add_test(NAME ecl_grid_cell_contains4 COMMAND ecl_grid_cell_contains ${_eclpath}/Brazil/R3_ICD.EGRID)

add_executable(ecl_grid_cell_contains_wellpath tests/ecl_grid_cell_contains_wellpath.c)
target_link_libraries(ecl_grid_cell_contains_wellpath ecl)
add_test(NAME ecl_grid_cell_contains_wellpath1
        COMMAND ecl_grid_cell_contains_wellpath
            ${_eclpath}/CellContains/model/SMS-0.EGRID
            ${_eclpath}/CellContains/R_PB-4H.jira)

add_executable(ecl_grid_cell_volume tests/ecl_grid_cell_volume.c)
target_link_libraries(ecl_grid_cell_volume ecl)
add_test(NAME ecl_grid_cell_volume1 COMMAND ecl_grid_cell_volume)
add_test(NAME ecl_grid_cell_volume2 COMMAND ecl_grid_cell_volume ${_eclpath}/Gurbat/ECLIPSE.EGRID)
add_test(NAME ecl_grid_cell_volume3 COMMAND ecl_grid_cell_volume ${_eclpath}/Heidrun/Summary/FF12_2013B3_CLEAN_RS.EGRID)

add_executable(ecl_region2region tests/ecl_region2region_test.c)
target_link_libraries(ecl_region2region ecl)
add_test(NAME ecl_region2region COMMAND ecl_region2region ${_eclpath}/R2R/R2R.SMSPEC)

add_executable(ecl_grid_case tests/ecl_grid_case.c)
target_link_libraries(ecl_grid_case ecl)
add_test(NAME ecl_grid_case COMMAND ecl_grid_case
    ${_eclpath}/Gurbat/ECLIPSE.EGRID
    ${_eclpath}/Gurbat/ECLIPSE)

add_executable(ecl_lgr_test tests/ecl_lgr_test.c)
target_link_libraries(ecl_lgr_test ecl)
add_test(NAME ecl_lgr_test1 COMMAND ecl_lgr_test ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID)
add_test(NAME ecl_lgr_test2 COMMAND ecl_lgr_test ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.GRID)
add_test(NAME ecl_lgr_test3 COMMAND ecl_lgr_test ${_eclpath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3.EGRID)

add_executable(ecl_grid_simple tests/ecl_grid_simple.c)
target_link_libraries(ecl_grid_simple ecl)
add_test(NAME ecl_grid_simple COMMAND ecl_grid_simple ${_eclpath}/Gurbat/ECLIPSE.EGRID)

add_test(NAME ecl_grid_ecl2015_2 COMMAND ecl_grid_simple
    ${_eclpath}/Eclipse2015_NNC_BUG/FF15_2015B2_LGRM_RDI15_HIST_RDIREAL1_20142.EGRID)

add_executable(ecl_grid_export_statoil tests/ecl_grid_export.c)
target_link_libraries(ecl_grid_export_statoil ecl)
add_test(NAME ecl_grid_export_statoil
    COMMAND ecl_grid_export_statoil ${_eclpath}/Gurbat/ECLIPSE.EGRID)

add_executable(ecl_grid_volume tests/ecl_grid_volume.c)
target_link_libraries(ecl_grid_volume ecl)
add_test(NAME ecl_grid_volume1 COMMAND ecl_grid_volume ${_eclpath}/Gurbat/ECLIPSE)
add_test(NAME ecl_grid_volume2 COMMAND ecl_grid_volume ${_eclpath}/VolumeTest/TEST1)
add_test(NAME ecl_grid_volume3 COMMAND ecl_grid_volume ${_eclpath}/OsebergSyd/Omega/OMEGA-0)
add_test(NAME ecl_grid_volume4 COMMAND ecl_grid_volume ${_eclpath}/Norne/reservoir_models/Norne_ATW2013/NORNE_ATW2013)

# The grid volume test fails miserably on the test case given as example five; looking at
# the failures one could actually suspect that the ECLIPSE algorithm for PORV calculations
# has been different in this file - i.e. that the absolute value of the individual
# tetrahedron parts have been taken during the sum, and not at the end. At least the ert
# algorithm gets volumes ~ 0 whereas ECLIPSE reports ~10^9 for the same cell.
# add_test( ecl_grid_volume5 ${EXECUTABLE_OUTPUT_PATH}/ecl_grid_volume ${_eclpath}/Heidrun/Summary/FF12_2013B3_CLEAN_RS)

add_executable(ecl_grid_dims tests/ecl_grid_dims.c)
target_link_libraries(ecl_grid_dims ecl)
add_test(NAME ecl_grid_dims0 COMMAND ecl_grid_dims)
add_test(NAME ecl_grid_dims1 COMMAND ecl_grid_dims ${_eclpath}/Gurbat/ECLIPSE.EGRID ${_eclpath}/Gurbat/ECLIPSE.INIT)
add_test(NAME ecl_grid_dims2 COMMAND ecl_grid_dims ${_eclpath}/Gurbat/ECLIPSE.GRID  ${_eclpath}/Gurbat/ECLIPSE.INIT)
add_test(NAME ecl_grid_dims3 COMMAND ecl_grid_dims ${_eclpath}/Gurbat/ECLIPSE.EGRID)
add_test(NAME ecl_grid_dims4 COMMAND ecl_grid_dims ${_eclpath}/Gurbat/ECLIPSE.GRID )
add_test(NAME ecl_grid_dims5 COMMAND ecl_grid_dims ${_eclpath}/AmalgLGRcase/TESTCASE_AMALG_LGR.EGRID)

add_executable(ecl_nnc_test tests/ecl_nnc_test.c)
target_link_libraries(ecl_nnc_test ecl)
add_test(NAME ecl_nnc_test1 COMMAND ecl_nnc_test ${_eclpath}/Gurbat/ECLIPSE.EGRID )
add_test(NAME ecl_nnc_test2 COMMAND ecl_nnc_test ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID )
add_test(NAME ecl_nnc_test3 COMMAND ecl_nnc_test ${_eclpath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3.EGRID)
add_test(NAME ecl_nnc_test4 COMMAND ecl_nnc_test ${_eclpath}/DualPoro/DUAL_DIFF.EGRID )
add_test(NAME ecl_nnc_test5 COMMAND ecl_nnc_test ${_eclpath}/nestedLGRcase/TESTCASE_NESTEDLGR.EGRID)

add_executable(ecl_layer_statoil tests/ecl_layer_statoil.c)
target_link_libraries(ecl_layer_statoil ecl)
add_test(NAME ecl_layer_statoil COMMAND ecl_layer_statoil
    ${_eclpath}/Mariner/MARINER.EGRID
    ${_eclpath}/Mariner/faultblock.grdecl)

add_executable(ecl_dualp tests/ecl_dualp.c)
target_link_libraries(ecl_dualp ecl)
add_test(NAME ecl_dualp COMMAND ecl_dualp ${_eclpath}/LGCcase/LGC_TESTCASE2)

add_executable(ecl_sum_test tests/ecl_sum_test.c)
target_link_libraries(ecl_sum_test ecl)
add_test(NAME ecl_sum_test COMMAND ecl_sum_test ${_eclpath}/Gurbat/ECLIPSE)

add_executable(ecl_sum_report_step_equal tests/ecl_sum_report_step_equal.c)
target_link_libraries(ecl_sum_report_step_equal ecl)
add_test(NAME ecl_sum_report_step_equal1 COMMAND ecl_sum_report_step_equal ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/Snorre/SNORRE FALSE)
add_test(NAME ecl_sum_report_step_equal2 COMMAND ecl_sum_report_step_equal ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/Gurbat/ECLIPSE TRUE)
add_test(NAME ecl_sum_report_step_equal3 COMMAND ecl_sum_report_step_equal ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/modGurbat/extraMinistep/ECLIPSE TRUE)
add_test(NAME ecl_sum_report_step_equal4 COMMAND ecl_sum_report_step_equal ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/modGurbat/short/ECLIPSE FALSE)
add_test(NAME ecl_sum_report_step_equal5 COMMAND ecl_sum_report_step_equal ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/modGurbat/enkf/ECLIPSE FALSE)
add_test(NAME ecl_sum_report_step_equal6 COMMAND ecl_sum_report_step_equal ${_eclpath}/Snorre/SNORRE  ${_eclpath}/Snorre2/SNORRE2 FALSE)

add_executable(ecl_sum_report_step_compatible tests/ecl_sum_report_step_compatible.c)
target_link_libraries(ecl_sum_report_step_compatible ecl)
add_test(NAME ecl_sum_report_step_compatible1 COMMAND ecl_sum_report_step_compatible ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/Snorre/SNORRE FALSE)
add_test(NAME ecl_sum_report_step_compatible2 COMMAND ecl_sum_report_step_compatible ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/Gurbat/ECLIPSE TRUE)
add_test(NAME ecl_sum_report_step_compatible3 COMMAND ecl_sum_report_step_compatible ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/modGurbat/extraMinistep/ECLIPSE TRUE)
add_test(NAME ecl_sum_report_step_compatible4 COMMAND ecl_sum_report_step_compatible ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/modGurbat/short/ECLIPSE TRUE)
add_test(NAME ecl_sum_report_step_compatible5 COMMAND ecl_sum_report_step_compatible ${_eclpath}/Gurbat/ECLIPSE ${_eclpath}/modGurbat/enkf/ECLIPSE TRUE)
add_test(NAME ecl_sum_report_step_compatible6 COMMAND ecl_sum_report_step_equal      ${_eclpath}/Snorre/SNORRE  ${_eclpath}/Snorre2/SNORRE2 FALSE)

add_executable(ecl_file tests/ecl_file.c)
target_link_libraries(ecl_file ecl)
add_test(NAME ecl_file
         COMMAND ecl_file ${_eclpath}/Gurbat/ECLIPSE.UNRST ECLIPSE.UNRST)

add_executable(ecl_fmt tests/ecl_fmt.c)
target_link_libraries(ecl_fmt  ecl)
add_test(NAME ecl_fmt COMMAND ecl_fmt
    ${_eclpath}/Gurbat/ECLIPSE.UNRST
    ${_eclpath}/Gurbat/ECLIPSE.DATA)


add_executable(ecl_rsthead tests/ecl_rsthead.c)
target_link_libraries(ecl_rsthead ecl)
add_test(NAME ecl_rsthead COMMAND ecl_rsthead
    ${_eclpath}/Gurbat/ECLIPSE.UNRST
    ${_eclpath}/DualPoro/DUALPORO.X0005)

add_executable(ecl_smspec tests/ecl_smspec.c)
target_link_libraries(ecl_smspec ecl)
add_test(NAME ecl_smspec COMMAND ecl_smspec
    ${_eclpath}/Gurbat/ECLIPSE.SMSPEC
    ${_eclpath}/Heidrun/Summary/FF12_2013B3_CLEAN_RS.SMSPEC)


add_executable(ecl_rft tests/ecl_rft.c)
target_link_libraries(ecl_rft ecl)
add_test(NAME ecl_rft_rft    COMMAND ecl_rft ${_eclpath}/Gurbat/ECLIPSE.RFT RFT)
add_test(NAME ecl_rft_rft_rw COMMAND ecl_rft ${_eclpath}/Gurbat/ECLIPSE.RFT RFT_RW)
add_test(NAME ecl_rft_plt    COMMAND ecl_rft ${_eclpath}/RFT/TEST1_1A.RFT PLT)
add_test(NAME ecl_rft_mswplt COMMAND ecl_rft ${_eclpath}/RFT/RFT2.RFT MSW-PLT)

add_executable(ecl_grid_copy_statoil tests/ecl_grid_copy_statoil.c)
target_link_libraries(ecl_grid_copy_statoil ecl)

add_test(NAME ecl_grid_copy_statoil1 COMMAND ecl_grid_copy_statoil ${_eclpath}/Gurbat/ECLIPSE.EGRID)
add_test(NAME ecl_grid_copy_statoil2 COMMAND ecl_grid_copy_statoil ${_eclpath}/Mariner/MARINER.EGRID)
add_test(NAME ecl_grid_copy_statoil3 COMMAND ecl_grid_copy_statoil ${_eclpath}/LGCcase/LGC_TESTCASE2.EGRID)
add_test(NAME ecl_grid_copy_statoil4 COMMAND ecl_grid_copy_statoil ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID)

add_executable(ecl_fault_block_layer_statoil tests/ecl_fault_block_layer_statoil.c)
target_link_libraries(ecl_fault_block_layer_statoil ecl)
add_test(NAME ecl_fault_block_layer_statoil COMMAND ecl_fault_block_layer_statoil
    ${_eclpath}/Mariner/MARINER.EGRID
    ${_eclpath}/Mariner/faultblock.grdecl)

if (HAVE_UTIL_ABORT_INTERCEPT)
   add_executable(ecl_fortio tests/ecl_fortio.c)
   target_link_libraries( ecl_fortio ecl)
   add_test(NAME ecl_fortio COMMAND ecl_fortio ${_eclpath}/Gurbat/ECLIPSE.UNRST)
endif()
