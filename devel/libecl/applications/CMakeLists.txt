link_directories( ${ERT_BINARY_DIR}/libecl/src )

include_directories( ${libutil_build_path} )
include_directories( ${libutil_src_path} )
include_directories( ${libecl_src_path} )

add_executable( convert.x convert.c )
add_executable( kw_list.x kw_list.c )
add_executable( grid_info.x grid_info.c )
add_executable( summary.x view_summary.c )

set(program_list convert.x kw_list.x grid_info.x summary.x)
foreach(prog ${program_list})
   target_link_libraries( ${prog} ecl_static util_static )
   #-----------------------------------------------------------------
   install(TARGETS ${prog} DESTINATION ${ECL_INSTALL_PREFIX}/bin)
   if (INSTALL_GROUP)
      install(CODE "EXECUTE_PROCESS(COMMAND chgrp ${INSTALL_GROUP} ${CMAKE_INSTALL_PREFIX}/${ECL_INSTALL_PREFIX}/bin/${prog})")
      install(CODE "EXECUTE_PROCESS(COMMAND chmod g+w ${CMAKE_INSTALL_PREFIX}/${ECL_INSTALL_PREFIX}/bin/${prog})")
   endif()
endforeach()
