link_directories( ${ERT_BINARY_DIR}/libecl/src )

include_directories( ${libutil_build_path} )
include_directories( ${libutil_src_path} )
include_directories( ${libecl_src_path} )

add_executable( grid_info.x grid_info.c )
add_executable( summary.x view_summary.c )
target_link_libraries( grid_info.x ecl_static util_static )
target_link_libraries( summary.x ecl_static util_static )
