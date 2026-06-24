import datetime
import os.path
import time

import hypothesis.strategies as st
import pytest
from hypothesis import given
from resdata import ResDataType
from resdata.grid import Grid
from resdata.resfile import (
    ResdataKW,
    ResdataRestartFile,
)
from resdata.resfile.rd_restart_file import ResdataRestartHead

from tests import ResdataTest, equinor_test, source_root


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
    logihead = ResdataKW("LOGIHEAD", 100, ResDataType.RD_BOOL)
    header = ResdataRestartHead(kw_arg=(1, intehead, doubhead, logihead))
    assert header.get_report_step() == 1


def restart_interpreted_date(year: int, month: int, day: int) -> datetime.datetime:
    intehead = ResdataKW("INTEHEAD", 100, ResDataType.RD_INT)
    intehead[64] = day
    intehead[65] = month
    intehead[66] = year
    doubhead = ResdataKW("DOUBHEAD", 100, ResDataType.RD_DOUBLE)
    logihead = ResdataKW("LOGIHEAD", 100, ResDataType.RD_BOOL)
    header = ResdataRestartHead(kw_arg=(1, intehead, doubhead, logihead))
    return header.get_sim_date()


@given(
    year=st.integers(min_value=1970, max_value=2600),
    month=st.integers(min_value=1, max_value=12),
    day=st.integers(min_value=1, max_value=27),
)
def test_that_date_roundtrips(year, month, day):
    dt = restart_interpreted_date(year, month, day)
    assert dt.year == year
    assert dt.month == month
    assert dt.day == day
    assert dt.hour == 0
    assert dt.second == 0
    assert dt.microsecond == 0


@given(
    year=st.integers(min_value=1970, max_value=2600),
    month=st.integers(min_value=1, max_value=12),
    day=st.integers(min_value=1, max_value=27),
)
def test_that_restart_interpreted_timestamp_has_no_tzinfo(year, month, day):
    assert restart_interpreted_date(year, month, day).tzinfo is None


@given(
    year=st.integers(min_value=1970, max_value=2600),
    month=st.integers(min_value=1, max_value=12),
    day=st.integers(min_value=1, max_value=27),
)
def test_that_restart_interpreted_timestamp_is_not_affected_by_local_timezone(
    year, month, day
):
    try:
        with pytest.MonkeyPatch.context() as monkeypatch:
            monkeypatch.setenv("TZ", "America/New_York")
            time.tzset()
            result_tz_ny = restart_interpreted_date(year, month, day)

            monkeypatch.setenv("TZ", "UTC")
            time.tzset()
            result_tz_utc = restart_interpreted_date(year, month, day)
            assert result_tz_ny == result_tz_utc
    finally:
        time.tzset()
