add_executable( well_conn_collection well_conn_collection.c )
target_link_libraries( well_conn_collection ecl_well )
set_target_properties( well_conn_collection PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_conn_collection ${EXECUTABLE_OUTPUT_PATH}/well_conn_collection )


add_executable( well_conn well_conn.c )
target_link_libraries( well_conn ecl_well )
set_target_properties( well_conn PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_conn ${EXECUTABLE_OUTPUT_PATH}/well_conn )


add_executable( well_segment well_segment.c )
target_link_libraries( well_segment ecl_well )
set_target_properties( well_segment PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_segment ${EXECUTABLE_OUTPUT_PATH}/well_segment )

add_executable( well_segment_conn well_segment_conn.c )
target_link_libraries( well_segment_conn ecl_well )
set_target_properties( well_segment_conn PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_segment_conn ${EXECUTABLE_OUTPUT_PATH}/well_segment_conn )

add_executable( well_segment_load well_segment_load.c )
target_link_libraries( well_segment_load ecl_well )
set_target_properties( well_segment_load PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_segment_load ${EXECUTABLE_OUTPUT_PATH}/well_segment_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/MSWcase/MSW_CASE.X0021)

add_executable( well_segment_conn_load well_segment_conn_load.c )
target_link_libraries( well_segment_conn_load ecl_well )
set_target_properties( well_segment_conn_load PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_segment_conn_load ${EXECUTABLE_OUTPUT_PATH}/well_segment_conn_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/MSWcase/MSW_CASE.X0021)

add_executable( well_info well_info.c )
target_link_libraries( well_info ecl_well )
set_target_properties( well_info PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_info ${EXECUTABLE_OUTPUT_PATH}/well_info ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/Gurbat/ECLIPSE.EGRID )


add_executable( well_segment_collection well_segment_collection.c )
target_link_libraries( well_segment_collection ecl_well )
set_target_properties( well_segment_collection PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_segment_collection ${EXECUTABLE_OUTPUT_PATH}/well_segment_collection )


add_executable( well_conn_load well_conn_load.c )
target_link_libraries( well_conn_load ecl_well )
set_target_properties( well_conn_load PROPERTIES COMPILE_FLAGS "-Werror")                                    
add_test( well_conn_load1 ${EXECUTABLE_OUTPUT_PATH}/well_conn_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/Gurbat/ECLIPSE.X0030 F)
add_test( well_conn_load2 ${EXECUTABLE_OUTPUT_PATH}/well_conn_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/10kcase/TEST10K_FLT_LGR_NNC.X0021 F)
add_test( well_conn_load3 ${EXECUTABLE_OUTPUT_PATH}/well_conn_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/MSWcase/MSW_CASE.X0021 T)
add_test( well_conn_load4 ${EXECUTABLE_OUTPUT_PATH}/well_conn_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/AmalgLGRcase/TESTCASE_AMALG_LGR.X0021 F)
add_test( well_conn_load5 ${EXECUTABLE_OUTPUT_PATH}/well_conn_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/DualPoro/DUALPORO.X0009 F)
add_test( well_conn_load6 ${EXECUTABLE_OUTPUT_PATH}/well_conn_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.X0003 F)

add_executable( well_ts well_ts.c )
target_link_libraries( well_ts ecl_well )
add_test( well_ts  ${EXECUTABLE_OUTPUT_PATH}/well_ts ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/CO2case/BASE_CASE )


add_executable( well_dualp well_dualp.c )
target_link_libraries( well_dualp ecl_well )
add_test( well_dualp  ${EXECUTABLE_OUTPUT_PATH}/well_dualp ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/Gurbat/ECLIPSE.UNRST 
                                                            ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/DualPoro/DUALPORO.X0005  )


add_executable( well_lgr_load well_lgr_load.c )
target_link_libraries( well_lgr_load ecl_well )

add_test( well_lgr_load1  ${EXECUTABLE_OUTPUT_PATH}/well_lgr_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/0.9.2_LGR/BASE_REF_XY3Z1_T30_WI.X0003) 
add_test( well_lgr_load2  ${EXECUTABLE_OUTPUT_PATH}/well_lgr_load ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/AmalgLGRcase/TESTCASE_AMALG_LGR.X0016) 

set_property( TEST well_lgr_load1 PROPERTY LABELS Statoil )
set_property( TEST well_lgr_load2 PROPERTY LABELS Statoil )
set_property( TEST well_dualp    PROPERTY LABELS Statoil )
set_property( TEST well_ts PROPERTY LABELS Statoil )
set_property( TEST well_dualp PROPERTY LABELS Statoil )
set_property( TEST well_conn_load1 PROPERTY LABELS Statoil )
set_property( TEST well_conn_load2 PROPERTY LABELS Statoil )
set_property( TEST well_conn_load3 PROPERTY LABELS Statoil )
set_property( TEST well_conn_load4 PROPERTY LABELS Statoil )
set_property( TEST well_conn_load5 PROPERTY LABELS Statoil )
set_property( TEST well_conn_load6 PROPERTY LABELS Statoil )
set_property( TEST well_info PROPERTY LABELS Statoil )
set_property( TEST well_segment_load PROPERTY LABELS Statoil )
set_property( TEST well_segment_conn_load PROPERTY LABELS Statoil )
