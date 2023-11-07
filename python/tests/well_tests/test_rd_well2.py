import datetime
import os.path

from resdata import FileMode
from resdata.resfile import ResdataFile
from resdata.grid import Grid
from tests import ResdataTest, equinor_test
from resdata.util.util.ctime import CTime
from resdata.well import (
    WellInfo,
    WellConnection,
    WellType,
    WellConnectionDirection,
    WellSegment,
)


@equinor_test()
class ResdataWellTest2(ResdataTest):
    grid = None

    def getGrid(self):
        if ResdataWellTest2.grid is None:
            ResdataWellTest2.grid = Grid(
                self.createTestPath(
                    "Equinor/ECLIPSE/Troll/Ref2014/T07-4A-W2014-06.EGRID"
                )
            )

        return ResdataWellTest2.grid

    def checkWell(self, rst_file):
        segment_length = [
            2660,
            20,
            121,
            1347.916,
            20.585,
            56.249,
            115.503,
            106.978,
            47.124,
            279.529,
            128.534,
            165.33,
            59.97,
            936.719,
        ]

        well_info = WellInfo(
            self.getGrid(),
            self.createTestPath(
                os.path.join("Equinor/ECLIPSE/Troll/Ref2014", rst_file)
            ),
        )
        well_time_line = well_info["F4BYH"]
        for well_state in well_time_line:
            self.assertTrue(well_state.isMultiSegmentWell())
            self.assertTrue(well_state.hasSegmentData())

            for index, length in enumerate(segment_length):
                segment = well_state.igetSegment(index)
                self.assertFloatEqual(segment.length(), length)

    def testWell(self):
        self.checkWell("T07-4A-W2014-06.X0695")
        self.checkWell("T07-4A-W2014-06.X0709")
        self.checkWell("T07-4A-W2014-06.UNRST")
