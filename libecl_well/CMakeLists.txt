project(libecl-well C)

add_library(ecl_well    src/well_state.c
                        src/well_conn.c
                        src/well_info.c
                        src/well_ts.c
                        src/well_conn_collection.c
                        src/well_segment.c
                        src/well_segment_collection.c
                        src/well_branch_collection.c
                        src/well_rseg_loader.c
           )

target_include_directories(ecl_well
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
    )

target_link_libraries(ecl_well ecl)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS ecl_well
        EXPORT  ecl-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set_target_properties(ecl_well PROPERTIES
        VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR}
        SOVERSION ${ERT_VERSION_MAJOR}
    )

if (NOT BUILD_TESTS)
    return ()
endif()

foreach (name well_conn_collection
              well_branch_collection
              well_conn
              well_state
              well_segment
              well_segment_conn
              well_segment_collection)

    add_executable(${name} tests/${name}.c)
    target_link_libraries(${name} ecl_well)
    add_test(NAME ${name} COMMAND ${name})
endforeach()

if (NOT STATOIL_TESTDATA_ROOT)
    return ()
endif ()

add_executable(well_state_load tests/well_state_load.c)
target_link_libraries( well_state_load ecl_well)

add_executable(well_state_load_missing_RSEG tests/well_state_load_missing_RSEG.c)
target_link_libraries(well_state_load_missing_RSEG ecl_well)

add_test(NAME well_state_load1 COMMAND well_state_load ${_eclpath}/Gurbat/ECLIPSE.EGRID
                                                       ${_eclpath}/Gurbat/ECLIPSE.X0030)
add_test(NAME well_state_load2 COMMAND well_state_load ${_eclpath}/MSWcase/MSW_CASE.EGRID
                                                       ${_eclpath}/MSWcase/MSW_CASE.X0021)
add_test(NAME well_state_load3 COMMAND well_state_load ${_eclpath}/Troll/MSW/MSW.EGRID
                                                       ${_eclpath}/Troll/MSW/MSW.X0123)
add_test(NAME well_state_load4 COMMAND well_state_load ${_eclpath}/Troll/MSW_LGR/LGR.EGRID
                                                       ${_eclpath}/Troll/MSW_LGR/LGR.X0095)
add_test(NAME well_state_load5 COMMAND well_state_load ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID
                                                       ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.X0061)
add_test(NAME well_state_load_missing_RSEG1
         COMMAND well_state_load_missing_RSEG ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.EGRID
                                              ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.X0061)
add_test(NAME well_state_load_missing_RSEG2
         COMMAND well_state_load_missing_RSEG ${_eclpath}/Troll/MSW/MSW.EGRID
                                              ${_eclpath}/Troll/MSW/MSW.X0123)

add_executable(well_segment_load tests/well_segment_load.c)
target_link_libraries(well_segment_load ecl_well)
add_test(NAME well_segment_load
        COMMAND well_segment_load ${_eclpath}/MSWcase/MSW_CASE.X0021)

add_executable(well_segment_branch_conn_load tests/well_segment_branch_conn_load.c)
target_link_libraries(well_segment_branch_conn_load ecl_well)
add_test(NAME well_segment_branch_conn_load
         COMMAND well_segment_branch_conn_load ${_eclpath}/MSWcase/MSW_CASE.X0021)

add_executable(well_info tests/well_info.c)
target_link_libraries(well_info ecl_well)
add_test(NAME well_info COMMAND well_info ${_eclpath}/Gurbat/ECLIPSE.EGRID)

add_executable(well_conn_CF tests/well_conn_CF.c)
target_link_libraries(well_conn_CF ecl_well)
add_test(NAME well_conn_CF COMMAND well_conn_CF ${_eclpath}/Gurbat/ECLIPSE.X0060)

add_executable(well_conn_load tests/well_conn_load.c)
target_link_libraries(well_conn_load ecl_well)
add_test(NAME well_conn_load1 COMMAND well_conn_load ${_eclpath}/Gurbat/ECLIPSE.X0030 F)
add_test(NAME well_conn_load2 COMMAND well_conn_load ${_eclpath}/10kcase/TEST10K_FLT_LGR_NNC.X0021 F)
add_test(NAME well_conn_load3 COMMAND well_conn_load ${_eclpath}/MSWcase/MSW_CASE.X0021 T)
add_test(NAME well_conn_load4 COMMAND well_conn_load ${_eclpath}/AmalgLGRcase/TESTCASE_AMALG_LGR.X0021 F)
add_test(NAME well_conn_load5 COMMAND well_conn_load ${_eclpath}/DualPoro/DUALPORO.X0009 F)
add_test(NAME well_conn_load6 COMMAND well_conn_load ${_eclpath}/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.X0003 F)

add_executable(well_ts tests/well_ts.c)
target_link_libraries(well_ts ecl_well)
add_test(NAME well_ts COMMAND well_ts ${_eclpath}/CO2case/BASE_CASE)

add_executable(well_dualp tests/well_dualp.c)
target_link_libraries(well_dualp ecl_well)
add_test(NAME well_dualp COMMAND well_dualp
    ${_eclpath}/Gurbat/ECLIPSE.UNRST
    ${_eclpath}/DualPoro/DUALPORO.X0005)

add_executable(well_lgr_load tests/well_lgr_load.c)
target_link_libraries(well_lgr_load ecl_well)

add_test(NAME well_lgr_load1 COMMAND well_lgr_load ${_eclpath}/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.EGRID
                                                   ${_eclpath}/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.X0003)
add_test(NAME well_lgr_load2 COMMAND well_lgr_load ${_eclpath}/AmalgLGRcase/TESTCASE_AMALG_LGR.EGRID
                                                   ${_eclpath}/AmalgLGRcase/TESTCASE_AMALG_LGR.X0016)
