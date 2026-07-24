import datetime
import itertools
import os.path
import shutil
from dataclasses import dataclass, replace
from textwrap import dedent
from typing import TypeAlias

import hypothesis.strategies as st
import numpy as np
import pandas as pd
import pytest
import resfo
from cwrap import open as copen
from hypothesis import assume, given
from pandas.testing import assert_frame_equal
from resdata.resfile import FortIO, ResdataKW, open_rd_file, openFortIO
from resdata.summary import (
    Summary,
    SummaryKeyWordVector,
    SummaryVarType,
)
from resdata.summary._date2num import date2num
from resdata.util.util import DoubleVector, StringList, TimeVector
from resfo_utilities.testing import (
    Date,
    Simulator,
    Smspec,
    SmspecIntehead,
    SummaryMiniStep,
    SummaryStep,
    UnitSystem,
    Unsmry,
    smspecs,
    summaries,
    summary_variables,
)

from tests import ResdataTest, equinor_test
from tests.util.mock import createSummary


def test_write_repr():
    """repr(Summary) used to segfault when there is only a startdate"""
    writer = Summary.writer("TEST", datetime.date(2000, 2, 3), 10, 10, 10)
    assert repr(writer).startswith(
        'Summary(name="writer", time=[2000-02-03 00:00:00, 2000-02-03 00:00:00], keys=0) at 0x'
    )


def test_that_year2263_through_pandas_works():
    """Date 2262-04-11 is the upper limit for nanosecond 64-bit
    timestamps, which is what pandas<2 prefers.
    """
    eclsum = Summary.from_pandas(
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


@pytest.mark.usefixtures("use_tmpdir")
def test_dump_csv_line():
    case = createSummary(
        "CASE1",
        [("FOPT", None, 0, "SM3"), ("WOPT", "OP1", 0, "SM3")],
    )
    case.fwrite()

    rd_sum = Summary("CASE1")
    kw_vector = SummaryKeyWordVector(rd_sum)
    kw_vector.add_keywords("F*")

    dtime = datetime.datetime(2010, 6, 1, 0, 0, 0)
    out_path = "dump.csv"
    with copen(out_path, "w") as out_handle:
        rd_sum.dump_csv_line(dtime, kw_vector, out_handle)

    assert os.path.isfile(out_path)
    assert os.path.getsize(out_path) > 0


@pytest.mark.parametrize("bad_keywords", [5, None, StringList()])
def test_dump_csv_line_with_wrong_type_keywords_raises_type_error(bad_keywords):
    case = createSummary(
        "CASE2",
        [("FOPT", None, 0, "SM3")],
    )
    case.fwrite()

    rd_sum = Summary("CASE2")
    dtime = datetime.datetime(2010, 6, 1, 0, 0, 0)
    with copen("dump_bad.csv", "w") as out_handle:
        with pytest.raises(TypeError, match="Expected SummaryKeyWordVector"):
            rd_sum.dump_csv_line(dtime, bad_keywords, out_handle)


@equinor_test()
class SummaryTest(ResdataTest):
    def setUp(self):
        self.test_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.SMSPEC")
        self.rd_sum = Summary(self.test_file)

    def test_time_range_year(self):
        real_range = self.rd_sum.time_range(interval="1y", extend_end=False)
        extended_range = self.rd_sum.time_range(interval="1y", extend_end=True)
        assert real_range[-1] < extended_range[-1]

    def test_time_range_day(self):
        real_range = self.rd_sum.time_range(interval="1d", extend_end=False)
        extended_range = self.rd_sum.time_range(interval="1d", extend_end=True)
        assert real_range[-1] == extended_range[-1]

    def test_time_range_month(self):
        real_range = self.rd_sum.time_range(interval="1m", extend_end=False)
        extended_range = self.rd_sum.time_range(interval="1m", extend_end=True)
        assert real_range[-1] < extended_range[-1]

    def test_dump_csv_line(self):
        rd_sum_vector = SummaryKeyWordVector(self.rd_sum)
        rd_sum_vector.add_keywords("F*")

        with self.assertRaises(KeyError):
            rd_sum_vector.add_keyword("MISSING")

        dtime = datetime.datetime(2002, 1, 1, 0, 0, 0)
        tmpdir = self.tmp_path_factory.mktemp("Summary_csv_dump", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            test_file_name = self.createTestPath("dump.csv")
            outputH = copen(test_file_name, "w")
            self.rd_sum.dump_csv_line(dtime, rd_sum_vector, outputH)
            assert os.path.isfile(test_file_name)

    def test_truncated_smspec(self):
        ta = self.tmp_path_factory.mktemp("Summary_truncated_smspec", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(ta)
            shutil.copy(self.test_file, ".")
            shutil.copy(
                self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"), "."
            )

            file_size = os.path.getsize("ECLIPSE.SMSPEC")
            with open("ECLIPSE.SMSPEC", "r+") as f:
                f.truncate(file_size // 2)

            with self.assertRaises(IOError):
                Summary("ECLIPSE")

    def test_truncated_data(self):
        ta = self.tmp_path_factory.mktemp("Summary_truncated_data", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(ta)
            shutil.copy(self.test_file, ".")
            shutil.copy(
                self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"), "."
            )

            file_size = os.path.getsize("ECLIPSE.UNSMRY")
            with open("ECLIPSE.UNSMRY", "r+") as f:
                f.truncate(file_size // 2)

            with self.assertRaises(IOError):
                Summary("ECLIPSE")

    def test_missing_smspec_keyword(self):
        ta = self.tmp_path_factory.mktemp("Summary_truncated_data", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(ta)
            shutil.copy(self.test_file, ".")
            shutil.copy(
                self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"), "."
            )

            with open_rd_file("ECLIPSE.SMSPEC") as f:
                kw_list = []
                for kw in f:
                    kw_list.append(ResdataKW.copy(kw))

            with openFortIO("ECLIPSE.SMSPEC", mode=FortIO.WRITE_MODE) as f:
                for kw in kw_list:
                    if kw.get_name() == "KEYWORDS":
                        continue
                    kw.fwrite(f)

            with self.assertRaises(IOError):
                Summary("ECLIPSE")

    def test_missing_unsmry_keyword(self):
        ta = self.tmp_path_factory.mktemp("Summary_truncated_data", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(ta)
            shutil.copy(self.test_file, ".")
            shutil.copy(
                self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNSMRY"), "."
            )

            with open_rd_file("ECLIPSE.UNSMRY") as f:
                kw_list = []
                for kw in f:
                    kw_list.append(ResdataKW.copy(kw))

            with openFortIO("ECLIPSE.UNSMRY", mode=FortIO.WRITE_MODE) as f:
                c = 0
                for kw in kw_list:
                    if kw.get_name() == "PARAMS":
                        if c % 5 == 0:
                            continue
                    c += 1
                    kw.fwrite(f)

            with self.assertRaises(IOError):
                Summary("ECLIPSE")

    def test_labscale(self):
        case = self.createTestPath("Equinor/ECLIPSE/LabScale/HDMODEL")
        summary = Summary(case, lazy_load=True)
        self.assertEqual(
            summary.get_start_time(), datetime.datetime(2013, 1, 1, 0, 0, 0)
        )
        self.assertEqual(
            summary.get_end_time(), datetime.datetime(2013, 1, 1, 19, 30, 0)
        )
        self.assertFloatEqual(summary.getSimulationLength(), 19.50)

        summary = Summary(case, lazy_load=False)
        self.assertEqual(
            summary.get_start_time(), datetime.datetime(2013, 1, 1, 0, 0, 0)
        )
        self.assertEqual(
            summary.get_end_time(), datetime.datetime(2013, 1, 1, 19, 30, 0)
        )
        self.assertFloatEqual(summary.getSimulationLength(), 19.50)


@given(summaries())
@pytest.mark.usefixtures("use_tmpdir")
def test_to_from_pandas(summary):
    smspec, unsmry = summary
    assume(len(smspec.keywords) == len(set(smspec.keywords)))
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")
    summary = Summary("TEST", lazy_load=False)

    baseline = summary.pandas_frame()
    roundtrip = Summary.from_pandas(
        "TEST", baseline, dims=(smspec.nx, smspec.ny, smspec.nz)
    ).pandas_frame()

    roundtrip.index = roundtrip.index.values.astype(np.int64)
    baseline.index = baseline.index.values.astype(np.int64)

    assert_frame_equal(roundtrip, baseline, check_exact=False, atol=0, rtol=1e-6)


@given(summaries())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_non_matching_dataframe_gives_empty_columns(summary):
    smspec, unsmry = summary
    assume("BOGUS" not in (smspec.keywords))
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")

    assert (
        Summary("TEST", lazy_load=False)
        .pandas_frame(column_keys=["BOGUS"])
        .columns.empty
    )


@given(summaries())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_equal_smspec_nodes_have_equal_hash(summary):
    """The __hash__/__eq__ contract requires that a == b implies
    hash(a) == hash(b) for all smspec_nodes of a summary case."""
    smspec, unsmry = summary
    assume(len(smspec.keywords) == len(set(smspec.keywords)))
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")
    summary = Summary("TEST", lazy_load=False)

    nodes = [summary.smspec_node(key) for key in summary.keys()]
    for node1, node2 in itertools.product(nodes, repeat=2):
        if node1 == node2:
            assert hash(node1) == hash(node2)


@given(summaries())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_smspec_node_comparison_with_other_types_is_consistent(summary):
    """Comparing a smspec_node to an object of another type should never
    consider them equal, and ordering comparisons should raise TypeError,
    matching normal Python semantics for unsupported comparisons."""
    smspec, unsmry = summary
    assume(len(smspec.keywords) == len(set(smspec.keywords)))
    assume(len(smspec.keywords) > 0)
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")
    summary = Summary("TEST", lazy_load=False)

    node = summary.smspec_node(next(iter(summary.keys())))

    assert node != "a_string"
    assert not (node == 5)
    assert node is not None
    assert node != None  # noqa: E711

    with pytest.raises(TypeError):
        node < "a_string"

    with pytest.raises(TypeError):
        node > 5


def create_summary(
    summary_keys=("FOPR",),
    time_units="DAYS",
    case="TEST",
    formatted=False,
    start_date=None,
    times=(0.0, 1.0, 2.0),
    values=None,
):
    """Helper function to create test summary files

    Args:
        summary_keys: List of summary keywords
        time_units: Time units (DAYS, HOURS, etc)
        case: Case name
        formatted: Whether to create formatted files
        start_date: Simulation start date
        times: List of time values
        values: List of value lists for each time
    """
    start_date = start_date or Date(
        day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0
    )
    num_values = len(summary_keys)

    if values is None:
        values = [[100.0 + i * 10.0] * num_values for i in range(len(times))]

    unsmry = Unsmry(
        steps=[
            SummaryStep(
                seqnum=i,
                ministeps=[
                    SummaryMiniStep(mini_step=0, params=[time] + values[i]),
                ],
            )
            for i, time in enumerate(times)
        ]
    )
    smspec = Smspec(
        nx=10,
        ny=10,
        nz=10,
        restarted_from_step=0,
        num_keywords=1 + len(summary_keys),
        restart="        ",
        keywords=["TIME    "] + list(summary_keys),
        well_names=[":+:+:+:+"] + [":+:+:+:+"] * len(summary_keys),
        region_numbers=[-32676] + [0] * len(summary_keys),
        units=[time_units.ljust(8)] + ["SM3     "] * len(summary_keys),
        start_date=start_date,
        intehead=SmspecIntehead(
            unit=UnitSystem.METRIC,
            simulator=Simulator.ECLIPSE_100,
        ),
    )
    _format = resfo.Format.FORMATTED if formatted else resfo.Format.UNFORMATTED
    smspec.to_file(f"{case}.{'F' if formatted else ''}SMSPEC", file_format=_format)
    unsmry.to_file(f"{case}.{'F' if formatted else ''}UNSMRY", file_format=_format)


@pytest.mark.usefixtures("use_tmpdir")
def test_has_key():
    create_summary(summary_keys=("FOPR", "FOPT", "FWCT"))

    summary = Summary("TEST")
    assert summary.has_key("FOPR")
    assert summary.has_key("FOPT")
    assert summary.has_key("FWCT")
    assert not summary.has_key("NONEXISTENT")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_start_date_property_is_the_start_date_from_the_file():
    create_summary(
        start_date=Date(day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0)
    )
    summary = Summary("TEST")
    assert summary.start_date == datetime.date(2000, 1, 1)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_numpy_vector():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0, 3.0),
        values=[[100.0], [110.0], [120.0], [130.0]],
    )

    summary = Summary("TEST")
    data = summary.numpy_vector("FOPR")

    assert data.tolist() == pytest.approx(
        [
            100.0,
            110.0,
            120.0,
            130.0,
        ]
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_first_and_last_value():
    create_summary(
        summary_keys=("FOPT",),
        times=(0.0, 1.0, 2.0),
        values=[[100.0], [200.0], [300.0]],
    )

    summary = Summary("TEST")
    assert summary.first_value("FOPT") == pytest.approx(100.0)
    assert summary.last_value("FOPT") == pytest.approx(300.0)
    assert summary.get_last("FOPT").value == pytest.approx(300.0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_dates_and_times():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0, 3.0))

    summary = Summary("TEST")
    dates = summary.dates

    assert len(dates) == 4
    assert dates[0] == datetime.datetime(2000, 1, 1, 0, 0, 0)
    assert dates[1] == datetime.datetime(2000, 1, 2, 0, 0, 0)

    days = summary.days
    assert len(days) == 4
    assert days[0] == pytest.approx(0.0)
    assert days[1] == pytest.approx(1.0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_keys_filtering():
    create_summary(summary_keys=("FOPR", "FOPT", "FWCT", "FGPR", "FGPT"))

    summary = Summary("TEST")
    assert list(summary.keys()) == ["FGPR", "FGPT", "FOPR", "FOPT", "FWCT"]

    f_keys = summary.keys(pattern="F*")
    assert len(f_keys) >= 5

    fop_keys = summary.keys(pattern="FOP*")
    assert "FOPR" in fop_keys
    assert "FOPT" in fop_keys


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_pandas_frame():
    create_summary(
        summary_keys=("FOPR", "FOPT"),
        times=(0.0, 1.0, 2.0),
        values=[[100.0, 1000.0], [110.0, 1100.0], [120.0, 1200.0]],
    )

    summary = Summary("TEST")
    df = summary.pandas_frame(column_keys=["FOPR", "FOPT"])

    assert set(df.columns) == {"FOPR", "FOPT"}
    assert df["FOPR"].to_numpy().tolist() == pytest.approx([100.0, 110.0, 120.0])
    assert df["FOPT"].to_numpy().tolist() == pytest.approx([1000.0, 1100.0, 1200.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_iget_access():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0),
        values=[[100.0], [200.0], [300.0]],
    )

    summary = Summary("TEST")
    assert summary.iget("FOPR", 0) == pytest.approx(100.0)
    assert summary.iget("FOPR", 1) == pytest.approx(200.0)
    assert summary.iget("FOPR", 2) == pytest.approx(300.0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_lazy_vs_eager_load():
    create_summary(summary_keys=("FOPR", "FOPT"), times=(0.0, 1.0, 2.0))

    sum_lazy = Summary("TEST", lazy_load=True)
    sum_eager = Summary("TEST", lazy_load=False)

    assert len(sum_lazy) == len(sum_eager)

    lazy_data = sum_lazy.numpy_vector("FOPR")
    eager_data = sum_eager.numpy_vector("FOPR")

    assert np.allclose(lazy_data, eager_data)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_contains_operator():
    create_summary(summary_keys=("FOPR", "FOPT"))

    summary = Summary("TEST")
    assert "FOPR" in summary
    assert "FOPT" in summary
    assert "NONEXISTENT" not in summary


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_iteration():
    keys = ["FOPR", "FOPT", "FWCT"]
    create_summary(summary_keys=keys)

    assert list(Summary("TEST")) == keys


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_start_and_end_time():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 5.0, 10.0),
        start_date=Date(day=15, month=3, year=2010, hour=0, minutes=0, micro_seconds=0),
    )

    summary = Summary("TEST")
    assert summary.start_time == datetime.datetime(2010, 3, 15, 0, 0, 0)
    assert summary.end_time == datetime.datetime(2010, 3, 25, 0, 0, 0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_sim_length():
    create_summary(summary_keys=("FOPR",), times=(0.0, 10.0, 20.0, 30.0))

    summary = Summary("TEST")
    assert summary.sim_length == pytest.approx(30.0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_iget_date():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    date0 = summary.iget_date(0)
    date1 = summary.iget_date(1)

    assert date0 == datetime.datetime(2000, 1, 1, 0, 0, 0)
    assert date1 == datetime.datetime(2000, 1, 2, 0, 0, 0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_iget_days():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.5, 3.0))

    summary = Summary("TEST")
    assert summary.iget_days(0) == pytest.approx(0.0)
    assert summary.iget_days(1) == pytest.approx(1.5)
    assert summary.iget_days(2) == pytest.approx(3.0)


def test_summary_var_type():
    assert Summary.var_type("AAQR") == SummaryVarType.RD_SMSPEC_AQUIFER_VAR
    assert Summary.var_type("BPR") == SummaryVarType.RD_SMSPEC_BLOCK_VAR
    assert Summary.var_type("FOPT") == SummaryVarType.RD_SMSPEC_FIELD_VAR
    assert Summary.var_type("GWPR") == SummaryVarType.RD_SMSPEC_GROUP_VAR
    assert Summary.var_type("RPR") == SummaryVarType.RD_SMSPEC_REGION_VAR
    assert Summary.var_type("WOPR") == SummaryVarType.RD_SMSPEC_WELL_VAR
    assert Summary.var_type("CGORL") == SummaryVarType.RD_SMSPEC_COMPLETION_VAR
    assert Summary.var_type("SOPR") == SummaryVarType.RD_SMSPEC_SEGMENT_VAR
    assert Summary.var_type("LBPR") == SummaryVarType.RD_SMSPEC_LOCAL_BLOCK_VAR
    assert Summary.var_type("LCGOR") == SummaryVarType.RD_SMSPEC_LOCAL_COMPLETION_VAR
    assert Summary.var_type("LWOPR") == SummaryVarType.RD_SMSPEC_LOCAL_WELL_VAR


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_export_csv():
    create_summary(summary_keys=("FOPR", "FOPT"), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    summary.export_csv("output.csv", keys=["FOPR", "FOPT"])

    assert os.path.exists("output.csv")
    with open("output.csv") as f:
        content = f.read()
        assert "FOPR" in content
        assert "FOPT" in content


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_check_sim_time():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 10.0, 20.0),
        start_date=Date(day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0),
    )

    summary = Summary("TEST")
    assert summary.check_sim_time(datetime.datetime(2000, 1, 1, 0, 0, 0))
    assert summary.check_sim_time(datetime.datetime(2000, 1, 15, 0, 0, 0))
    assert summary.check_sim_time(datetime.datetime(2000, 1, 21, 0, 0, 0))
    assert not summary.check_sim_time(datetime.datetime(2000, 1, 31, 0, 0, 0))


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_numpy_dates():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    np_dates = summary.numpy_dates

    assert np_dates.tolist() == [
        datetime.datetime(2000, 1, 1, 0, 0),
        datetime.datetime(2000, 1, 2, 0, 0),
        datetime.datetime(2000, 1, 3, 0, 0),
    ]
    assert np_dates.dtype == np.dtype("datetime64[ms]")


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_key_error_on_missing_key():
    create_summary(summary_keys=("FOPR",))

    summary = Summary("TEST")
    with pytest.raises(KeyError):
        summary.numpy_vector("NONEXISTENT")

    with pytest.raises(KeyError):
        summary.last_value("NONEXISTENT")

    with pytest.raises(KeyError):
        summary.first_value("NONEXISTENT")


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_writer_basic():
    start_date = datetime.datetime(2000, 1, 1)
    writer = Summary.writer("WRITER_TEST", start_date, 10, 10, 10)

    var = writer.add_variable("FOPT", unit="SM3")
    t_step = writer.add_t_step(1, 1.0)
    t_step[var.get_key1()] = 1000.0

    t_step = writer.add_t_step(2, 2.0)
    t_step[var.get_key1()] = 2000.0

    writer.fwrite()

    loaded = Summary("WRITER_TEST")
    assert "FOPT" in loaded
    assert loaded.first_value("FOPT") == pytest.approx(1000.0)
    assert loaded.last_value("FOPT") == pytest.approx(2000.0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_writer_multiple_variables():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("MULTI_VAR", start_date, 5, 5, 5)
    assert writer.base == "MULTI_VAR"
    assert writer.path is None

    fopt = writer.add_variable("FOPT", unit="SM3")
    fopr = writer.add_variable("FOPR", unit="SM3/DAY")
    fwpt = writer.add_variable("FWPT", unit="SM3")
    wopt = writer.add_variable("WOPT", unit="SM3", wgname="OP1")
    rpr = writer.add_variable("RPR", unit="SM3/DAY", num=1)
    aaqr = writer.add_variable("AAQR", unit="SM3/DAY", num=1)
    boft = writer.add_variable("BOFT", unit="SM3/DAY", num=1)
    gvpr = writer.add_variable("GVPR", unit="SM3/DAY", wgname="GROUP1")
    sofr = writer.add_variable("SOFR", unit="SM3/DAY", wgname="OP1", num=2)
    cgor = writer.add_variable("CGOR", unit="SM3/SM3", wgname="OP1", num=2)
    rsft = writer.add_variable("RSFT", unit="SM3", num=2)

    for i in range(1, 5):
        t_step = writer.add_t_step(i, float(i))
        t_step[fopt.get_key1()] = 100.0 * i
        t_step[fopr.get_key1()] = 10.0 * i
        t_step[fwpt.get_key1()] = 50.0 * i
        t_step[wopt.get_key1()] = 30.0 * i
        t_step[rpr.get_key1()] = 1000.0 * i
        t_step[aaqr.get_key1()] = 2000.0 * i
        t_step[boft.get_key1()] = 0.1 * i
        t_step[gvpr.get_key1()] = 0.3 * i
        t_step[sofr.get_key1()] = 1.0 * i
        t_step[cgor.get_key1()] = 0.9 * i
        t_step[rsft.get_key1()] = 3000.0 * i

    writer.fwrite()

    loaded = Summary("MULTI_VAR")
    assert len(loaded) == 4

    assert list(loaded.numpy_vector("FOPT")) == pytest.approx(
        [100.0 * i for i in range(1, 5)]
    )
    assert loaded.groups() == ["GROUP1"]
    assert loaded.groups("*") == ["GROUP1"]
    assert loaded.groups("NO") == []

    assert loaded.pandas_frame().to_dict() == {
        "FOPT": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(100.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(200.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(300.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(400.0),
        },
        "FOPR": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(10.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(20.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(30.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(40.0),
        },
        "FWPT": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(50.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(100.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(150.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(200.0),
        },
        "WOPT:OP1": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(30.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(60.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(90.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(120.0),
        },
        "RPR:1": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(1000.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(2000.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(3000.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(4000.0),
        },
        "AAQR:1": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(2000.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(4000.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(6000.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(8000.0),
        },
        "BOFT:1,1,1": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(0.1),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(0.2),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(0.3),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(0.4),
        },
        "GVPR:GROUP1": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(0.3),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(0.6),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(0.9),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(1.2),
        },
        "SOFR:OP1:2": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(1.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(2.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(3.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(4.0),
        },
        "CGOR:OP1:2,1,1": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(0.9),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(1.8),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(2.7),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(3.6),
        },
        "RSFT:2--10": {
            pd.Timestamp("2005-05-11 00:00:00"): pytest.approx(3000.0),
            pd.Timestamp("2005-05-12 00:00:00"): pytest.approx(6000.0),
            pd.Timestamp("2005-05-13 00:00:00"): pytest.approx(9000.0),
            pd.Timestamp("2005-05-14 00:00:00"): pytest.approx(12000.0),
        },
    }
    np.testing.assert_allclose(
        loaded.numpy_vector("FOPT"), [100.0 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector("FOPR"), [10.0 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector("FWPT"), [50.0 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(wopt.get_key1()), [30.0 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(rpr.get_key1()), [1000.0 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(aaqr.get_key1()), [2000.0 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(boft.get_key1()), [0.1 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(gvpr.get_key1()), [0.3 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(sofr.get_key1()), [1.0 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(cgor.get_key1()), [0.9 * i for i in range(1, 5)]
    )
    np.testing.assert_allclose(
        loaded.numpy_vector(rsft.get_key1()), [3000.0 * i for i in range(1, 5)]
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_writer_local_variables():
    start_date = datetime.datetime(2005, 5, 10)
    # writer does not actually support writing local
    # variables but will add them
    writer = Summary.writer("MULTI_VAR", start_date, 5, 5, 5)
    lwopt = writer.add_variable(
        "LWOPT", unit="SM3", wgname="OP1", lgr="LGR1", lgr_ijk=(0, 0, 0)
    )
    assert lwopt.get_key1() == "LWOPT:LGR1:OP1"
    assert writer.has_key(lwopt.get_key1())

    for i in range(1, 5):
        t_step = writer.add_t_step(i, float(i))
        t_step[lwopt.get_key1()] = 100.0 * i


@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_variable_constructs_a_field_var_with_no_wgname_or_num():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("FIELD_VAR", start_date, 5, 5, 5)

    node = writer.add_variable("FOPT", unit="SM3", default_value=1.0)

    assert node.keyword == "FOPT"
    assert node.wgname is None
    assert node.unit == "SM3"
    assert node.num == 0
    assert node.default == pytest.approx(1.0)
    assert node.get_key1() == "FOPT"
    assert node.get_key2() is None
    assert node.var_type() == SummaryVarType.RD_SMSPEC_FIELD_VAR
    assert node.get_num() == 0
    assert node.is_total()
    assert not node.is_rate()
    assert not node.is_historical()


@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_variable_constructs_a_well_var_with_wgname():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("WELL_VAR", start_date, 5, 5, 5)

    node = writer.add_variable("WOPR", unit="SM3/DAY", wgname="OP1", default_value=2.0)

    assert node.keyword == "WOPR"
    assert node.wgname == "OP1"
    assert node.unit == "SM3/DAY"
    assert node.default == pytest.approx(2.0)
    assert node.get_key1() == "WOPR:OP1"
    assert node.var_type() == SummaryVarType.RD_SMSPEC_WELL_VAR
    assert node.is_rate()
    assert not node.is_total()
    assert not node.is_historical()


@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_variable_constructs_a_group_var_with_wgname():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("GROUP_VAR", start_date, 5, 5, 5)

    node = writer.add_variable("GOPR", unit="SM3/DAY", wgname="GROUP1")

    assert node.keyword == "GOPR"
    assert node.wgname == "GROUP1"
    assert node.get_key1() == "GOPR:GROUP1"
    assert node.var_type() == SummaryVarType.RD_SMSPEC_GROUP_VAR


@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_variable_constructs_a_region_var_with_num():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("REGION_VAR", start_date, 5, 5, 5)

    node = writer.add_variable("RPR", unit="BARS", num=3)

    assert node.keyword == "RPR"
    assert node.wgname is None
    assert node.num == 3
    assert node.get_num() == 3
    assert node.get_key1() == "RPR:3"
    assert node.var_type() == SummaryVarType.RD_SMSPEC_REGION_VAR


@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_variable_constructs_a_well_completion_var_with_wgname_and_num():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("COMPLETION_VAR", start_date, 5, 5, 5)

    node = writer.add_variable("CGOR", unit="SM3/SM3", wgname="OP1", num=2)

    assert node.keyword == "CGOR"
    assert node.wgname == "OP1"
    assert node.num == 2
    assert node.get_key1() == "CGOR:OP1:2,1,1"
    assert node.get_key2() == "CGOR:OP1:2"
    assert node.var_type() == SummaryVarType.RD_SMSPEC_COMPLETION_VAR


@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_local_variable_constructs_a_local_well_var():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("LOCAL_VAR", start_date, 5, 5, 5)

    node = writer.add_variable(
        "LWOPT", unit="SM3", wgname="OP1", lgr="LGR1", lgr_ijk=(0, 0, 0)
    )

    assert node.keyword == "LWOPT"
    assert node.wgname == "OP1"
    assert node.get_key1() == "LWOPT:LGR1:OP1"
    assert node.var_type() == SummaryVarType.RD_SMSPEC_LOCAL_WELL_VAR


@pytest.mark.usefixtures("use_tmpdir")
def test_that_wgname_empty_string_and_none_are_equivalent_in_add_variable():
    """
    An empty wgname string is, like None, treated as "no well/group name
    given" - the resulting node.wgname is None in both cases.
    """
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("WGNAME_EMPTY", start_date, 5, 5, 5)

    node_empty = writer.add_variable("FOPT", unit="SM3", wgname="")
    node_none = writer.add_variable("FGPT", unit="SM3", wgname=None)

    assert node_empty.wgname is None
    assert node_none.wgname is None


@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_variable_raises_valueerror_for_well_var_without_wgname():
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("WELL_VAR_NO_WGNAME", start_date, 5, 5, 5)

    with pytest.raises(ValueError):
        writer.add_variable("WOPR", unit="SM3/DAY", wgname=None)


@given(
    keyword=summary_variables(),
    wgname=st.one_of(
        st.none(), st.just(""), st.sampled_from(["OP1", "OP2", "GROUP1", "INJ1"])
    ),
    num=st.integers(min_value=0, max_value=50),
    unit=st.sampled_from(["SM3", "SM3/DAY", "BARSA", "SM3/SM3", "FRACTION"]),
    default_value=st.floats(
        min_value=-1.0e5,
        max_value=1.0e5,
        allow_nan=False,
        allow_infinity=False,
        width=32,
    ),
    lgr_ijk=st.tuples(
        st.integers(min_value=0, max_value=4),
        st.integers(min_value=0, max_value=4),
        st.integers(min_value=0, max_value=4),
    ),
)
@pytest.mark.usefixtures("use_tmpdir")
def test_that_add_variable_constructed_nodes_roundtrip_and_compare_to_self(
    keyword, wgname, num, unit, default_value, lgr_ijk
):
    start_date = datetime.datetime(2005, 5, 10)
    writer = Summary.writer("ADD_VAR_PROP", start_date, 5, 5, 5)

    is_local = len(keyword) >= 2 and keyword[0] == "L" and keyword[1] in "BCW"

    try:
        if is_local:
            node = writer.add_variable(
                keyword,
                unit=unit,
                wgname=wgname,
                num=num,
                default_value=default_value,
                lgr="LGR1",
                lgr_ijk=lgr_ijk,
            )
        else:
            node = writer.add_variable(
                keyword,
                unit=unit,
                wgname=wgname,
                num=num,
                default_value=default_value,
            )
    except (ValueError, RuntimeError):
        assume(False)
        raise

    assert node.keyword == keyword
    assert node.unit == unit
    assert node.default == pytest.approx(default_value)
    assert node.wgname is None or node.wgname == wgname

    if is_local:
        # add_local_variable() does not accept a NUMS value.
        assert node.num == 0
        assert node.get_num() == 0
    else:
        assert node.num == num
        assert node.get_num() == num

    assert node == node


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_from_pandas_roundtrip():
    dates = pd.date_range("2000-01-01", periods=5, freq="D")
    df = pd.DataFrame(
        {
            "FOPR": [100.0, 110.0, 120.0, 130.0, 140.0],
            "FOPT": [1000.0, 1100.0, 1200.0, 1300.0, 1400.0],
        },
        index=dates,
    )

    sum_from_df = Summary.from_pandas("FROM_PANDAS", df)

    assert "FOPR" in sum_from_df
    assert "FOPT" in sum_from_df
    assert sum_from_df.first_value("FOPR") == pytest.approx(100.0)
    assert sum_from_df.last_value("FOPT") == pytest.approx(1400.0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_time_range():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 30.0, 60.0, 90.0, 120.0),
        start_date=Date(day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0),
    )

    summary = Summary("TEST")
    time_range = summary.time_range(interval="10d")
    assert isinstance(time_range, TimeVector)
    assert [t.datetime() for t in time_range] == [
        datetime.datetime(2000, 1, 1, 0, 0),
        datetime.datetime(2000, 1, 11, 0, 0),
        datetime.datetime(2000, 1, 21, 0, 0),
        datetime.datetime(2000, 1, 31, 0, 0),
        datetime.datetime(2000, 2, 10, 0, 0),
        datetime.datetime(2000, 2, 20, 0, 0),
        datetime.datetime(2000, 3, 1, 0, 0),
        datetime.datetime(2000, 3, 11, 0, 0),
        datetime.datetime(2000, 3, 21, 0, 0),
        datetime.datetime(2000, 3, 31, 0, 0),
        datetime.datetime(2000, 4, 10, 0, 0),
        datetime.datetime(2000, 4, 20, 0, 0),
        datetime.datetime(2000, 4, 30, 0, 0),
    ]

    assert len(time_range) > 0


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_unit():
    create_summary(summary_keys=("FOPR",))

    summary = Summary("TEST")
    unit = summary.unit("FOPR")
    assert unit == "SM3"


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_case_property():
    create_summary(summary_keys=("FOPR",), case="MYCASE")

    summary = Summary("MYCASE")
    assert summary.case == "MYCASE"


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_base_and_path(tmpdir):
    create_summary(summary_keys=("FOPR",))

    summary = Summary("TEST")
    assert summary.case == "TEST"
    assert summary.base == "TEST"
    assert summary.path is None
    assert summary.abs_path == tmpdir


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_length_property():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0, 3.0, 4.0))

    summary = Summary("TEST")
    assert summary.length == 5
    assert len(summary) == 5


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_smspec_node():
    create_summary(summary_keys=("FOPR",))

    summary = Summary("TEST")
    node = summary.smspec_node("FOPR")

    assert node is not None
    assert node.keyword == "FOPR"


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_interp():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 2.0, 4.0),
        values=[[100.0], [200.0], [300.0]],
    )

    summary = Summary("TEST")

    # Test at exact time points
    value_at_0 = summary.get_interp("FOPR", days=0.0)
    assert value_at_0 == pytest.approx(100.0)

    value_at_2 = summary.get_interp("FOPR", days=2.0)
    assert value_at_2 == pytest.approx(200.0)

    value_at_4 = summary.get_interp("FOPR", days=4.0)
    assert value_at_4 == pytest.approx(300.0)

    assert summary.get_interp_direct("FOPR", date=datetime.date(2000, 1, 1)) == 100.0
    keyvec = SummaryKeyWordVector(summary)
    keyvec.add_keyword("FOPR")
    assert list(
        summary.get_interp_row(keyvec, datetime.date(2000, 1, 1))
    ) == pytest.approx([100.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_interp_vector():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 2.0, 4.0),
        values=[[100.0], [200.0], [300.0]],
    )

    summary = Summary("TEST")

    interp_values = summary.get_interp_vector(
        "FOPR", days_list=[0.0, 1.0, 2.0, 3.0, 4.0]
    )
    assert list(interp_values) == pytest.approx(
        [
            100.0,
            200.0,
            200.0,
            300.0,
            300.0,
        ]
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_pandas_frame_with_time_index():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 5.0, 10.0, 15.0),
        values=[[100.0], [150.0], [200.0], [250.0]],
    )

    summary = Summary("TEST")

    df = summary.pandas_frame(
        time_index=[
            datetime.datetime(2000, 1, 1),
            datetime.datetime(2000, 1, 6),
            datetime.datetime(2000, 1, 11),
        ],
        column_keys=["FOPR"],
    )

    assert list(df.columns) == ["FOPR"]
    assert df["FOPR"].to_numpy().tolist() == pytest.approx([100.0, 150.0, 200.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_static_methods():
    """Test static methods is_rate and is_total"""
    assert Summary.is_rate("FOPR")
    assert not Summary.is_rate("FOPT")
    assert Summary.is_total("FOPT")
    assert not Summary.is_total("FOPR")


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_key_index():
    create_summary(summary_keys=("FOPR", "FOPT"))

    summary = Summary("TEST")

    assert summary.get_key_index("FOPR") == 1
    assert summary.get_key_index("FOPT") == 2


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_iiget():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0),
        values=[[100.0], [200.0], [300.0]],
    )

    summary = Summary("TEST")
    key_index = summary.get_key_index("FOPR")

    assert summary.iiget(0, key_index) == pytest.approx(100.0)
    assert summary.iiget(1, key_index) == pytest.approx(200.0)
    assert summary.iiget(2, key_index) == pytest.approx(300.0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_load_method():
    create_summary(summary_keys=("FOPR",), case="LOADTEST")

    summary = Summary.load("LOADTEST.SMSPEC", "LOADTEST.UNSMRY")

    assert "FOPR" in summary
    assert len(summary) == 3


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_data_start():
    create_summary(
        summary_keys=("FOPR",),
        start_date=Date(day=20, month=6, year=2015, hour=0, minutes=0, micro_seconds=0),
    )

    summary = Summary("TEST")
    assert summary.data_start == datetime.datetime(2015, 6, 20, 0, 0, 0)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_first_day():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    assert summary.first_day == pytest.approx(0.0)


def test_date2num():
    dt = datetime.datetime(2000, 1, 1, 12, 30, 0)
    num = date2num(dt)

    assert isinstance(num, float)
    assert num == pytest.approx(730120.520833333)


def test_that_summary_raises_value_error_when_load_case_is_falsy():
    with pytest.raises(ValueError, match="load_case must be the basename"):
        Summary(None)

    with pytest.raises(ValueError, match="load_case must be the basename"):
        Summary("")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_summary_load_raises_ioerror_when_smspec_is_not_a_file():
    with pytest.raises(IOError, match="No such file"):
        Summary.load("NONEXISTENT.SMSPEC", "TEST.UNSMRY")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_summary_load_raises_ioerror_when_unsmry_is_not_a_file():
    create_summary(summary_keys=("FOPR",))

    with pytest.raises(IOError, match="No such file"):
        Summary.load("TEST.SMSPEC", "NONEXISTENT.UNSMRY")


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_wells():
    smspec = Smspec(
        nx=10,
        ny=10,
        nz=10,
        restarted_from_step=0,
        num_keywords=3,
        restart="        ",
        keywords=["TIME    ", "WOPR    ", "WWCT    "],
        well_names=[":+:+:+:+", "WELL1   ", "WELL2   "],
        region_numbers=[-32676, 0, 0],
        units=["DAYS    ", "SM3/DAY ", "        "],
        start_date=Date(day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0),
        intehead=SmspecIntehead(
            unit=UnitSystem.METRIC, simulator=Simulator.ECLIPSE_100
        ),
    )
    unsmry = Unsmry(
        steps=[
            SummaryStep(
                seqnum=0,
                ministeps=[SummaryMiniStep(mini_step=0, params=[0.0, 100.0, 0.5])],
            )
        ]
    )
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")

    summary = Summary("TEST")
    wells = summary.wells()
    assert isinstance(wells, StringList)
    assert list(wells) == ["WELL1", "WELL2"]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_groups():
    create_summary(summary_keys=("FOPR",))

    summary = Summary("TEST")
    groups = summary.groups()
    assert isinstance(groups, StringList)
    assert list(groups) == []


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_numpy_vector_with_time_index():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 5.0, 10.0),
        values=[[100.0], [150.0], [200.0]],
    )

    summary = Summary("TEST")

    time_index = [
        datetime.datetime(2000, 1, 1),
        datetime.datetime(2000, 1, 6),
    ]

    data = summary.numpy_vector("FOPR", time_index=time_index)
    assert data.tolist() == pytest.approx([100.0, 150.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_time_range_with_start():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 30.0, 60.0, 90.0),
        start_date=Date(day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0),
    )

    summary = Summary("TEST")

    custom_start = datetime.datetime(2000, 1, 15)
    time_range = summary.time_range(start=custom_start, interval="10d")

    assert isinstance(time_range, TimeVector)
    assert [t.datetime() for t in time_range] == [
        datetime.datetime(2000, 1, 15, 0, 0),
        datetime.datetime(2000, 1, 25, 0, 0),
        datetime.datetime(2000, 2, 4, 0, 0),
        datetime.datetime(2000, 2, 14, 0, 0),
        datetime.datetime(2000, 2, 24, 0, 0),
        datetime.datetime(2000, 3, 5, 0, 0),
        datetime.datetime(2000, 3, 15, 0, 0),
        datetime.datetime(2000, 3, 25, 0, 0),
        datetime.datetime(2000, 4, 4, 0, 0),
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_time_range_with_end():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 30.0, 60.0, 90.0),
        start_date=Date(day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0),
    )

    summary = Summary("TEST")

    custom_end = datetime.datetime(2000, 2, 15)
    time_range = summary.time_range(end=custom_end, interval="10d")

    assert isinstance(time_range, TimeVector)
    assert [t.datetime() for t in time_range] == [
        datetime.datetime(2000, 1, 1, 0, 0),
        datetime.datetime(2000, 1, 11, 0, 0),
        datetime.datetime(2000, 1, 21, 0, 0),
        datetime.datetime(2000, 1, 31, 0, 0),
        datetime.datetime(2000, 2, 10, 0, 0),
        datetime.datetime(2000, 2, 20, 0, 0),
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_blocked_production():
    """Test blocked_production method"""
    create_summary(
        summary_keys=("FOPT",),
        times=(0.0, 10.0, 20.0, 30.0),
        values=[[0.0], [1000.0], [2500.0], [4000.0]],
    )

    summary = Summary("TEST")

    time_range = [
        datetime.datetime(2000, 1, 1),
        datetime.datetime(2000, 1, 11),
        datetime.datetime(2000, 1, 21),
    ]

    blocked = summary.blocked_production("FOPT", time_range)
    assert isinstance(blocked, DoubleVector)
    assert list(blocked) == pytest.approx([1000.0, 1500.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_report_time():
    create_summary(
        start_date=Date(day=1, month=1, year=2000, hour=0, minutes=0, micro_seconds=0)
    )

    summary = Summary("TEST")

    assert summary.get_report_time(0) == datetime.date(day=1, month=1, year=2000)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_report_time_with_invalid_step_raises():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))
    summary = Summary("TEST")

    with pytest.raises(ValueError, match="Internal error when looking up report"):
        summary.get_report_time(99999)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_interp_vector_with_date_list():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 2.0, 4.0),
        values=[[100.0], [200.0], [300.0]],
    )

    summary = Summary("TEST")

    date_list = [
        datetime.datetime(2000, 1, 1),
        datetime.datetime(2000, 1, 3),
        datetime.datetime(2000, 1, 5),
    ]

    interp_values = summary.get_interp_vector("FOPR", date_list=date_list)
    assert list(interp_values) == pytest.approx([100.0, 200.0, 300.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_case_for_cases_without_restart_is_none():
    create_summary(summary_keys=("FOPR",))

    summary = Summary("TEST")
    assert summary.restart_case is None


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_days():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    assert summary.get_days() == pytest.approx([0.0, 1.0, 2.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_dates():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    assert summary.get_dates() == [
        datetime.datetime(2000, 1, 1, 0, 0),
        datetime.datetime(2000, 1, 2, 0, 0),
        datetime.datetime(2000, 1, 3, 0, 0),
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_report_step_property():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    assert summary.report_step == [1, 2, 3]
    assert summary.get_report_step() == [1, 2, 3]
    assert [summary.iget_report(i) for i in range(3)] == [1, 2, 3]
    assert summary.first_report == 1
    assert summary.last_report == 3


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_end_date():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0),
        start_date=Date(day=10, month=5, year=2010, hour=0, minutes=0, micro_seconds=0),
    )

    summary = Summary("TEST")
    assert summary.end_date == datetime.date(2010, 5, 12)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_first_gt_index():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0),
        values=[[100.0], [200.0], [300.0]],
    )

    summary = Summary("TEST")
    assert summary.first_gt_index("FOPR", 150.0) == 1


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_first_lt_index():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0),
        values=[[300.0], [200.0], [100.0]],
    )

    summary = Summary("TEST")
    assert summary.first_lt_index("FOPR", 250.0) == 1


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_solve_dates():
    """Test solve_dates method"""
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0, 3.0),
        values=[[100.0], [150.0], [200.0], [250.0]],
    )

    summary = Summary("TEST")
    assert summary.solve_dates("FOPR", 175.0) == [
        datetime.datetime(2000, 1, 2, 0, 0, 1)
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_solve_days():
    """Test solve_days method"""
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 1.0, 2.0, 3.0),
        values=[[100.0], [150.0], [200.0], [250.0]],
    )

    summary = Summary("TEST")
    days = summary.solve_days("FOPR", 175.0)
    assert isinstance(days, DoubleVector)
    assert list(days) == pytest.approx([1.000011])


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_fwrite():
    start_date = datetime.datetime(2000, 1, 1)
    writer = Summary.writer("FWRITE_TEST", start_date, 5, 5, 5)

    var = writer.add_variable("FOPT")
    t_step = writer.add_t_step(1, 1.0)
    t_step[var.get_key1()] = 1000.0

    assert writer.can_write()
    writer.fwrite()

    summary = Summary("FWRITE_TEST")
    assert len(summary) == 1
    assert list(summary.keys()) == ["FOPT"]
    assert summary.days == [1.0]
    kw = summary["FOPT"]
    assert kw.values.tolist() == [1000.0]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_alloc_time_vector():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    time_vec = summary.alloc_time_vector(False)
    assert isinstance(time_vec, TimeVector)
    assert [t.datetime() for t in time_vec] == [
        datetime.datetime(2000, 1, 1, 0, 0),
        datetime.datetime(2000, 1, 2, 0, 0),
        datetime.datetime(2000, 1, 3, 0, 0),
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_alloc_data_vector():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    key_index = summary.get_key_index("FOPR")
    data_vec = summary.alloc_data_vector(key_index, False)
    assert isinstance(data_vec, DoubleVector)
    assert list(data_vec) == [100.0, 110.0, 120.0]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_alloc_time_vector_report_only():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    time_vec = summary.alloc_time_vector(True)
    assert isinstance(time_vec, TimeVector)
    assert [t.datetime() for t in time_vec] == [
        datetime.datetime(2000, 1, 1, 0, 0),
        datetime.datetime(2000, 1, 2, 0, 0),
        datetime.datetime(2000, 1, 3, 0, 0),
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_alloc_data_vector_report_only():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    key_index = summary.get_key_index("FOPR")
    data_vec = summary.alloc_data_vector(key_index, True)
    assert isinstance(data_vec, DoubleVector)
    assert list(data_vec) == [100.0, 110.0, 120.0]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_report_from_time():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    # Exact report dates map back to their report step.
    assert summary.get_report(date=datetime.date(2000, 1, 1)) == 1
    assert summary.get_report(date=datetime.date(2000, 1, 2)) == 2
    assert summary.get_report(date=datetime.date(2000, 1, 3)) == 3
    # A time strictly inside the range but not matching any report gives -1.
    assert summary.get_report(date=datetime.datetime(2000, 1, 2, 12, 0)) == -1


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_fwrite_non_unified_roundtrip():
    start_date = datetime.datetime(2000, 1, 1)
    writer = Summary.writer("NON_UNIFIED", start_date, 5, 5, 5, unified=False)

    var = writer.add_variable("FOPT")
    for report_step, sim_days, value in [
        (1, 1.0, 100.0),
        (2, 2.0, 200.0),
        (3, 3.0, 300.0),
    ]:
        t_step = writer.add_t_step(report_step, sim_days)
        t_step[var.get_key1()] = value

    assert writer.can_write()
    writer.fwrite()

    # A non-unified write produces one summary file per report step.
    assert sorted(f for f in os.listdir(".") if f.startswith("NON_UNIFIED")) == [
        "NON_UNIFIED.S0001",
        "NON_UNIFIED.S0002",
        "NON_UNIFIED.S0003",
        "NON_UNIFIED.SMSPEC",
    ]

    summary = Summary("NON_UNIFIED")
    assert len(summary) == 3
    assert summary["FOPT"].values.tolist() == [100.0, 200.0, 300.0]
    assert summary.days == [1.0, 2.0, 3.0]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_writer_out_of_order_tstep_same_report_is_sorted():
    writer = Summary.writer("OUT_OF_ORDER", datetime.datetime(2000, 1, 1), 5, 5, 5)
    var = writer.add_variable("FOPT")

    # Two ministeps within the same report step, added with a decreasing
    # sim_days. This forces a full index rebuild (rather than the fast-path
    # append) and the ministeps are sorted by simulation time.
    t_step = writer.add_t_step(1, 2.0)
    t_step[var.get_key1()] = 200.0
    t_step = writer.add_t_step(1, 1.0)
    t_step[var.get_key1()] = 100.0

    assert writer.days == [1.0, 2.0]
    assert writer["FOPT"].values.tolist() == [100.0, 200.0]


def _write_restart_chain(restart_step, prediction_start):
    """Write a HISTORY case and a PREDICTION case restarting from it, and
    return the combined summary loaded with include_restart=True."""
    history = createSummary(
        "HISTORY",
        [("FOPT", None, 0, "SM3")],
        sim_length_days=100,
        num_report_step=10,
        num_mini_step=10,
        sim_start=datetime.date(2000, 1, 1),
    )
    history.fwrite()
    prediction = createSummary(
        "PREDICTION",
        [("FOPT", None, 0, "SM3")],
        sim_length_days=100,
        num_report_step=10,
        num_mini_step=10,
        sim_start=datetime.date(2000, 1, 1),
        data_start=prediction_start(history),
        restart_case="HISTORY",
        restart_step=restart_step,
    )
    prediction.fwrite()
    return Summary("PREDICTION", include_restart=True)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_restart_chain_with_overlap():
    summary = _write_restart_chain(
        restart_step=5, prediction_start=lambda h: h.get_report_time(5)
    )
    assert summary.first_report == 1
    assert summary.last_report == 15
    assert len(summary) == 149


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_restart_chain_with_gap():
    summary = _write_restart_chain(
        restart_step=10,
        prediction_start=lambda h: h.end_date + datetime.timedelta(days=30),
    )
    assert summary.first_report == 1
    assert summary.last_report == 20
    assert len(summary) == 200


def _load_restart_chain(prediction_start, restart_step=5, **load_kwargs):
    """Write a HISTORY case and a PREDICTION case restarting from it"""
    history = createSummary(
        "HISTORY",
        [("FOPT", None, 0, "SM3")],
        sim_length_days=100,
        num_report_step=10,
        num_mini_step=10,
        sim_start=datetime.date(2000, 1, 1),
    )
    history.fwrite()
    prediction = createSummary(
        "PREDICTION",
        [("FOPT", None, 0, "SM3")],
        sim_length_days=100,
        num_report_step=10,
        num_mini_step=10,
        sim_start=datetime.date(2000, 1, 1),
        data_start=prediction_start(history),
        restart_case="HISTORY",
        restart_step=restart_step,
    )
    prediction.fwrite()
    return Summary("PREDICTION", include_restart=True, **load_kwargs), history


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_drops_history_reports_at_or_after_prediction_start():
    summary, history = _load_restart_chain(lambda h: h.get_report_time(5))
    prediction_start = history.get_report_time(5)
    idx = _boundary_index(summary, history)
    assert all(summary.iget_date(i).date() < prediction_start for i in range(idx))
    assert all(
        summary.iget_date(i).date() >= prediction_start
        for i in range(idx, len(summary))
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_with_gap_retains_all_history_reports():
    summary, history = _load_restart_chain(
        lambda h: h.end_date + datetime.timedelta(days=30), restart_step=10
    )
    prediction = Summary("PREDICTION", include_restart=False)
    assert summary.first_report == 1
    assert len(summary) == len(history) + len(prediction)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_with_prediction_starting_at_history_start_retains_no_history_data():
    summary, _ = _load_restart_chain(lambda h: h.start_date, restart_step=1)
    prediction = Summary("PREDICTION", include_restart=False)
    assert len(summary) == len(prediction)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_length_equals_history_before_boundary_plus_prediction():
    summary, history = _load_restart_chain(lambda h: h.get_report_time(5))
    prediction = Summary("PREDICTION", include_restart=False)
    retained_history = sum(
        1
        for i in range(len(history))
        if history.iget_date(i).date() < history.get_report_time(5)
    )
    assert len(summary) == retained_history + len(prediction)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_values_switch_from_history_to_prediction_at_the_boundary():
    summary, history = _load_restart_chain(lambda h: h.get_report_time(5))
    idx = _boundary_index(summary, history)
    prediction = Summary("PREDICTION", include_restart=False)
    assert summary["FOPT"].values[idx - 1] == pytest.approx(
        history["FOPT"].values[idx - 1]
    )
    assert summary["FOPT"].values[idx] == pytest.approx(prediction["FOPT"].values[0])


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_interpolated_value_is_continuous_at_the_boundary():
    summary, history = _load_restart_chain(lambda h: h.get_report_time(5))
    idx = _boundary_index(summary, history)
    before = summary.get_interp("FOPT", date=summary.iget_date(idx - 1))
    after = summary.get_interp("FOPT", date=summary.iget_date(idx))
    assert after >= before


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_first_and_last_values_come_from_the_correct_cases():
    summary, history = _load_restart_chain(lambda h: h.get_report_time(5))
    prediction = Summary("PREDICTION", include_restart=False)
    assert summary.first_value("FOPT") == pytest.approx(history.first_value("FOPT"))
    assert summary.last_value("FOPT") == pytest.approx(prediction.last_value("FOPT"))


@pytest.mark.usefixtures("use_tmpdir")
def test_that_loading_with_include_restart_false_ignores_the_parent_case():
    _load_restart_chain(lambda h: h.get_report_time(5))
    with_parent = Summary("PREDICTION", include_restart=True)
    without_parent = Summary("PREDICTION", include_restart=False)
    assert len(without_parent) < len(with_parent)
    assert without_parent.first_report == 6
    assert with_parent.first_report == 1


def _boundary_index(summary, history):
    """Index of the first PREDICTION-sourced timestep, i.e. the first timestep
    whose date is at or after the PREDICTION data start (history report 5).
    """
    prediction_start = history.get_report_time(5)
    return next(
        i
        for i in range(len(summary))
        if summary.iget_date(i).date() >= prediction_start
    )


Name: TypeAlias = str


@dataclass
class RestartChain:
    """A chain of restarted summary cases, parent first and main case last."""

    cases: list[tuple[Name, Smspec, Unsmry]]

    @property
    def name(self) -> Name:
        """Name of the last (main) case - the one to load."""
        return self.cases[-1][0]

    def to_files(self) -> None:
        """Write every case's .SMSPEC/.UNSMRY pair to the current directory."""
        for name, smspec, unsmry in self.cases:
            smspec.to_file(f"{name}.SMSPEC")
            unsmry.to_file(f"{name}.UNSMRY")


_chain_values = st.floats(
    min_value=0.1, max_value=1e9, allow_nan=False, allow_infinity=False
)
_chain_start_dates = st.datetimes(
    min_value=datetime.datetime(1970, 1, 1),
    max_value=datetime.datetime(2030, 1, 1),
)


@st.composite
def restart_chains(draw, min_cases: int = 2, max_cases: int = 4) -> RestartChain:
    """Hypothesis strategy for chains of restarted summary cases.

    A single strictly increasing timeline of whole-day report dates is generated
    and then cut into overlapping cases. Each case is restarted from the previous.
    """
    template = draw(
        smspecs(
            sum_keys=st.lists(summary_variables(), min_size=1, max_size=5),
            use_days=st.just(True),
        )
    )
    # Ensure keys are unique
    assume(len(set(template.summary_keys())) == len(template.keywords))

    # A strictly increasing timeline of whole-day offsets with D[0] == 0.
    deltas = draw(
        st.lists(
            st.integers(min_value=1, max_value=200),
            min_size=max_cases,
            max_size=15,
            unique=True,
        )
    )
    offsets = [0.0]
    for delta in sorted(deltas):
        offsets.append(offsets[-1] + float(delta))
    total = len(offsets)

    num_cases = draw(st.integers(min_value=min_cases, max_value=min(max_cases, total)))
    # Case k will start at time index cut_indices[k]
    cut_indices = [0] + sorted(
        draw(
            st.lists(
                st.integers(min_value=1, max_value=total - 1),
                min_size=num_cases - 1,
                max_size=num_cases - 1,
                unique=True,
            )
        )
    )

    base = draw(_chain_start_dates)
    n_params = len(template.keywords)
    cases = []
    for k, start_index in enumerate(cut_indices):
        name = f"RCHAIN{k}"
        case_offsets = [offsets[j] for j in range(start_index, total)]
        smspec = replace(
            template,
            start_date=Date.from_datetime(base),
            restart="" if k == 0 else cases[k - 1][0],
            restarted_from_step=start_index,
        )
        unsmry = Unsmry(
            steps=[
                SummaryStep(
                    j,
                    [
                        SummaryMiniStep(
                            j,
                            [
                                offset,
                                *draw(
                                    st.lists(
                                        _chain_values,
                                        min_size=n_params - 1,
                                        max_size=n_params - 1,
                                    )
                                ),
                            ],
                        )
                    ],
                )
                for j, offset in enumerate(case_offsets)
            ]
        )
        cases.append((name, smspec, unsmry))
    return RestartChain(cases)


@given(chain=restart_chains(), lazy=st.booleans())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_has_a_strictly_increasing_timeline(chain, lazy):
    chain.to_files()
    summary = Summary(chain.name, include_restart=True, lazy_load=lazy)
    dates = summary.dates
    assert all(a < b for a, b in itertools.pairwise(dates))
    assert len(summary) == len(set(dates))

    days = summary.days
    assert all(a < b for a, b in itertools.pairwise(days))
    # Simulated days are measured from the chain's own data start.
    assert days[0] == pytest.approx(summary.first_day)


@given(chain=restart_chains())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_report_steps_are_contiguous(chain):
    chain.to_files()
    summary = Summary(chain.name, include_restart=True, lazy_load=False)
    reports = [summary.iget_report(i) for i in range(len(summary))]
    # The chain always begins at report 1
    assert reports[0] == summary.first_report == 1
    assert reports[-1] == summary.last_report
    assert {b - a for a, b in itertools.pairwise(reports)} == {1}
    assert summary.get_report_step(report_only=True) == list(
        range(summary.first_report, summary.last_report + 1)
    )
    assert set(range(summary.first_report, summary.last_report + 1)) <= set(
        summary.get_report_step(report_only=False)
    )


@given(chain=restart_chains())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_report_time_matches_last_timestep(chain):
    chain.to_files()
    summary = Summary(chain.name, include_restart=True, lazy_load=False)
    for report in range(summary.first_report, summary.last_report + 1):
        indices = [i for i in range(len(summary)) if summary.iget_report(i) == report]
        assert summary.iget_date(indices[-1]).date() == summary.get_report_time(report)


@given(chain=restart_chains())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_chain_pandas_frame_has_one_sorted_unique_row_per_step(chain):
    chain.to_files()
    summary = Summary(chain.name, include_restart=True, lazy_load=False)
    frame = summary.pandas_frame()
    assert len(frame) == len(summary)
    assert list(frame.index) == sorted(frame.index)
    assert frame.index.is_unique


@given(chain=restart_chains())
@pytest.mark.usefixtures("use_tmpdir")
def test_that_generated_restart_chain_is_identical_under_lazy_and_eager_load(chain):
    chain.to_files()
    lazy = Summary(chain.name, include_restart=True, lazy_load=True)
    eager = Summary(chain.name, include_restart=True, lazy_load=False)
    assert len(lazy) == len(eager)
    assert lazy.dates == eager.dates
    assert [lazy.iget_report(i) for i in range(len(lazy))] == [
        eager.iget_report(i) for i in range(len(eager))
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_get_general_var_index():
    create_summary(summary_keys=("FOPR",))

    summary = Summary("TEST")
    assert summary.get_general_var_index("FOPR") == 1


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_export_csv_with_keys_none():
    create_summary(summary_keys=("FOPR", "FOPT", "FWCT"), times=(0.0, 1.0, 2.0))

    summary = Summary("TEST")
    summary.export_csv("all_keys.csv", keys=None)

    assert os.path.exists("all_keys.csv")

    with open("all_keys.csv") as f:
        content = f.read()
    assert content == dedent("""\
        DAYS;DATE;FOPR;FOPT;FWCT
           0.00;2000-01-01;100;100;100
           1.00;2000-01-02;110;110;110
           2.00;2000-01-03;120;120;120
    """)


@pytest.mark.usefixtures("use_tmpdir")
def test_summary_resample():
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 5.0, 10.0, 15.0),
        values=[[100.0], [150.0], [200.0], [250.0]],
    )

    summary = Summary("TEST")

    time_points = TimeVector()
    time_points.append(datetime.datetime(2000, 1, 1))
    time_points.append(datetime.datetime(2000, 1, 6))
    time_points.append(datetime.datetime(2000, 1, 11))

    resampled = summary.resample("RESAMPLED", time_points)

    assert resampled.alloc_time_vector(False) == time_points
    assert resampled["FOPR"].values.tolist() == pytest.approx([100.0, 150.0, 200.0])


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("bad_time_points", [5, None, DoubleVector()])
def test_summary_resample_with_wrong_type_time_points_raises_type_error(
    bad_time_points,
):
    create_summary(
        summary_keys=("FOPR",),
        times=(0.0, 5.0, 10.0),
        values=[[100.0], [150.0], [200.0]],
    )
    summary = Summary("TEST")
    with pytest.raises(TypeError, match="Expected TimeVector"):
        summary.resample("RESAMPLED", bad_time_points)


def test_that_reading_non_existent_summary_raises_oserror(tmp_path):
    with pytest.raises(OSError, match="Failed to create summary"):
        Summary(str(tmp_path / "does_not_exist"))

    with pytest.raises(OSError, match="No such file"):
        Summary.load(
            str(tmp_path / "does_not_exist.SMSPEC"),
            str(tmp_path / "does_not_exist.UNSMRY"),
        )


def test_that_missing_permissions_raises_oserror(tmp_path):
    (smspec_path := tmp_path / "CASE.SMSPEC").write_text("", encoding="utf-8")
    (unsmry_path := tmp_path / "CASE.UNSMRY").write_text("", encoding="utf-8")
    mode = smspec_path.stat().st_mode
    smspec_path.chmod(0o000)  # no permissions
    try:
        with pytest.raises(OSError, match="Failed to create summary"):
            Summary(str(smspec_path))
        with pytest.raises(OSError, match="Failed to create summary"):
            Summary.load(str(smspec_path), str(unsmry_path))
    finally:
        smspec_path.chmod(mode)


@pytest.mark.usefixtures("use_tmpdir")
def test_add_variable_after_tstep_raises():
    writer = Summary.writer("AFTER_TSTEP", datetime.datetime(2000, 1, 1), 5, 5, 5)
    var = writer.add_variable("FOPT")
    t_step = writer.add_t_step(1, 1.0)
    t_step[var.get_key1()] = 1.0

    with pytest.raises(ValueError, match="interchange variable adding and timesteps"):
        writer.add_variable("FWPT")


@pytest.mark.usefixtures("use_tmpdir")
def test_add_local_variable_after_tstep_raises():
    writer = Summary.writer("AFTER_TSTEP_LGR", datetime.datetime(2000, 1, 1), 5, 5, 5)
    var = writer.add_variable("FOPT")
    t_step = writer.add_t_step(1, 1.0)
    t_step[var.get_key1()] = 1.0

    with pytest.raises(ValueError, match="interchange variable adding and timesteps"):
        writer.add_variable("BLOCK", lgr="LGR1", lgr_ijk=(1, 1, 1))


@pytest.mark.usefixtures("use_tmpdir")
def test_iget_date_with_invalid_index_raises():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))
    summary = Summary("TEST")

    with pytest.raises(ValueError, match="Internal error when looking up index"):
        summary.iget_date(99999)


@pytest.mark.usefixtures("use_tmpdir")
def test_get_from_report_with_invalid_step_raises():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))
    summary = Summary("TEST")

    with pytest.raises(ValueError, match="Internal error when looking up report"):
        summary.get_from_report("FOPR", 99999)


@pytest.mark.usefixtures("use_tmpdir")
def test_get_interp_direct_with_out_of_range_time_raises():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))
    summary = Summary("TEST")

    with pytest.raises(IndexError, match="Invalid time_t instance"):
        summary.get_interp_direct("FOPR", datetime.datetime(2099, 1, 1))


@pytest.mark.usefixtures("use_tmpdir")
def test_alloc_data_vector_with_out_of_range_index_raises():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))
    summary = Summary("TEST")

    with pytest.raises(IndexError, match="Out of range"):
        summary.alloc_data_vector(99999, False)


@pytest.mark.usefixtures("use_tmpdir")
def test_get_general_var_index_with_unknown_key_raises():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))
    summary = Summary("TEST")

    with pytest.raises(IndexError, match="Invalid lookup summary object"):
        summary.get_general_var_index("NO_SUCH_KEY")


@pytest.mark.usefixtures("use_tmpdir")
def test_smspec_keyword_without_nul_terminator_is_read_correctly():
    smspec_kws = [
        ("INTEHEAD", np.array([1, 100], dtype=np.int32)),
        ("RESTART ", ["        "] * 9),
        ("DIMENS  ", np.array([2, 10, 10, 10, 0, 0], dtype=np.int32)),
        ("KEYWORDS", ["TIME    ", "FOPRTEST"]),
        ("WGNAMES ", [":+:+:+:+", ":+:+:+:+"]),
        ("NUMS    ", np.array([-32676, 0], dtype=np.int32)),
        ("UNITS   ", ["DAYS    ", "SM3     "]),
        ("STARTDAT", np.array([1, 1, 2000, 0, 0, 0], dtype=np.int32)),
    ]
    resfo.write("TEST.SMSPEC", smspec_kws)

    unsmry_kws = [
        ("SEQHDR  ", np.array([1], dtype=np.int32)),
        ("MINISTEP", np.array([0], dtype=np.int32)),
        ("PARAMS  ", np.array([1.0, 100.0], dtype=np.float32)),
    ]
    resfo.write("TEST.UNSMRY", unsmry_kws)

    summary = Summary("TEST")
    assert summary.has_key("FOPRTEST")


@pytest.mark.usefixtures("use_tmpdir")
def test_smspec_missing_required_keyword_raises():
    for missing_kw in ("WGNAMES ", "KEYWORDS", "UNITS   ", "DIMENS  "):
        smspec_kws = [
            ("INTEHEAD", np.array([1, 100], dtype=np.int32)),
            ("RESTART ", ["        "] * 9),
            ("DIMENS  ", np.array([2, 10, 10, 10, 0, 0], dtype=np.int32)),
            ("KEYWORDS", ["TIME    ", "FOPR    "]),
            ("WGNAMES ", [":+:+:+:+", ":+:+:+:+"]),
            ("NUMS    ", np.array([-32676, 0], dtype=np.int32)),
            ("UNITS   ", ["DAYS    ", "SM3     "]),
            ("STARTDAT", np.array([1, 1, 2000, 0, 0, 0], dtype=np.int32)),
        ]
        smspec_kws = [(k, v) for k, v in smspec_kws if k != missing_kw]
        resfo.write("TEST.SMSPEC", smspec_kws)
        resfo.write(
            "TEST.UNSMRY",
            [
                ("SEQHDR  ", np.array([1], dtype=np.int32)),
                ("MINISTEP", np.array([0], dtype=np.int32)),
                ("PARAMS  ", np.array([1.0, 100.0], dtype=np.float32)),
            ],
        )

        with pytest.raises((IOError, ValueError)):
            Summary("TEST")


@pytest.mark.usefixtures("use_tmpdir")
def test_smspec_startdat_with_empty_payload_raises():
    smspec_kws = [
        ("INTEHEAD", np.array([1, 100], dtype=np.int32)),
        ("RESTART ", ["        "] * 9),
        ("DIMENS  ", np.array([2, 10, 10, 10, 0, 0], dtype=np.int32)),
        ("KEYWORDS", ["TIME    ", "FOPR    "]),
        ("WGNAMES ", [":+:+:+:+", ":+:+:+:+"]),
        ("NUMS    ", np.array([-32676, 0], dtype=np.int32)),
        ("UNITS   ", ["DAYS    ", "SM3     "]),
        ("STARTDAT", ["NOTANINT"]),
    ]
    resfo.write("TEST.SMSPEC", smspec_kws)
    resfo.write(
        "TEST.UNSMRY",
        [
            ("SEQHDR  ", np.array([1], dtype=np.int32)),
            ("MINISTEP", np.array([0], dtype=np.int32)),
            ("PARAMS  ", np.array([1.0, 100.0], dtype=np.float32)),
        ],
    )

    with pytest.raises((IOError, ValueError)):
        Summary("TEST")


@pytest.mark.usefixtures("use_tmpdir")
def test_unsmry_params_non_float_raises():
    create_summary(summary_keys=("FOPR",), times=(0.0, 1.0, 2.0))

    kws = list(resfo.read("TEST.UNSMRY"))
    patched = []
    for name, value in kws:
        if name.strip() == "PARAMS":
            patched.append((name, value.astype(np.int32)))
        else:
            patched.append((name, value))
    resfo.write("TEST.UNSMRY", patched)

    with pytest.raises((IOError, ValueError)):
        Summary("TEST")
