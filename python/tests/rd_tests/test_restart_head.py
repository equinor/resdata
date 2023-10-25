import datetime

from tests import ResdataTest, equinor_test
from resdata import FileMode
from resdata.resfile import (
    Resdata3DKW,
    ResdataKW,
    ResdataRestartFile,
    ResdataFile,
    FortIO,
)
from resdata.grid import Grid


@equinor_test()
class RestartHeadTest(ResdataTest):
    def setUp(self):
        self.grid_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.EGRID")
        self.unrst_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNRST")
        self.xrst_file0 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.X0000")

    def test_headers(self):
        g = Grid(self.grid_file)
        f = ResdataRestartFile(g, self.unrst_file)

        headers = f.headers()
        self.assertEqual(len(headers), 63)

        with self.assertRaises(IndexError):
            f.get_header(1000)

        header = f.get_header(10)
        details = header.well_details()
        self.assertTrue("NXCONZ" in details)
        self.assertTrue("NCWMAX" in details)
