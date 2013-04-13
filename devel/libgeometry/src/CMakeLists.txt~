set( source_files geo_surface.c geo_util.c geo_pointset.c geo_region.c geo_polygon.c)
set( header_files geo_surface.h geo_util.h geo_pointset.h geo_region.h geo_polygon.h)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${libert_util_build_path} )
include_directories( ${libert_util_src_path} )

add_library( geometry ${LIBRARY_TYPE} ${source_files} )
target_link_libraries( geometry ert_util )


#-----------------------------------------------------------------
install(TARGETS geometry DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
foreach(header ${header_files})
    install(FILES ../include/ert/geometry/${header} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ert/geometry)
endforeach()


