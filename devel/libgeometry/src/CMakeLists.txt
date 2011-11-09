set( source_files geo_2dsurface.c geo_util.c)
set( header_files geo_2dsurface.h geo_util.h)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${libutil_build_path} )
include_directories( ${libutil_src_path} )

add_library( geometry_static  STATIC ${source_files} )
add_library( geometry_shared  SHARED ${source_files} )

set_target_properties( geometry_static PROPERTIES OUTPUT_NAME geometry )
set_target_properties( geometry_shared PROPERTIES OUTPUT_NAME geometry )

target_link_libraries( geometry_static ${util_static} )
target_link_libraries( geometry_shared ${util_shared} )

#-----------------------------------------------------------------
if (GEOMETRY_INSTALL_PREFIX)
   install(TARGETS geometry_static geometry_shared DESTINATION ${INSTALL_ROOT}/${GEOMETRY_INSTALL_PREFIX}/lib)
   install(FILES ${header_files} DESTINATION ${INSTALL_ROOT}/${GEOMETRY_INSTALL_PREFIX}/include)
else()
   install(TARGETS geometry_static geometry_shared DESTINATION ${INSTALL_ROOT}/lib)
   install(FILES ${header_files} DESTINATION ${INSTALL_ROOT}/include)
endif()


