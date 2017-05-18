project(libecl-geometry)

add_library(ert_geometry src/geo_surface.c
                         src/geo_util.c
                         src/geo_pointset.c
                         src/geo_region.c
                         src/geo_polygon.c
                         src/geo_polygon_collection.c)
target_include_directories(ert_util
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
    )

set_target_properties(ert_geometry PROPERTIES
                        VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR}
                        SOVERSION ${ERT_VERSION_MAJOR})
target_link_libraries(ert_geometry ert_util)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS ert_geometry
        EXPORT  ecl-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (NOT BUILD_TESTS)
    return ()
endif()

foreach (name geo_util_xlines geo_polygon geo_polygon_collection)
    add_executable(${name} tests/${name}.c)
    target_link_libraries(${name} ert_geometry)
    add_test(NAME ${name} COMMAND ${name})
endforeach ()

if (NOT STATOIL_TESTDATA_ROOT)
    return ()
endif ()

add_executable(geo_surface tests/geo_surface.c)
target_link_libraries(geo_surface ert_geometry)
add_test(NAME geo_surface COMMAND geo_surface
        ${_geopath}/Surface.irap
        ${_geopath}/Surface_incompatible.irap)
