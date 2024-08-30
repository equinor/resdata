import datetime
from tests import ResdataTest
from resdata.grid import GridGenerator
from resdata.resfile import ResdataFile
from resdata.well import (
    WellInfo,
    WellType,
    WellConnectionDirection,
)


class ResdataWellICONTest(ResdataTest):
    def setUp(self):
        self.grid = GridGenerator.create_rectangular((46, 112, 22), (1, 1, 1))
        self.rst_file_ICON0 = self.createTestPath(
            "local/ECLIPSE/well/missing-ICON/ICON0.X0027"
        )
        self.rst_file_ICON1 = self.createTestPath(
            "local/ECLIPSE/well/missing-ICON/ICON1.X0027"
        )

    def check_connections(self, well_info, expected):
        well = well_info["B-2H"]
        well_state = well[0]
        self.assertEqual(well_state.hasGlobalConnections(), expected)
        if not expected:
            self.assertEqual(well_state.globalConnections(), [])
        else:
            self.assertGreater(len(well_state.globalConnections()), 0)
        self.assertEqual(well_state.wellType(), WellType.PRODUCER)
        self.assertEqual(well_state.name(), "B-2H")
        self.assertTrue(well_state.isOpen())
        self.assertEqual(well_state.wellNumber(), 1)
        self.assertEqual(well_state.reportNumber(), 27)
        self.assertEqual(
            well_state.simulationTime().datetime(),
            datetime.datetime(1998, 10, 13, 0, 0),
        )
        self.assertEqual(len(well_state), 0)
        self.assertEqual(well_state.numSegments(), 0)
        self.assertEqual(well_state.segments(), [])
        self.assertFalse(well_state.isMultiSegmentWell())
        self.assertFalse(well_state.hasSegmentData())
        self.assertEqual(well_state.gasRate(), 0)
        self.assertEqual(well_state.waterRate(), 0)
        self.assertEqual(well_state.oilRate(), 0)
        self.assertEqual(well_state.volumeRate(), 0)
        self.assertEqual(well_state.gasRateSI(), 0)
        self.assertEqual(well_state.oilRateSI(), 0)
        self.assertEqual(well_state.waterRateSI(), 0)
        self.assertEqual(well_state.volumeRateSI(), 0)

        well_connection = well_state.wellHead()
        self.assertEqual(well_connection.ijk(), (14, 30, 8))
        self.assertTrue(well_connection.isOpen())
        self.assertFalse(well_connection.isMultiSegmentWell())
        self.assertEqual(
            well_connection.direction(), WellConnectionDirection.well_conn_dirX
        )
        self.assertEqual(well_connection.segmentId(), -999)
        self.assertEqual(well_connection.connectionFactor(), -1.0)
        self.assertFalse(well_connection.isFractureConnection())
        self.assertTrue(well_connection.isMatrixConnection())
        self.assertEqual(well_connection, well_connection)
        self.assertEqual(well_connection.gasRate(), 0)
        self.assertEqual(well_connection.waterRate(), 0)
        self.assertEqual(well_connection.oilRate(), 0)
        self.assertEqual(well_connection.volumeRate(), 0)
        self.assertEqual(well_connection.gasRateSI(), 0)
        self.assertEqual(well_connection.oilRateSI(), 0)
        self.assertEqual(well_connection.waterRateSI(), 0)
        self.assertEqual(well_connection.volumeRateSI(), 0)

    def test_missing_icon(self):
        well_info_ICON0 = WellInfo(self.grid, self.rst_file_ICON0)
        well_info_ICON1 = WellInfo(self.grid, self.rst_file_ICON1)
        assert len(well_info_ICON0) == 8

        time_line = well_info_ICON0[0]
        self.assertEqual(time_line.getName(), "C-4H")
        self.assertIn("C-4H", well_info_ICON0.allWellNames())

        self.check_connections(well_info_ICON0, False)
        self.check_connections(well_info_ICON1, True)

        well_info_ICON0.addWellFile(
            ResdataFile(self.rst_file_ICON0), load_segment_information=False
        )
