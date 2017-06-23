project(libecl-python)

if (NOT BUILD_PYTHON)
    return ()
endif ()

if (NOT ${BUILD_SHARED_LIBS})
    message(FATAL_ERROR "The Python wrappers require shared libraries")
endif()

find_package(PythonInterp REQUIRED 2.7)
include(FindPythonModule)
include(PythonPackage)

python_module(numpy REQUIRED 1.7)
if (NOT DEFINED PY_numpy)
   message(WARNING "numpy module not found - Python wrappers not enabled")
   set( BUILD_PYTHON OFF PARENT_SCOPE )
   return()
endif()

configure_file(python/ecl/ecl_lib_info_pip.py.in     ecl/__ecl_pip.py)
configure_file(python/ecl/ecl_lib_info_build.py.in   ecl_lib_info_build.py)
configure_file(python/ecl/ecl_lib_info_install.py.in ecl_lib_info_install.py)
configure_file(python/test_env.py.in                 test_env.py)

add_python_package(pyecl ecl
    SOURCES python/ecl/__init__.py
    VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR}.${ERT_VERSION_MICRO}
    TARGETS ecl
    DEPEND_DIRS ecl ${CMAKE_SOURCE_DIR}/lib
    NO_LINK_FLAGS
    TARGET_COPYONLY
)

add_python_package(pyecl ecl SUBDIR ecl
    APPEND
    SOURCES python/ecl/ecl/__init__.py
            python/ecl/ecl/ecl_3d_file.py
            python/ecl/ecl/ecl_3dkw.py
            python/ecl/ecl/ecl_file.py
            python/ecl/ecl/ecl_file_view.py
            python/ecl/ecl/ecl_grav.py
            python/ecl/ecl/ecl_grav_calc.py
            python/ecl/ecl/ecl_grid.py
            python/ecl/ecl/ecl_init_file.py
            python/ecl/ecl/ecl_kw.py
            python/ecl/ecl/ecl_npv.py
            python/ecl/ecl/ecl_region.py
            python/ecl/ecl/ecl_restart_file.py
            python/ecl/ecl/ecl_rft.py
            python/ecl/ecl/ecl_rft_cell.py
            python/ecl/ecl/ecl_smspec_node.py
            python/ecl/ecl/ecl_subsidence.py
            python/ecl/ecl/ecl_sum.py
            python/ecl/ecl/ecl_sum_keyword_vector.py
            python/ecl/ecl/ecl_sum_node.py
            python/ecl/ecl/ecl_sum_tstep.py
            python/ecl/ecl/ecl_sum_vector.py
            python/ecl/ecl/ecl_util.py
            python/ecl/ecl/fortio.py
            python/ecl/ecl/ecl_sum_keyword_vector.py
            python/ecl/ecl/ecl_cmp.py
            python/ecl/ecl/ecl_sum_var_type.py
            python/ecl/ecl/ecl_type.py
            python/ecl/ecl/ecl_grid_generator.py
)

add_python_package(pyecl ecl SUBDIR ecl/faults
    APPEND
    SOURCES python/ecl/ecl/faults/__init__.py
            python/ecl/ecl/faults/fault_block.py
            python/ecl/ecl/faults/fault_block_layer.py
            python/ecl/ecl/faults/fault_collection.py
            python/ecl/ecl/faults/fault.py
            python/ecl/ecl/faults/fault_line.py
            python/ecl/ecl/faults/fault_segments.py
            python/ecl/ecl/faults/layer.py
)

add_python_package(pyecl ecl SUBDIR ecl/rft
    APPEND
    SOURCES python/ecl/ecl/rft/__init__.py
            python/ecl/ecl/rft/well_trajectory.py
)

add_python_package(pyecl ecl SUBDIR geo
    APPEND
    SOURCES python/ecl/geo/__init__.py
            python/ecl/geo/cpolyline.py
            python/ecl/geo/cpolyline_collection.py
            python/ecl/geo/geometry_tools.py
            python/ecl/geo/geo_pointset.py
            python/ecl/geo/geo_region.py
            python/ecl/geo/polyline.py
            python/ecl/geo/xyz_io.py
            python/ecl/geo/surface.py
)

add_python_package(pyecl ecl SUBDIR util
    APPEND
    SOURCES python/ecl/util/__init__.py
            python/ecl/util/bool_vector.py
            python/ecl/util/buffer.py
            python/ecl/util/ctime.py
            python/ecl/util/double_vector.py
            python/ecl/util/hash.py
            python/ecl/util/int_vector.py
            python/ecl/util/install_abort_signals.py
            python/ecl/util/log.py
            python/ecl/util/lookup_table.py
            python/ecl/util/matrix.py
            python/ecl/util/profiler.py
            python/ecl/util/rng.py
            python/ecl/util/stat.py
            python/ecl/util/stringlist.py
            python/ecl/util/substitution_list.py
            python/ecl/util/thread_pool.py
            python/ecl/util/cthread_pool.py
            python/ecl/util/time_vector.py
            python/ecl/util/ui_return.py
            python/ecl/util/util_func.py
            python/ecl/util/vector_template.py
            python/ecl/util/permutation_vector.py
            python/ecl/util/version.py
            python/ecl/util/arg_pack.py
            python/ecl/util/path_format.py
)

add_python_package(pyecl ecl SUBDIR util/enums
    APPEND
    SOURCES python/ecl/util/enums/__init__.py
            python/ecl/util/enums/rng_alg_type_enum.py
            python/ecl/util/enums/rng_init_mode_enum.py
            python/ecl/util/enums/ui_return_status_enum.py
            python/ecl/util/enums/llsq_result_enum.py
)

add_python_package(pyecl ecl SUBDIR well
    APPEND
    SOURCES python/ecl/well/__init__.py
            python/ecl/well/well_connection.py
            python/ecl/well/well_connection_direction_enum.py
            python/ecl/well/well_info.py
            python/ecl/well/well_segment.py
            python/ecl/well/well_state.py
            python/ecl/well/well_time_line.py
            python/ecl/well/well_type_enum.py
)

file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ecl/__ecl_lib_info.py
    INPUT  ${CMAKE_CURRENT_BINARY_DIR}/ecl_lib_info_build.py
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ecl_lib_info_install.py
        DESTINATION ${PYTHON_INSTALL_PREFIX}/ecl
        RENAME __ecl_lib_info.py)

add_setup_py(pyecl setup.py.in)

# add a separate package for the legacy bindings

if (INSTALL_ERT_LEGACY)
    add_python_package(pyert_legacy ert SOURCES python/legacy/ert/__init__.py)
    add_python_package(pyert_legacy ert SUBDIR ecl
        APPEND SOURCES python/legacy/ert/ecl/__init__.py)
    add_python_package(pyert_legacy ert SUBDIR ecl/faults
        APPEND SOURCES python/legacy/ert/ecl/faults/__init__.py)
    add_python_package(pyert_legacy ert SUBDIR geo
        APPEND SOURCES python/legacy/ert/geo/__init__.py)
    add_python_package(pyert_legacy ert SUBDIR util
        APPEND SOURCES python/legacy/ert/util/__init__.py)
    add_python_package(pyert_legacy ert SUBDIR well
        APPEND SOURCES python/legacy/ert/well/__init__.py)

    add_python_package(pyert_legacy ert SUBDIR test
        APPEND SOURCES python/legacy/ert/test/__init__.py)
    if (BUILD_TESTS)
        add_python_test(tests.legacy.test_ecl.ErtLegacyEclTest   tests/legacy/test_ecl.py)
        add_python_test(tests.legacy.test_geo.ErtLegacyGeoTest   tests/legacy/test_geo.py)
        add_python_test(tests.legacy.test_well.ErtLegacyWellTest tests/legacy/test_test.py)
        add_python_test(tests.legacy.test_util.ErtLegacyUtilTest tests/legacy/test_util.py)
        add_python_test(tests.legacy.test_test.ErtLegacyTestTest tests/legacy/test_well.py)
    endif ()
endif ()

add_python_package(testpyecl ecl SUBDIR test
    APPEND
    SOURCES python/ecl/test/__init__.py
            python/ecl/test/ert_test_context.py
            python/ecl/test/ert_test_runner.py
            python/ecl/test/extended_testcase.py
            python/ecl/test/test_run.py
            python/ecl/test/source_enumerator.py
            python/ecl/test/test_area.py
            python/ecl/test/temp_area.py
            python/ecl/test/path_context.py
            python/ecl/test/lint_test_case.py
            python/ecl/test/import_test_case.py
)

add_python_package(testpyecl ecl SUBDIR test/ecl_mock
    APPEND
    SOURCES python/ecl/test/ecl_mock/__init__.py
            python/ecl/test/ecl_mock/ecl_sum_mock.py
)

#----------------------------------------------------------

configure_file(tests/test_install.in ${CMAKE_BINARY_DIR}/bin/test_install @ONLY)

if (NOT BUILD_TESTS)
    return ()
endif()

set(NFS_RUNPATH "" CACHE
    STRING  "Disk area which is shared among cluster nodes and can be used as CWD for LSF/RSH jobs.")
set(RSH_SERVERS "" CACHE
    STRING  "List of nodes which will be used to test the RSH driver")

add_python_test(tests.ecl.test_fk_user_data.FKTest              tests/ecl/test_fk_user_data.py)
add_python_test(tests.ecl.test_grid.GridTest                    tests/ecl/test_grid.py)
add_python_test(tests.ecl.test_grid_generator.GridGeneratorTest tests/ecl/test_grid_generator.py)
add_python_test(tests.ecl.test_ecl_kw.KWTest                    tests/ecl/test_ecl_kw.py)
add_python_test(tests.ecl.test_kw_function.KWFunctionTest       tests/ecl/test_kw_function.py)
add_python_test(tests.ecl.test_ecl_3dkw.Ecl3DKWTest             tests/ecl/test_ecl_3dkw.py)
add_python_test(tests.ecl.test_rft.RFTTest                      tests/ecl/test_rft.py)
add_python_test(tests.ecl.test_rft_cell.RFTCellTest             tests/ecl/test_rft_cell.py)
add_python_test(tests.ecl.test_sum.SumTest                      tests/ecl/test_sum.py)
add_python_test(tests.ecl.test_layer.LayerTest                  tests/ecl/test_layer.py)
add_python_test(tests.ecl.test_faults.FaultTest                 tests/ecl/test_faults.py)
add_python_test(tests.ecl.test_fault_blocks.FaultBlockTest      tests/ecl/test_fault_blocks.py)
add_python_test(tests.ecl.test_deprecation.Deprecation_1_9_Test tests/ecl/test_deprecation.py)
add_python_test(tests.ecl.test_deprecation.Deprecation_2_0_Test tests/ecl/test_deprecation.py)
add_python_test(tests.ecl.test_deprecation.Deprecation_2_1_Test tests/ecl/test_deprecation.py)
add_python_test(tests.ecl.test_removed.Removed_2_1_Test         tests/ecl/test_removed.py)
add_python_test(tests.ecl.test_ecl_util.EclUtilTest             tests/ecl/test_ecl_util.py)
add_python_test(tests.ecl.test_fortio.FortIOTest                tests/ecl/test_fortio.py)
add_python_test(tests.ecl.test_ecl_file.EclFileTest             tests/ecl/test_ecl_file.py)
add_python_test(tests.ecl.test_grav.EclGravTest                 tests/ecl/test_grav.py)
add_python_test(tests.ecl.test_geertsma.GeertsmaTest            tests/ecl/test_geertsma.py)
add_python_test(tests.ecl.test_ecl_type.EclDataTypeTest         tests/ecl/test_ecl_type.py)
add_python_test(tests.ecl.test_region.RegionTest                tests/ecl/test_region.py)

add_python_test(tests.util.test_ctime.CTimeTest                 tests/util/test_ctime.py)
add_python_test(tests.util.test_cstring.CStringTest             tests/util/test_cstring.py)
add_python_test(tests.util.test_hash.HashTest                   tests/util/test_hash.py)
add_python_test(tests.util.test_lookup_table.LookupTableTest    tests/util/test_lookup_table.py)
add_python_test(tests.util.test_matrix.MatrixTest               tests/util/test_matrix.py)
add_python_test(tests.util.test_rng.RngTest                     tests/util/test_rng.py)
add_python_test(tests.util.test_stat.StatTest                   tests/util/test_stat.py)
add_python_test(tests.util.test_string_list.StringListTest      tests/util/test_string_list.py)
add_python_test(tests.util.test_vectors.UtilTest                tests/util/test_vectors.py)
add_python_test(tests.util.test_ui_return.UIReturnTest          tests/util/test_ui_return.py)
add_python_test(tests.util.test_work_area.WorkAreaTest          tests/util/test_work_area.py)
add_python_test(tests.util.test_version.VersionTest             tests/util/test_version.py)
add_python_test(tests.util.test_path_context.PathContextTest    tests/util/test_path_context.py)
add_python_test(tests.util.test_thread_pool.ThreadPoolTest      tests/util/test_thread_pool.py)
add_python_test(tests.util.test_cthread_pool.CThreadPoolTest    tests/util/test_cthread_pool.py)
add_python_test(tests.util.test_arg_pack.ArgPackTest            tests/util/test_arg_pack.py)
add_python_test(tests.util.test_spawn.SpawnTest                 tests/util/test_spawn.py)

add_python_test(tests.geometry.test_geo_pointset.GeoPointsetTest                    tests/geometry/test_geo_pointset.py)
add_python_test(tests.geometry.test_geo_region.GeoRegionTest                        tests/geometry/test_geo_region.py)
add_python_test(tests.geometry.test_surface.SurfaceTest                             tests/geometry/test_surface.py)
add_python_test(tests.geometry.test_polyline.PolylineTest                           tests/geometry/test_polyline.py)
add_python_test(tests.geometry.test_intersection.IntersectionTest                   tests/geometry/test_intersection.py)
add_python_test(tests.geometry.test_convex_hull.ConvexHullTest                      tests/geometry/test_convex_hull.py)
add_python_test(tests.geometry.test_point_in_polygon.PointInPolygonTest             tests/geometry/test_point_in_polygon.py)
add_python_test(tests.geometry.test_polygon_slicing.PolygonSlicingTest              tests/geometry/test_polygon_slicing.py)
add_python_test(tests.geometry.test_cpolyline.CPolylineTest                         tests/geometry/test_cpolyline.py)
add_python_test(tests.geometry.test_cpolyline_collection.CPolylineCollectionTest    tests/geometry/test_cpolyline_collection.py)
add_python_test(tests.geometry.test_geometry_tools.GeometryToolsTest                tests/geometry/test_geometry_tools.py)

add_python_test(tests.global.test_import.ImportEcl tests/global/test_import.py)
add_python_test(tests.global.test_pylint.LintErt tests/global/test_pylint.py)

if (NOT STATOIL_TESTDATA_ROOT)
    return ()
endif ()

add_python_test(tests.ecl.test_ecl_file_statoil.EclFileStatoilTest  tests/ecl/test_ecl_file_statoil.py)
add_python_test(tests.ecl.test_grdecl.GRDECLTest                    tests/ecl/test_grdecl.py)
add_python_test(tests.ecl.test_grid_statoil.GridTest                tests/ecl/test_grid_statoil.py)
add_python_test(tests.ecl.test_ecl_kw_statoil.KWTest                tests/ecl/test_ecl_kw_statoil.py)
add_python_test(tests.ecl.test_ecl_init_file.InitFileTest           tests/ecl/test_ecl_init_file.py)
add_python_test(tests.ecl.test_ecl_restart_file.RestartFileTest     tests/ecl/test_ecl_restart_file.py)
add_python_test(tests.ecl.test_restart.RestartTest                  tests/ecl/test_restart.py)
add_python_test(tests.ecl.test_region_statoil.RegionTest            tests/ecl/test_region_statoil.py)
add_python_test(tests.ecl.test_rft_statoil.RFTTest                  tests/ecl/test_rft_statoil.py)
add_python_test(tests.ecl.test_sum_statoil.SumTest                  tests/ecl/test_sum_statoil.py)
add_python_test(tests.ecl.test_ecl_sum_vector.EclSumVectorTest      tests/ecl/test_ecl_sum_vector.py)
add_python_test(tests.ecl.test_ecl_sum.EclSumTest                   tests/ecl/test_ecl_sum.py)
add_python_test(tests.ecl.test_statoil_faults.StatoilFaultTest      tests/ecl/test_statoil_faults.py)
add_python_test(tests.ecl.test_fault_blocks_statoil.FaultBlockTest  tests/ecl/test_fault_blocks_statoil.py)
add_python_test(tests.ecl.test_npv.NPVTest                          tests/ecl/test_npv.py)
add_python_test(tests.ecl.test_indexed_read.EclIndexedReadTest      tests/ecl/test_indexed_read.py)
add_python_test(tests.ecl.test_ecl_cmp.EclCmpTest                   tests/ecl/test_ecl_cmp.py)
add_python_test(tests.ecl.test_restart_head.RestartHeadTest         tests/ecl/test_restart_head.py)

add_python_test(tests.test_ecl_well.EclWellTest      tests/well/test_ecl_well.py)
add_python_test(tests.test_ecl_well2.EclWellTest2    tests/well/test_ecl_well2.py)
add_python_test(tests.test_ecl_well3.EclWellTest3    tests/well/test_ecl_well3.py)
