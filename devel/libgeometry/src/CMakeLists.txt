set( source_files geo_surface.c geo_util.c geo_pointset.c geo_region.c geo_polygon.c)
set( header_files geo_surface.h geo_util.h geo_pointset.h geo_region.h geo_polygon.h)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${libutil_build_path} )
include_directories( ${libutil_src_path} )

add_library( geometry_static  STATIC ${source_files} )
add_library( geometry_shared  SHARED ${source_files} )

set_target_properties( geometry_static PROPERTIES OUTPUT_NAME geometry )
set_target_properties( geometry_shared PROPERTIES OUTPUT_NAME geometry )

#-----------------------------------------------------------------
if (GEOMETRY_INSTALL_PREFIX)
   install(TARGETS geometry_static geometry_shared DESTINATION ${INSTALL_ROOT}/${GEOMETRY_INSTALL_PREFIX}/lib)
   install(FILES ${header_files} DESTINATION ${INSTALL_ROOT}/${GEOMETRY_INSTALL_PREFIX}/include)
else()
   install(TARGETS geometry_static geometry_shared DESTINATION ${INSTALL_ROOT}/lib)
   install(FILES ${header_files} DESTINATION ${INSTALL_ROOT}/include)
endif()


