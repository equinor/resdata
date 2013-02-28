add_executable( well_ts well_ts.c )
target_link_libraries( well_ts ecl_well )
add_test( well_ts  ${EXECUTABLE_OUTPUT_PATH}/well_ts ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/CO2case/BASE_CASE )
set_property( TEST well_ts PROPERTY LABELS Statoil )


add_executable( well_dualp well_dualp.c )
target_link_libraries( well_dualp ecl_well )
add_test( well_dualp  ${EXECUTABLE_OUTPUT_PATH}/well_dualp ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/Gurbat/ECLIPSE.UNRST 
                                                            ${PROJECT_SOURCE_DIR}/test-data/Statoil/ECLIPSE/DualPoro/DUALPORO.X0005  )
set_property( TEST well_dualp PROPERTY LABELS Statoil )
