#!/usr/bin/env python
import datetime
import os.path
import pandas as pd
import numpy as np

from cwrap import CFILE
from cwrap import Prototype, load, open as copen

from ecl.eclfile import EclFile, FortIO, openFortIO, openEclFile, EclKW
from ecl.summary import EclSum, EclSumKeyWordVector
from ecl.util.test import TestAreaContext
from tests import EclTest, equinor_test


def test_write_repr():
    """repr(EclSum) used to segfault when there is only a startdate"""
    writer = EclSum.writer("TEST", datetime.date(2000, 2, 3), 10, 10, 10)
    assert repr(writer).startswith(
        'EclSum(name="writer", time=[2000-02-03 00:00:00, 2000-02-03 00:00:00], keys=0) at 0x'
    )


def test_that_year2263_through_pandas_works():
    """Date 2262-04-11 is the upper limit for nanosecond 64-bit
    timestamps, which is what pandas<2 prefers.
    """
    eclsum = EclSum.from_pandas(
        "TESTCASE",
        pd.DataFrame(
            [
                {"DATE": datetime.date(2000, 1, 1), "FPR": 200},
                {"DATE": datetime.date(2263, 1, 1), "FPR": 1},
            ]
        ).set_index("DATE"),
    )
    assert eclsum.numpy_dates[0] == np.datetime64("2000-01-01T00:00:00.000")

    # (This is hit by a round-off error due to 32-bit floats):
    assert eclsum.numpy_dates[1] == np.datetime64("2262-12-31T23:57:52.000")

    # Roundtrip:
    assert eclsum.pandas_frame().index[1] == datetime.datetime(2262, 12, 31, 23, 57, 52)


@equinor_test()
class EclSumTest(EclTest):
    def setUp(self):
        self.test_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.SMSPEC")
        self.ecl_sum = EclSum(self.test_file)

    def test_time_range_year(self):
        real_range = self.ecl_sum.timeRange(interval="1y", extend_end=False)
        extended_range = self.ecl_sum.timeRange(interval="1y", extend_end=True)
        assert real_range[-1] < extended_range[-1]

    def test_time_range_day(self):
        real_range = self.ecl_sum.timeRange(interval="1d", extend_end=False)
        extended_range = self.ecl_sum.timeRange(interval="1d", extend_end=True)
        assert real_range[-1] == extended_range[-1]

    def test_time_range_month(self):
        real_range = self.ecl_sum.timeRange(interval="1m", extend_end=False)
        extended_range = self.ecl_sum.timeRange(interval="1m", extend_end=True)
        assert real_range[-1] < extended_range[-1]

    def test_dump_csv_line(self):
        ecl_sum_vector = EclSumKeyWordVector(self.ecl_sum)
        ecl_sum_vector.addKeywords("F*")

        with self.assertRaises(KeyError):
            ecl_sum_vector.addKeyword("MISSING")

        dtime = datetime.datetime(2002, 1, 1, 0, 0, 0)
        with TestAreaContext("EclSum/csv_dump"):
            test_file_name = self.createTestPath("dump.csv")
            outputH = copen(test_file_name, "w")
            self.ecl_sum.dumpCSVLine(dtime, ecl_sum_vector, outputH)
            assert os.path.isfile(test_file_name)

    def test_truncated_smspec(self):
        with TestAreaContext("EclSum/truncated_smspec") as ta:
            ta.copy_file(self.test_file)
            ta.copy_file(self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"))

            file_size = os.path.getsize("ECLIPSE.SMSPEC")
            with open("ECLIPSE.SMSPEC", "r+") as f:
                f.truncate(file_size / 2)

            with self.assertRaises(IOError):
                EclSum("ECLIPSE")

    def test_truncated_data(self):
        with TestAreaContext("EclSum/truncated_data") as ta:
            ta.copy_file(self.test_file)
            ta.copy_file(self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"))

            file_size = os.path.getsize("ECLIPSE.UNSMRY")
            with open("ECLIPSE.UNSMRY", "r+") as f:
                f.truncate(file_size / 2)

            with self.assertRaises(IOError):
                EclSum("ECLIPSE")

    def test_missing_smspec_keyword(self):
        with TestAreaContext("EclSum/truncated_data") as ta:
            ta.copy_file(self.test_file)
            ta.copy_file(self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"))

            with openEclFile("ECLIPSE.SMSPEC") as f:
                kw_list = []
                for kw in f:
                    kw_list.append(EclKW.copy(kw))

            with openFortIO("ECLIPSE.SMSPEC", mode=FortIO.WRITE_MODE) as f:
                for kw in kw_list:
                    if kw.getName() == "KEYWORDS":
                        continue
                    kw.fwrite(f)

            with self.assertRaises(IOError):
                EclSum("ECLIPSE")

    def test_missing_unsmry_keyword(self):
        with TestAreaContext("EclSum/truncated_data") as ta:
            ta.copy_file(self.test_file)
            ta.copy_file(self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"))

            with openEclFile("ECLIPSE.UNSMRY") as f:
                kw_list = []
                for kw in f:
                    kw_list.append(EclKW.copy(kw))

            with openFortIO("ECLIPSE.UNSMRY", mode=FortIO.WRITE_MODE) as f:
                c = 0
                for kw in kw_list:
                    if kw.getName() == "PARAMS":
                        if c % 5 == 0:
                            continue
                    c += 1
                    kw.fwrite(f)

            with self.assertRaises(IOError):
                EclSum("ECLIPSE")

    def test_labscale(self):
        case = self.createTestPath("Equinor/ECLIPSE/LabScale/HDMODEL")
        sum = EclSum(case, lazy_load=True)
        self.assertEqual(sum.getStartTime(), datetime.datetime(2013, 1, 1, 0, 0, 0))
        self.assertEqual(sum.getEndTime(), datetime.datetime(2013, 1, 1, 19, 30, 0))
        self.assertFloatEqual(sum.getSimulationLength(), 19.50)

        sum = EclSum(case, lazy_load=False)
        self.assertEqual(sum.getStartTime(), datetime.datetime(2013, 1, 1, 0, 0, 0))
        self.assertEqual(sum.getEndTime(), datetime.datetime(2013, 1, 1, 19, 30, 0))
        self.assertFloatEqual(sum.getSimulationLength(), 19.50)
