import datetime
import os.path

from tests import ResdataTest, equinor_test, source_root
from resdata import ResDataType
from resdata.resfile import (
    ResdataKW,
    ResdataRestartFile,
)
from resdata.resfile.rd_restart_file import ResdataRestartHead
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


def test_restart_headers():
    case_path = os.path.join(
        source_root(), "test-data", "local", "ECLIPSE", "simple", "SIMPLE"
    )
    g = Grid(case_path + ".EGRID")
    f = ResdataRestartFile(g, case_path + ".UNRST")

    headers = f.headers()
    assert len(headers) == 4
    header = headers[0]
    assert header.get_report_step() == 1
    assert header.get_sim_date() == datetime.datetime(2017, 1, 16, 0, 0)
    assert header.get_sim_days() == 15.0
    assert header.well_details() == {"NCWMAX": 0, "NXCONZ": 58}


def test_restart_headers_from_kw():
    intehead = ResdataKW("INTEHEAD", 100, ResDataType.RD_INT)
    doubhead = ResdataKW("DOUBHEAD", 100, ResDataType.RD_DOUBLE)
    logihead = ResdataKW("DOUBHEAD", 100, ResDataType.RD_BOOL)
    header = ResdataRestartHead(kw_arg=(1, intehead, doubhead, logihead))
    assert header.get_report_step() == 1
