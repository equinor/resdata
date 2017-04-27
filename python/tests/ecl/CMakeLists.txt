set(NFS_RUNPATH "" CACHE STRING  "Disk area which is shared among cluster nodes and can be used as CWD for LSF/RSH jobs.")
set(RSH_SERVERS "" CACHE STRING  "List of nodes which will be used to test the RSH driver")

set(TEST_SOURCES
    __init__.py
    test_deprecation.py
    test_removed.py
    test_ecl_3dkw.py
    test_ecl_file_statoil.py
    test_ecl_file.py
    test_ecl_init_file.py
    test_ecl_restart_file.py
    test_ecl_sum.py
    test_ecl_sum_vector.py
    test_fault_blocks.py
    test_fault_blocks_statoil.py
    test_faults.py
    test_fortio.py
    test_grdecl.py
    test_grid.py
    test_grid_statoil.py
    test_indexed_read.py
    test_ecl_kw_statoil.py
    test_ecl_kw.py
    test_kw_function.py
    test_layer.py
    test_npv.py
    test_region.py
    test_region_statoil.py
    test_restart.py
    test_rft.py
    test_rft_statoil.py
    test_rft_cell.py
    test_statoil_faults.py
    test_sum_statoil.py
    test_ecl_util.py
    test_ecl_cmp.py
    test_sum.py
    test_grav.py
    test_geertsma.py
    test_ecl_type.py
)

add_python_package("python.tests.ecl"  ${PYTHON_INSTALL_PREFIX}/tests/ecl "${TEST_SOURCES}" False)

addPythonTest(ecl.ecl_grid tests.ecl.test_grid.GridTest )
addPythonTest(ecl.ecl_kw tests.ecl.test_ecl_kw.KWTest)
addPythonTest(ecl.ecl_kw_function tests.ecl.test_kw_function.KWFunctionTest)
addPythonTest(ecl.ecl_3dkw tests.ecl.test_ecl_3dkw.Ecl3DKWTest )
addPythonTest(ecl.ecl_rft tests.ecl.test_rft.RFTTest)
addPythonTest(ecl.ecl_rft_cell tests.ecl.test_rft_cell.RFTCellTest)
addPythonTest(ecl.ecl_sum2 tests.ecl.test_sum.SumTest)
addPythonTest(ecl.layer tests.ecl.test_layer.LayerTest )
addPythonTest(ecl.faults tests.ecl.test_faults.FaultTest )
addPythonTest(ecl.fault_blocks tests.ecl.test_fault_blocks.FaultBlockTest )
addPythonTest(ecl.ecl_deprecation1.9 tests.ecl.test_deprecation.Deprecation_1_9_Test )
addPythonTest(ecl.ecl_deprecation2.0 tests.ecl.test_deprecation.Deprecation_2_0_Test )
addPythonTest(ecl.ecl_deprecation2.1 tests.ecl.test_deprecation.Deprecation_2_1_Test )
addPythonTest(ecl.ecl_removed2.1 tests.ecl.test_removed.Removed_2_1_Test )
addPythonTest(ecl.ecl_util tests.ecl.test_ecl_util.EclUtilTest )
addPythonTest(ecl.fortio tests.ecl.test_fortio.FortIOTest)
addPythonTest(ecl.ecl_file tests.ecl.test_ecl_file.EclFileTest)
addPythonTest(ecl.ecl_grav tests.ecl.test_grav.EclGravTest)
addPythonTest(ecl.ecl_geertsma tests.ecl.test_geertsma.GeertsmaTest)
addPythonTest(ecl.ecl_type tests.ecl.test_ecl_type.EclDataTypeTest)
addPythonTest(ecl.ecl_region tests.ecl.test_region.RegionTest)


if (STATOIL_TESTDATA_ROOT)
  addPythonTest(ecl.ecl_file_statoil tests.ecl.test_ecl_file_statoil.EclFileStatoilTest LABELS StatoilData)
  addPythonTest(ecl.ecl_grdecl tests.ecl.test_grdecl.GRDECLTest LABELS StatoilData)
  addPythonTest(ecl.ecl_grid_statoil tests.ecl.test_grid_statoil.GridTest LABELS StatoilData:Slow)
  addPythonTest(ecl.ecl_kw_statoil tests.ecl.test_ecl_kw_statoil.KWTest LABELS StatoilData)
  addPythonTest(ecl.ecl_init_file tests.ecl.test_ecl_init_file.InitFileTest LABELS StatoilData)
  addPythonTest(ecl.ecl_restart_file tests.ecl.test_ecl_restart_file.RestartFileTest LABELS StatoilData)
  addPythonTest(ecl.ecl_restart tests.ecl.test_restart.RestartTest LABELS StatoilData )
  addPythonTest(ecl.ecl_region_statoil tests.ecl.test_region_statoil.RegionTest LABELS StatoilData)
  addPythonTest(ecl.ecl_rft_statoil tests.ecl.test_rft_statoil.RFTTest LABELS StatoilData)
  addPythonTest(ecl.ecl_sum1 tests.ecl.test_sum_statoil.SumTest LABELS StatoilData)
  addPythonTest(ecl.ecl_sum_vector tests.ecl.test_ecl_sum_vector.EclSumVectorTest LABELS StatoilData)
  addPythonTest(ecl.ecl_sum_time_range tests.ecl.test_ecl_sum.EclSumTest LABELS StatoilData)
  addPythonTest(ecl.statoil_faults tests.ecl.test_statoil_faults.StatoilFaultTest LABELS StatoilData)
  addPythonTest(ecl.fault_blocks_statoil tests.ecl.test_fault_blocks_statoil.FaultBlockTest LABELS StatoilData)
  addPythonTest(ecl.ecl_npv tests.ecl.test_npv.NPVTest LABELS StatoilData)
  addPythonTest(ecl.indexed_read tests.ecl.test_indexed_read.EclIndexedReadTest LABELS StatoilData)
  addPythonTest(ecl.ecl_cmp tests.ecl.test_ecl_cmp.EclCmpTest LABELS StatoilData)
endif()
