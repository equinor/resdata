#!/usr/bin/env python
from _ctypes import ArgumentError
import os
import datetime
from resdata.resfile import ResdataFile
from tests import ResdataTest, equinor_test


@equinor_test()
class RestartTest(ResdataTest):
    def setUp(self):
        self.xfile0 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.X0000")
        self.u_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNRST")
        self.fmt_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.FUNRST")
        self.grid_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.EGRID")
        self.file_list = []

    def addFile(self, filename):
        self.file_list.append(filename)

    def tearDown(self):
        for f in self.file_list:
            if os.path.exists(f):
                os.unlink(f)

    def report_file_test(self, fname):
        self.assertTrue(ResdataFile.contains_report_step(fname, 4))
        self.assertTrue(ResdataFile.contains_report_step(fname, 0))
        self.assertTrue(ResdataFile.contains_report_step(fname, 62))
        self.assertFalse(ResdataFile.contains_report_step(fname, -1))
        self.assertFalse(ResdataFile.contains_report_step(fname, 100))

        f = ResdataFile(fname)
        self.assertTrue(f.has_report_step(4))
        self.assertTrue(f.has_report_step(0))
        self.assertTrue(f.has_report_step(62))

        self.assertFalse(f.has_report_step(-1))
        self.assertFalse(f.has_report_step(100))

    def test_report(self):
        self.report_file_test(self.u_file)

    def test_date(self):
        f = ResdataFile(self.u_file)
        self.assertTrue(f.has_sim_time(datetime.datetime(2001, 6, 1)))
        self.assertFalse(f.has_sim_time(datetime.datetime(2005, 6, 1)))
        self.assertFalse(f.has_sim_time(datetime.datetime(1999, 6, 1)))
        self.assertFalse(f.has_sim_time(datetime.datetime(2001, 6, 11)))

        self.assertTrue(
            ResdataFile.contains_sim_time(self.u_file, datetime.datetime(2001, 6, 1))
        )
        self.assertFalse(
            ResdataFile.contains_sim_time(self.u_file, datetime.datetime(2005, 6, 1))
        )
        self.assertFalse(
            ResdataFile.contains_sim_time(self.u_file, datetime.datetime(1999, 6, 1))
        )
        self.assertFalse(
            ResdataFile.contains_sim_time(self.u_file, datetime.datetime(2001, 6, 11))
        )

    def report_list_file_test(self, fname, rlist0):
        rlist = ResdataFile.file_report_list(fname)
        self.assertListEqual(rlist, rlist0)

        f = ResdataFile(fname)
        rlist = f.report_list
        self.assertListEqual(rlist, rlist0)

    def test_report_list(self):
        rlist0 = list(range(63))
        self.report_list_file_test(self.u_file, rlist0)

        rlist0 = [0]
        self.report_list_file_test(self.xfile0, rlist0)

    def test_dates(self):
        f = ResdataFile(self.u_file)
        dates = f.dates
        self.assertTrue(len(dates) == 63)

        f = ResdataFile(self.xfile0)
        dates = f.dates
        self.assertTrue(len(dates) == 1)
        self.assertTrue(dates[0] == datetime.datetime(2000, 1, 1))

    def test_name(self):
        f = ResdataFile(self.u_file)
        self.assertTrue(f.getFilename() == self.u_file)

        f = ResdataFile(self.xfile0)
        self.assertTrue(f.getFilename() == self.xfile0)

    def test_kw(self):
        f = ResdataFile(self.u_file)
        kw1 = f["SWAT"][40]
        kw2 = f.restart_get_kw("SWAT", datetime.datetime(2003, 3, 1))
        kw3 = f.restart_get_kw("SWAT", datetime.datetime(2003, 3, 1), copy=True)

        self.assertTrue(kw1.equal(kw2))
        self.assertTrue(kw1.equal(kw3))

        with self.assertRaises(IndexError):
            kw4 = f.restart_get_kw("SWAT", datetime.datetime(2009, 3, 17))
