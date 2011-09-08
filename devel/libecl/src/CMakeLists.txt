set( source_files ecl_file_kw.c ecl_grav.c ecl_grav_calc.c ecl_smspec.c ecl_sum_data.c ecl_util.c ecl_kw.c ecl_sum.c fortio.c ecl_rft_file.c ecl_rft_node.c ecl_grid.c ecl_box.c ecl_io_config.c ecl_file.c ecl_region.c point.c tetrahedron.c)
set( header_files ecl_file_kw.h ecl_grav.h ecl_grav_calc.h ecl_endian_flip.h ecl_smspec.h ecl_sum_data.h ecl_util.h ecl_kw.h ecl_sum.h fortio.h ecl_rft_file.h ecl_rft_node.h ecl_box.h ecl_grid.h ecl_io_config.h ecl_file.h ecl_region.h)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${libutil_build_path} )
include_directories( ${libutil_src_path} )

add_library( ecl_static  STATIC ${source_files} )
add_library( ecl_shared  SHARED ${source_files} )

set_target_properties( ecl_static PROPERTIES OUTPUT_NAME ecl )
set_target_properties( ecl_shared PROPERTIES OUTPUT_NAME ecl )

target_link_libraries( ecl_static util_static )
target_link_libraries( ecl_shared util_shared )

#-----------------------------------------------------------------
install(TARGETS ecl_static ecl_shared DESTINATION ${ECL_INSTALL_PREFIX}/lib)
install(FILES ${header_files} DESTINATION ${ECL_INSTALL_PREFIX}/include)


