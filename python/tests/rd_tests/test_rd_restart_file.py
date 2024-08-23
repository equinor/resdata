import datetime

from resdata.grid import Grid
from resdata.resfile import (
    Resdata3DKW,
    ResdataKW,
    ResdataRestartFile,
)

from tests import ResdataTest, equinor_test


@equinor_test()
class RestartFileTest(ResdataTest):
    def setUp(self):
        self.grid_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.EGRID")
        self.unrst_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNRST")
        self.xrst_file0 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.X0000")
        self.xrst_file10 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.X0010")
        self.xrst_file20 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.X0020")

    def test_load(self):
        g = Grid(self.grid_file)
        f = ResdataRestartFile(g, self.unrst_file)

        head = f["INTEHEAD"][0]
        self.assertTrue(isinstance(head, ResdataKW))

        swat = f["SWAT"][0]
        self.assertTrue(isinstance(swat, Resdata3DKW))

        pressure = f["PRESSURE"][0]
        self.assertTrue(isinstance(pressure, Resdata3DKW))

    def test_type(self):
        g = Grid(self.grid_file)
        with self.assertRaises(ValueError):
            _f = ResdataRestartFile(g, "NOT_A_RESTART_FILE")

    def test_unified(self):
        g = Grid(self.grid_file)
        f_unrst = ResdataRestartFile(g, self.unrst_file)
        f_x0 = ResdataRestartFile(g, self.xrst_file0)
        f_x10 = ResdataRestartFile(g, self.xrst_file10)
        f_x20 = ResdataRestartFile(g, self.xrst_file20)

        self.assertTrue(f_unrst.unified())
        self.assertFalse(f_x0.unified())
        self.assertFalse(f_x10.unified())
        self.assertFalse(f_x20.unified())

        self.assertEqual(
            [(10, datetime.datetime(2000, 10, 1, 0, 0, 0), 274.0)], f_x10.timeList()
        )

        unrst_timeList = f_unrst.timeList()
        self.assertEqual(len(unrst_timeList), 63)
        self.assertEqual(
            (62, datetime.datetime(2004, 12, 31, 0, 0, 0), 1826.0), unrst_timeList[62]
        )
