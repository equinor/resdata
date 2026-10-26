import csv
import datetime
import gc
import os
import os.path
import shutil
import stat
import tempfile

import numpy as np
import pytest
import resfo
from resfo_utilities.testing import (
    Date,
    Simulator,
    Smspec,
    SmspecIntehead,
    SummaryMiniStep,
    SummaryStep,
    Unsmry,
)
from resfo_utilities.testing import (
    UnitSystem as ResfoUnitSystem,
)


def assert_frame_equal(a, b):
    if not a.equals(b):
        raise AssertionError("Expected dataframes to be equal")


try:
    from pandas.testing import assert_frame_equal  # type: ignore
except ImportError:
    pass

from contextlib import contextmanager

from resdata import FileMode, ResDataType, UnitSystem
from resdata.resfile import FortIO, ResdataFile, ResdataKW, openFortIO
from resdata.summary import Summary, SummaryKeyWordVector, SummaryVarType
from resdata.util.util import CTime

from tests import ResdataTest
from tests.util.mock import createSummary


@contextmanager
def pushd(path):
    if not os.path.isdir(path):
        os.makedirs(path)
    cwd = os.getcwd()
    os.chdir(path)

    yield

    os.chdir(cwd)


def create_prediction(history, pred_path):
    restart_case = os.path.join(os.getcwd(), history.base)
    restart_step = history.last_report
    with pushd(pred_path):
        prediction = create_case(
            case="PREDICTION",
            restart_case=restart_case,
            restart_step=restart_step,
            data_start=history.end_date,
        )
        prediction.fwrite()


def fopr(days):
    return days


def fopt(days):
    return days


def fgpt(days):
    if days < 50:
        return days
    else:
        return 100 - days


def create_case(case="CSV", restart_case=None, restart_step=-1, data_start=None):
    length = 100
    return createSummary(
        case,
        [
            ("FOPT", None, 0, "SM3"),
            ("FOPR", None, 0, "SM3/DAY"),
            ("FGPT", None, 0, "SM3"),
        ],
        sim_length_days=length,
        num_report_step=10,
        num_mini_step=10,
        data_start=data_start,
        func_table={"FOPT": fopt, "FOPR": fopr, "FGPT": fgpt},
        restart_case=restart_case,
        restart_step=restart_step,
    )


def create_case2(case="CSV", restart_case=None, restart_step=-1, data_start=None):
    length = 100
    return createSummary(
        case,
        [
            ("WOPT", "OPX", 0, "SM3"),
            ("FOPR", None, 0, "SM3/DAY"),
            ("BPR", None, 10, "SM3"),
            ("RPR", None, 3, "BARS"),
            ("COPR", "OPX", 421, "BARS"),
        ],
        sim_length_days=length,
        num_report_step=10,
        num_mini_step=10,
        data_start=data_start,
        func_table={"FOPT": fopt, "FOPR": fopr, "FGPT": fgpt},
        restart_case=restart_case,
        restart_step=restart_step,
    )


class SumTest(ResdataTest):
    def test_mock(self):
        case = createSummary(
            "CSV", [("FOPT", None, 0, "SM3"), ("FOPR", None, 0, "SM3/DAY")]
        )
        self.assertTrue("FOPT" in case)
        self.assertFalse("WWCT:OPX" in case)

    def test_TIME_special_case(self):
        case = createSummary(
            "CSV", [("FOPT", None, 0, "SM3"), ("FOPR", None, 0, "SM3/DAY")]
        )
        keys = case.keys()
        self.assertEqual(len(keys), 2)
        self.assertIn("FOPT", keys)
        self.assertIn("FOPR", keys)

        keys = case.keys(pattern="*")
        self.assertEqual(len(keys), 2)
        self.assertIn("FOPT", keys)
        self.assertIn("FOPR", keys)

    def test_identify_var_type(self):
        self.assertEnumIsFullyDefined(
            SummaryVarType, "rd_smspec_var_type", "lib/include/resdata/smspec_node.hpp"
        )
        self.assertEqual(
            Summary.var_type("WWCT:OP_X"), SummaryVarType.RD_SMSPEC_WELL_VAR
        )
        self.assertEqual(Summary.var_type("RPR"), SummaryVarType.RD_SMSPEC_REGION_VAR)
        self.assertEqual(Summary.var_type("WNEWTON"), SummaryVarType.RD_SMSPEC_MISC_VAR)
        self.assertEqual(Summary.var_type("YEAR"), SummaryVarType.RD_SMSPEC_MISC_VAR)
        self.assertEqual(Summary.var_type("MONTH"), SummaryVarType.RD_SMSPEC_MISC_VAR)
        self.assertEqual(
            Summary.var_type("AARQ:4"), SummaryVarType.RD_SMSPEC_AQUIFER_VAR
        )

        self.assertEqual(
            Summary.var_type("RXFT"), SummaryVarType.RD_SMSPEC_REGION_2_REGION_VAR
        )
        self.assertEqual(
            Summary.var_type("RxxFT"), SummaryVarType.RD_SMSPEC_REGION_2_REGION_VAR
        )
        self.assertEqual(
            Summary.var_type("RXFR"), SummaryVarType.RD_SMSPEC_REGION_2_REGION_VAR
        )
        self.assertEqual(
            Summary.var_type("RxxFR"), SummaryVarType.RD_SMSPEC_REGION_2_REGION_VAR
        )
        self.assertEqual(Summary.var_type("RORFR"), SummaryVarType.RD_SMSPEC_REGION_VAR)

        case = createSummary(
            "CSV",
            [
                ("FOPT", None, 0, "SM3"),
                ("FOPR", None, 0, "SM3/DAY"),
                ("AARQ", None, 10, "???"),
                ("RGPT", None, 1, "SM3"),
            ],
        )

        node1 = case.smspec_node("FOPT")
        self.assertEqual(node1.var_type(), SummaryVarType.RD_SMSPEC_FIELD_VAR)
        self.assertIsNone(node1.wgname)

        node2 = case.smspec_node("AARQ:10")
        self.assertEqual(node2.var_type(), SummaryVarType.RD_SMSPEC_AQUIFER_VAR)
        self.assertEqual(node2.get_num(), 10)
        self.assertFalse(node2.is_rate())

        node3 = case.smspec_node("RGPT:1")
        self.assertEqual(node3.var_type(), SummaryVarType.RD_SMSPEC_REGION_VAR)
        self.assertEqual(node3.get_num(), 1)
        self.assertEqual(node3.num, 1)
        self.assertEqual(node3.default, 0)
        self.assertTrue(node3.is_total())
        self.assertFalse(node3.is_historical())

        self.assertEqual(node3.keyword, "RGPT")

        self.assertLess(node1, node3)
        self.assertGreater(node2, node3)
        self.assertEqual(node1, node1)
        self.assertNotEqual(node1, node2)

        with self.assertRaises(TypeError):
            a = node1 < 1

    def test_csv_export(self):
        case = createSummary(
            "CSV", [("FOPT", None, 0, "SM3"), ("FOPR", None, 0, "SM3/DAY")]
        )
        sep = ";"
        tmpdir = self.tmp_path_factory.mktemp("resdata_csv", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            case.export_csv("file.csv", sep=sep)
            self.assertTrue(os.path.isfile("file.csv"))
            input_file = csv.DictReader(open("file.csv"), delimiter=sep)
            for row in input_file:
                self.assertIn("DAYS", row)
                self.assertIn("DATE", row)
                self.assertIn("FOPT", row)
                self.assertIn("FOPR", row)
                self.assertEqual(len(row), 4)
                break

            self.assertEqual(case.unit("FOPT"), "SM3")

        tmpdir = self.tmp_path_factory.mktemp("resdata_csv", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            case.export_csv("file.csv", keys=["FOPT"], sep=sep)
            self.assertTrue(os.path.isfile("file.csv"))
            input_file = csv.DictReader(open("file.csv"), delimiter=sep)
            for row in input_file:
                self.assertIn("DAYS", row)
                self.assertIn("DATE", row)
                self.assertIn("FOPT", row)
                self.assertEqual(len(row), 3)
                break

        tmpdir = self.tmp_path_factory.mktemp("resdata_csv", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            date_format = "%y-%m-%d"
            sep = ","
            case.export_csv("file.csv", keys=["F*"], sep=sep, date_format=date_format)
            self.assertTrue(os.path.isfile("file.csv"))
            with open("file.csv") as f:
                time_index = -1
                for line in f:
                    tmp = line.split(sep)
                    self.assertEqual(len(tmp), 4)

                    if time_index >= 0:
                        d = datetime.datetime.strptime(tmp[1], date_format)
                        self.assertEqual(case.iget_date(time_index), d)

                    time_index += 1

    def test_solve(self):
        length = 100
        case = create_case()
        self.assert_solve(case)

    def assert_solve(self, case):
        with self.assertRaises(KeyError):
            case.solve_days("MISSING:KEY", 0.56)

        sol = case.solve_days("FOPT", 150)
        self.assertEqual(len(sol), 0)

        sol = case.solve_days("FOPT", -10)
        self.assertEqual(len(sol), 0)

        sol = case.solve_days("FOPT", 50)
        self.assertEqual(len(sol), 1)
        self.assertFloatEqual(sol[0], 50)

        sol = case.solve_days("FOPT", 50.50)
        self.assertEqual(len(sol), 1)
        self.assertFloatEqual(sol[0], 50.50)

        sol = case.solve_days("FOPR", 50.90)
        self.assertEqual(len(sol), 1)
        self.assertFloatEqual(sol[0], 50.00 + 1.0 / 86400)

        sol = case.solve_dates("FOPR", 50.90)
        t = (
            case.get_data_start_time()
            + datetime.timedelta(days=50)
            + datetime.timedelta(seconds=1)
        )
        self.assertEqual(sol[0], t)

        sol = case.solve_days("FOPR", 50.90, rates_clamp_lower=False)
        self.assertEqual(len(sol), 1)
        self.assertFloatEqual(sol[0], 51.00)

        sol = case.solve_days("FGPT", 25.0)
        self.assertEqual(len(sol), 2)
        self.assertFloatEqual(sol[0], 25.00)
        self.assertFloatEqual(sol[1], 75.00)

        sol = case.solve_dates("FGPT", 25)
        self.assertEqual(len(sol), 2)
        t0 = case.get_data_start_time()
        t1 = t0 + datetime.timedelta(days=25)
        t2 = t0 + datetime.timedelta(days=75)
        self.assertEqual(sol[0], t1)
        self.assertEqual(sol[1], t2)

    def test_different_names(self):
        case = create_case()
        tmpdir = self.tmp_path_factory.mktemp("sum_different", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            case.fwrite()
            shutil.move("CSV.SMSPEC", "CSVX.SMSPEC")
            with self.assertRaises(IOError):
                case2 = Summary.load("Does/not/exist", "CSV.UNSMRY")

            with self.assertRaises(IOError):
                case2 = Summary.load("CSVX.SMSPEC", "CSVX.UNSMRY")

            case2 = Summary.load("CSVX.SMSPEC", "CSV.UNSMRY")
            self.assert_solve(case2)
            self.assertEqual(case.unit("FOPR"), "SM3/DAY")

    def test_invalid(self):
        case = create_case()
        tmpdir = self.tmp_path_factory.mktemp("sum_invalid", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            case.fwrite()
            with open("CASE.txt", "w") as f:
                f.write("No - this is not ResdataKW file ....")

            with self.assertRaises(IOError):
                case2 = Summary.load("CSV.SMSPEC", "CASE.txt")

            with self.assertRaises(IOError):
                case2 = Summary.load("CASE.txt", "CSV.UNSMRY")

            kw1 = ResdataKW("TEST1", 30, ResDataType.RD_INT)
            kw2 = ResdataKW("TEST2", 30, ResDataType.RD_INT)

            with openFortIO("CASE.KW", FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                kw2.fwrite(f)

            with self.assertRaises(IOError):
                case2 = Summary.load("CSV.SMSPEC", "CASE.KW")

            with self.assertRaises(IOError):
                case2 = Summary.load("CASE.KW", "CSV.UNSMRY")

    def test_kw_vector(self):
        case1 = create_case()
        case2 = createSummary(
            "CSV",
            [
                ("FOPR", None, 0, "SM3/DAY"),
                ("FOPT", None, 0, "SM3"),
                ("FWPT", None, 0, "SM3"),
            ],
            sim_length_days=100,
            num_report_step=10,
            num_mini_step=10,
            func_table={"FOPT": fopt, "FOPR": fopr, "FWPT": fgpt},
        )
        self.assertEqual(case2.get_key_index("FOPR"), 1)

        kw_list = SummaryKeyWordVector(case1)
        kw_list.add_keyword("FOPT")
        kw_list.add_keyword("FGPT")
        kw_list.add_keyword("FOPR")

        t = case1.get_data_start_time() + datetime.timedelta(days=43)
        data = case1.get_interp_row(kw_list, t)
        for d1, d2 in zip(
            data,
            [
                case1.get_interp("FOPT", date=t),
                case1.get_interp("FOPT", date=t),
                case1.get_interp("FOPT", date=t),
            ],
        ):
            self.assertFloatEqual(d1, d2)

        tmp = []
        for key in kw_list:
            tmp.append(key)

        for k1, k2 in zip(kw_list, tmp):
            self.assertEqual(k1, k2)

        kw_list2 = kw_list.copy(case2)
        self.assertIn("FOPT", kw_list2)
        self.assertIn("FOPR", kw_list2)
        self.assertIn("FGPT", kw_list2)
        data2 = case2.get_interp_row(kw_list2, t)

        self.assertEqual(len(data2), 3)
        self.assertEqual(data[0], data2[0])
        self.assertEqual(data[2], data2[2])

        tmpdir = self.tmp_path_factory.mktemp("sum_vector", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            with open("f1.txt", "w") as f:
                case1.dump_csv_line(t, kw_list, f)

            with open("f2.txt", "w") as f:
                case2.dump_csv_line(t, kw_list2, f)

            with open("f1.txt") as f:
                d1 = f.readline().split(",")

            with open("f2.txt") as f:
                d2 = f.readline().split(",")

            self.assertEqual(d1[0], d2[0])
            self.assertEqual(d1[2], d2[2])
            self.assertEqual(d2[1], "")

    def test_vector_select_all(self):
        case = create_case()
        rd_sum_vector = SummaryKeyWordVector(case, True)
        keys = case.keys()
        self.assertEqual(len(keys), len(rd_sum_vector))
        for key in keys:
            self.assertIn(key, rd_sum_vector)

    def test_first_last(self):
        case = create_case()
        with self.assertRaises(KeyError):
            case.last_value("NO_SUCH_KEY")
        last_fopt = case.last_value("FOPT")
        values = list(case.numpy_vector("FOPT"))
        self.assertEqual(last_fopt, values[-1])

        with self.assertRaises(KeyError):
            case.first_value("NO_SUCH_KEY")

        first_fopt = case.first_value("FOPT")
        self.assertEqual(first_fopt, values[0])

    def test_time_range(self):
        case = create_case()
        with self.assertRaises(ValueError):
            case.time_range(num_timestep=1)

        time_range = case.time_range(num_timestep=10)
        self.assertEqual(len(time_range), 10)
        self.assertEqual(time_range[0], case.get_data_start_time())
        self.assertEqual(time_range[-1], case.get_end_time())

        # Cover clamping when start is before data_start_time and end is after end_time.
        data_start = case.get_data_start_time()
        data_end = case.get_end_time()
        early_start = data_start - datetime.timedelta(days=10)
        late_end = data_end + datetime.timedelta(days=10)
        clamped = case.time_range(start=early_start, end=late_end, num_timestep=5)
        self.assertEqual(clamped[0], data_start)
        self.assertEqual(clamped[-1], data_end)

    def test_time_range_yearly_does_not_extend_when_end_is_at_year_start(self):
        case = createSummary(
            "CSV",
            [("FOPT", None, 0, "SM3")],
            sim_start=datetime.date(2010, 1, 1),
            sim_length_days=3 * 365,
            num_report_step=10,
            num_mini_step=10,
        )

        # The end date is already at the start of a year, so there is
        # nothing to extend.
        trange = case.time_range(
            end=datetime.datetime(2012, 1, 1), interval="1Y", extend_end=True
        )
        self.assertEqual(
            [t.datetime() for t in trange],
            [
                datetime.datetime(2010, 1, 1),
                datetime.datetime(2011, 1, 1),
                datetime.datetime(2012, 1, 1),
            ],
        )

        # An end date within a year is extended to the start of the next year.
        trange = case.time_range(
            end=datetime.datetime(2012, 6, 1), interval="1Y", extend_end=True
        )
        self.assertEqual(
            [t.datetime() for t in trange],
            [
                datetime.datetime(2010, 1, 1),
                datetime.datetime(2011, 1, 1),
                datetime.datetime(2012, 1, 1),
                datetime.datetime(2013, 1, 1),
            ],
        )

    def test_resample(self):
        case = create_case()
        time_vector = case.alloc_time_vector(False)
        case2 = case.resample("RS", time_vector.tolist())
        time_vector_resample = case2.alloc_time_vector(False)
        self.assertEqual(list(time_vector_resample), list(time_vector))

    # The purpose of this test is to reproduce a slightly contrived error situation.
    #
    # 1. A history simulation is created and stored somewhere in the
    #    filesystem.
    #
    # 2. We create a prediction, which has 'RESTART' reference to
    #    the history case.
    #
    # 3. The prediction case is loaded from disk, with a cwd different from the
    #    location of the predition case.
    #
    # This configuration would previously lead to a bug in the path used to
    # resolve the history case, and the history would silently be ignored.

    def test_restart_abs_path(self):
        tmpdir = self.tmp_path_factory.mktemp("restart_test", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            history = create_case(case="HISTORY")
            history.fwrite()

            pred_path = "prediction"
            create_prediction(history, pred_path)

            pred = Summary(os.path.join(pred_path, "PREDICTION"))
            # The restart case has a maximum length of 72 characters, depending
            # on the path used for $TMP and so on we do not really know here if
            # the restart_case has been set or not.
            if pred.restart_case:
                self.assertTrue(isinstance(pred.restart_case, Summary))
                self.assertEqual(
                    pred.restart_case.case, os.path.join(os.getcwd(), history.case)
                )
                self.assertEqual(pred.restart_step, history.last_report)

                length = pred.sim_length
                pred_times = pred.alloc_time_vector(False)
                hist_times = history.alloc_time_vector(False)

                for index in range(len(hist_times)):
                    self.assertEqual(hist_times[index], pred_times[index])

    def test_restart_too_long_history_path(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "restart_test_too_long_path_for_the_eclipse_restart_keyword_1234567890123456789012345678901234567890"
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            history = create_case(case="HISTORY")
            history.fwrite()

            pred_path = "prediction"
            create_prediction(history, pred_path)

            pred = Summary(os.path.join(pred_path, "PREDICTION"))
            self.assertIsNone(pred.restart_case)

    def test_restart_perm_denied(self):
        tmpdir = self.tmp_path_factory.mktemp("restart_test", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            with pushd("history/case1"):
                history = create_case(case="HISTORY")
                history.fwrite()

            prediction = create_case(
                case="PREDICTION",
                restart_case="history/case1/HISTORY",
                data_start=history.end_date,
            )
            prediction.fwrite()

            os.chmod("history", 0)

            # This just tests that we can create a summary instance even if we do not
            # have access to load the history case.
            pred = Summary("PREDICTION")

            os.chmod("history", stat.S_IRWXU)

    def test_units(self):
        case = create_case()
        self.assertEqual(case.unit_system, UnitSystem.METRIC)

        # We do not really have support for writing anything else than the
        # default MERIC unit system. To be able to test the read functionality
        # we therefor monkey-patch the summary files in place.
        tmpdir = self.tmp_path_factory.mktemp("unit_test", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            case = create_case("UNITS")
            case.fwrite()
            case2 = Summary("UNITS")

            kw_list = []
            f = ResdataFile("UNITS.SMSPEC")
            for kw in f:
                if kw.name == "INTEHEAD":
                    kw[0] = 3
                kw_list.append(kw.copy())

            f.close()
            with openFortIO("UNITS.SMSPEC", mode=FortIO.WRITE_MODE) as f:
                for kw in kw_list:
                    kw.fwrite(f)

            case = Summary("UNITS")
            self.assertEqual(case.unit_system, UnitSystem.LAB)

    def test_load_non_unified_case_via_summary_extension(self):
        tmpdir = self.tmp_path_factory.mktemp("non_unified", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            rd_sum = Summary.writer(
                "CASE", datetime.date(2010, 1, 1), 3, 3, 3, unified=False
            )
            rd_sum.add_variable("FOPT", num=0, unit="SM3")
            for report_step in range(1, 4):
                t_step = rd_sum.add_t_step(report_step, sim_days=float(report_step))
                t_step["FOPT"] = report_step * 100.0
            rd_sum.fwrite()

            # A non-unified case is written as separate CASE.Snnnn files.
            self.assertTrue(os.path.exists("CASE.S0001"))
            self.assertFalse(os.path.exists("CASE.UNSMRY"))

            # Passing a path whose extension is a non-unified summary file
            # (.S0001) exercises the non-unified branch of the case loader.
            case = Summary("CASE.S0001")
            self.assertEqual(len(case), 3)
            self.assertEqual(case.numpy_vector("FOPT")[-1], 300.0)

    def test_numpy_vector(self):
        case = create_case()

        with self.assertRaises(KeyError):
            case.numpy_vector("NO_SUCH_KEY")

        numpy_vector = case.numpy_vector("FOPT")
        self.assertEqual(len(numpy_vector), len(case))
        numpy_dates = case.numpy_dates
        self.assertEqual(numpy_dates[0].tolist(), case.get_data_start_time())
        self.assertEqual(numpy_dates[-1].tolist(), case.get_end_time())

        dates = case.dates
        self.assertEqual(dates[0], case.get_data_start_time())
        self.assertEqual(dates[-1], case.get_end_time())

        dates = (
            [datetime.datetime(2000, 1, 1)]
            + case.dates
            + [datetime.datetime(2020, 1, 1)]
        )
        fopr = case.numpy_vector("FOPR", time_index=dates)
        fopt = case.numpy_vector("FOPT", time_index=dates)
        self.assertEqual(len(fopt), len(dates))

        self.assertEqual(fopr[0], 0)
        self.assertEqual(fopr[-1], 0)

        self.assertEqual(fopt[0], 0)
        self.assertEqual(fopt[0], case.first_value("FOPT"))
        self.assertEqual(fopt[-1], case.last_value("FOPT"))

        with self.assertRaises(ValueError):
            v = case.numpy_vector("FOPR", time_index=dates, report_only=True)

        v = case.numpy_vector("FOPR", report_only=True)
        self.assertEqual(len(v), len(case.report_dates))

    def test_wells_and_groups(self):
        case = create_case()
        self.assertEqual(case.wells(), [])
        self.assertEqual(case.groups(), [])

    def test_csv_load(self):
        case = create_case2()
        frame = case.pandas_frame()
        rd_sum = Summary.from_pandas("PANDAS", frame, dims=[20, 10, 5])

        for key in frame.columns:
            self.assertTrue(key in rd_sum)

        df = rd_sum.pandas_frame()
        assert_frame_equal(frame, df)

        rd_sum_less = Summary.from_pandas(
            "PANDAS",
            frame,
            dims=[20, 10, 5],
            headers=["BPR:10", "RPR:3", "COPR:OPX:1,2,3"],
        )
        del frame["WOPT:OPX"]
        del frame["FOPR"]
        df_less = rd_sum_less.pandas_frame()
        assert_frame_equal(frame, df_less)

    def test_total_and_rate(self):
        self.assertTrue(Summary.is_total("FOPT"))
        self.assertTrue(Summary.is_total("WWPT:OP_3"))
        self.assertFalse(Summary.is_total("RPR:2"))

        self.assertTrue(Summary.is_rate("WOPR:OP_4"))
        self.assertFalse(Summary.is_rate("BPR:123"))
        self.assertTrue(Summary.is_rate("FWIR"))

    def test_load_case(self):
        path = os.path.join(
            self.TESTDATA_ROOT, "local/ECLIPSE/cp_simple3/SIMPLE_SUMMARY3"
        )
        case = Summary(path)
        self.assertFloatEqual(case.sim_length, 545.0)

        fopr = case.numpy_vector("FOPR")
        for time_index, value in enumerate(fopr):
            self.assertEqual(fopr[time_index], value)

    def test_load_case_lazy_and_eager(self):
        path = os.path.join(self.TESTDATA_ROOT, "local/ECLIPSE/cp_simple3/SHORT.UNSMRY")
        lazy_dates = Summary(path, lazy_load=True).numpy_dates
        eager_dates = Summary(path, lazy_load=False).numpy_dates
        self.assertEqual(len(lazy_dates), 107)
        self.assertEqual(len(eager_dates), 107)
        for l, e in zip(lazy_dates, eager_dates):
            self.assertEqual(l, e)

    def test_write_not_implemented(self):
        path = os.path.join(
            self.TESTDATA_ROOT, "local/ECLIPSE/cp_simple3/SIMPLE_SUMMARY3"
        )
        case = Summary(path, lazy_load=True)
        self.assertFalse(case.can_write())
        with self.assertRaises(NotImplementedError):
            case.fwrite()

    def test_directory_conflict(self):
        tmpdir = self.tmp_path_factory.mktemp("dir_conflict", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            case = create_case("UNITS")
            case.fwrite()
            os.mkdir("UNITS")
            case2 = Summary("./UNITS")

    def test_resample_extrapolate(self):
        """
        Test resampling of summary with extrapolate option of lower and upper boundaries enabled
        """

        path = os.path.join(
            self.TESTDATA_ROOT, "local/ECLIPSE/cp_simple3/SIMPLE_SUMMARY3"
        )
        rd_sum = Summary(path, lazy_load=True)

        start_time = rd_sum.get_data_start_time() - datetime.timedelta(seconds=86400)
        end_time = rd_sum.get_end_time() + datetime.timedelta(seconds=86400)
        delta = end_time - start_time

        N = 25
        step_len = delta.total_seconds() / (N - 1)
        time_points = [
            start_time + datetime.timedelta(seconds=i * step_len) for i in range(N - 1)
        ]
        time_points.append(end_time)
        resampled = rd_sum.resample(
            "OUTPUT_CASE",
            time_points,
            lower_extrapolation=True,
            upper_extrapolation=True,
        )

        for key in rd_sum.keys():
            self.assertIn(key, resampled)

        self.assertEqual(
            rd_sum.get_data_start_time() - datetime.timedelta(seconds=86400),
            resampled.get_data_start_time(),
        )

        key_not_rate = "FOPT"
        for time_index, t in enumerate(time_points):
            if t < rd_sum.get_data_start_time():
                self.assertFloatEqual(
                    resampled.iget(key_not_rate, time_index),
                    rd_sum.first_value(key_not_rate),
                )
            elif t > rd_sum.get_end_time():
                self.assertFloatEqual(
                    resampled.iget(key_not_rate, time_index),
                    rd_sum.last_value(key_not_rate),
                )
            else:
                self.assertFloatEqual(
                    resampled.iget(key_not_rate, time_index),
                    rd_sum.get_interp_direct(key_not_rate, t),
                )

        key_rate = "FOPR"
        for time_index, t in enumerate(time_points):
            if t < rd_sum.get_data_start_time():
                self.assertFloatEqual(resampled.iget(key_rate, time_index), 0)
            elif t > rd_sum.get_end_time():
                self.assertFloatEqual(resampled.iget(key_rate, time_index), 0)
            else:
                self.assertFloatEqual(
                    resampled.iget(key_rate, time_index),
                    rd_sum.get_interp_direct(key_rate, t),
                )


@pytest.mark.parametrize("time_index_type", [list, tuple])
def test_summary_to_pandas_frame(time_index_type):
    case = create_case()
    dates = time_index_type(
        [datetime.datetime(2000, 1, 1)] + case.dates + [datetime.datetime(2020, 1, 1)]
    )
    frame = case.pandas_frame(column_keys=["FOPT", "FOPR"], time_index=dates)

    fopr = frame["FOPR"]
    fopt = frame["FOPT"]

    assert fopr.iloc[0] == 0
    assert fopr.iloc[-1] == 0

    assert fopt.iloc[0] == 0
    assert fopt.iloc[0] == case.first_value("FOPT")
    assert fopt.iloc[-1] == case.last_value("FOPT")

    frame = case.pandas_frame()
    rows, columns = frame.shape
    assert len(case.keys()) == columns
    assert len(case) == rows


def test_t_step():
    summary = createSummary(
        "CASE",
        [
            ("FOPT", None, 0, "SM3"),
            ("FOPR", None, 0, "SM3/DAY"),
            ("FGPT", None, 0, "SM3"),
        ],
        sim_length_days=100,
        num_report_step=10,
        num_mini_step=10,
        sim_start=datetime.date(2010, 1, 1),
        func_table={"FOPT": fopt, "FOPR": fopr, "FGPT": fgpt},
    )
    assert summary.path is None
    assert summary.end_time.date() == summary.end_date

    t_step = summary.add_t_step(11, 101)
    assert t_step.get_report() == 11
    assert t_step.get_sim_days() == 101
    assert t_step.get_mini_step() == 10 * 10
    assert summary.iget_days(100) == 101
    assert "FOPT" in t_step
    assert t_step["FOPT"] == 0.0
    t_step["FOPT"] = 100.0
    assert t_step["FOPT"] == 100.0
    assert t_step.get_sim_time().datetime() == datetime.datetime(2010, 4, 12, 0, 0)

    node = summary.get_last("FOPT")
    assert node.days == 101
    assert summary.get_report(days=101) == 11

    assert list(summary.get_interp_vector("FOPT", days_list=[1, 10, 20, 30, 101])) == [
        1.0,
        10.0,
        20.0,
        30.0,
        100.0,
    ]

    assert summary.get_from_report("FOPT", 11) == 100.0
    assert summary.first_gt("FOPT", 10).days == 11
    assert summary.first_lt("FOPT", 10).days == 0
    assert summary.first_gt_index("FOPT", 10) == 11
    assert summary.first_lt_index("FOPT", 10) == 0

    assert summary.get_last("FOPT").days == 101.0
    assert summary["FOPT"].first.days == 0


def test_that_end_time_is_sim_start_when_there_are_no_steps():
    summary = createSummary(
        "CASE",
        [],
        sim_length_days=100,
        num_report_step=0,
        num_mini_step=0,
        sim_start=datetime.date(2010, 1, 1),
        func_table={},
    )
    assert summary.end_date == summary.start_date
    # Note that in this situation
    # sum.data_start will raise


def _write_day_month_year_case(case, dates, values):
    """Write a summary whose only time information is DAY/MONTH/YEAR.

    The simulation start is taken from the first date, and a single FOPT
    keyword carries `values`.
    """
    smspec = Smspec(
        nx=2,
        ny=2,
        nz=2,
        restarted_from_step=0,
        num_keywords=4,
        restart="        ",
        keywords=["DAY     ", "MONTH   ", "YEAR    ", "FOPT    "],
        well_names=[":+:+:+:+", ":+:+:+:+", ":+:+:+:+", "A_NAME  "],
        region_numbers=[-32676, -32676, -32676, 0],
        units=["        ", "        ", "        ", "SM3"],
        start_date=Date(
            day=dates[0].day,
            month=dates[0].month,
            year=dates[0].year,
            hour=0,
            minutes=0,
            micro_seconds=0,
        ),
        intehead=SmspecIntehead(
            unit=ResfoUnitSystem.METRIC,
            simulator=Simulator.ECLIPSE_100,
        ),
    )
    unsmry = Unsmry(
        steps=[
            SummaryStep(
                seqnum=i,
                ministeps=[
                    SummaryMiniStep(
                        mini_step=i,
                        params=[
                            float(date.day),
                            float(date.month),
                            float(date.year),
                            value,
                        ],
                    )
                ],
            )
            for i, (date, value) in enumerate(zip(dates, values))
        ]
    )
    smspec.to_file(f"{case}.SMSPEC")
    unsmry.to_file(f"{case}.UNSMRY")


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_a_summary_with_day_month_year_instead_of_time_is_read_correctly(
    lazy_load,
):
    case = "DAY_MONTH_YEAR_CASE"
    expected_dates = [
        datetime.datetime(2010, 1, 1),
        datetime.datetime(2010, 3, 15),
    ]
    expected_values = [10.0, 20.0]
    _write_day_month_year_case(case, expected_dates, expected_values)

    summary = Summary(case, lazy_load=lazy_load)
    assert summary.dates == expected_dates
    assert list(summary["FOPT"].values) == expected_values
    assert summary.start_date == datetime.date(2010, 1, 1)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_sim_length_of_a_day_month_year_summary_is_measured_in_days(lazy_load):
    """
    sim_length is the elapsed simulation time expressed in the unit of the
    TIME keyword. A summary without a TIME keyword has no unit of its own, so
    it is reported in days.
    """
    case = "DAY_MONTH_YEAR_LENGTH_CASE"
    _write_day_month_year_case(
        case,
        [datetime.datetime(2010, 1, 1), datetime.datetime(2010, 3, 15)],
        [10.0, 20.0],
    )

    summary = Summary(case, lazy_load=lazy_load)
    assert summary.sim_length == pytest.approx(73.0)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_a_timestep_cannot_be_added_to_a_day_month_year_summary(lazy_load):
    case = "DAY_MONTH_YEAR_APPEND_CASE"
    _write_day_month_year_case(case, [datetime.datetime(2010, 1, 1)], [10.0])

    summary = Summary(case, lazy_load=lazy_load)
    with pytest.raises(ValueError, match="without a TIME variable"):
        summary.add_t_step(1, 10.0)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize(
    "date_keywords",
    [
        pytest.param([], id="no_time_keywords"),
        pytest.param(["DAY     ", "MONTH   "], id="day_and_month_but_no_year"),
    ],
)
def test_that_a_summary_lacking_time_information_is_rejected(lazy_load, date_keywords):
    """
    Time information must come either from TIME or from a complete set of
    DAY/MONTH/YEAR keywords. A partial set of date keywords is not usable,
    so such a case must be rejected rather than silently read with a
    wrong/default params index.
    """
    case = "NO_TIME_CASE"
    summary_keys = ["FOPT"]
    keywords = [*date_keywords, *summary_keys]

    smspec = Smspec(
        nx=2,
        ny=2,
        nz=2,
        restarted_from_step=0,
        num_keywords=len(keywords),
        restart="        ",
        keywords=keywords,
        well_names=[
            *([":+:+:+:+"] * len(date_keywords)),
            *(["A_NAME  "] * len(summary_keys)),
        ],
        region_numbers=[
            *([-32676] * len(date_keywords)),
            *([0] * len(summary_keys)),
        ],
        units=[
            *(["        "] * len(date_keywords)),
            *(["SM3"] * len(summary_keys)),
        ],
        start_date=Date(day=1, month=1, year=2010, hour=0, minutes=0, micro_seconds=0),
        intehead=SmspecIntehead(
            unit=ResfoUnitSystem.METRIC,
            simulator=Simulator.ECLIPSE_100,
        ),
    )
    unsmry = Unsmry(
        steps=[
            SummaryStep(
                seqnum=0,
                ministeps=[
                    SummaryMiniStep(
                        mini_step=0,
                        params=[*([1.0] * len(date_keywords)), 10.0],
                    )
                ],
            )
        ]
    )
    smspec.to_file(f"{case}.SMSPEC")
    unsmry.to_file(f"{case}.UNSMRY")

    with pytest.raises(ValueError, match="lack time information"):
        Summary(case, lazy_load=lazy_load)


def test_that_summary_keyword_vector_lifetime_is_longer_than_summary():
    """A regression test for a bug where SummaryKeyWordVector outlived its Summary"""

    def _create():
        summary = createSummary(
            "CASE",
            [("FOPR", None, 0, "SM3")],
            sim_length_days=100,
            num_report_step=0,
            num_mini_step=0,
            sim_start=datetime.date(2010, 1, 1),
            func_table={},
        )
        return SummaryKeyWordVector(summary)

    vector = _create()
    gc.collect()
    vector.add_keywords("FOPR")
    assert len(vector) == 1
    assert vector[0] == "FOPR"


NUM_REPORTS = 4
NUM_MINISTEPS = 3


def _write_case(
    case,
    *,
    unified=True,
    fmt_output=False,
    num_reports=NUM_REPORTS,
    num_ministeps=NUM_MINISTEPS,
    start=datetime.datetime(2010, 1, 1),
):
    """Writes FOPT=10*days, FOPR=100-days and WOPR:OP1=days on a case with
    `num_reports` report steps, each of `num_ministeps` daily ministeps."""
    writer = Summary.writer(
        case, start, 5, 5, 5, fmt_output=fmt_output, unified=unified
    )
    writer.add_variable("FOPT", unit="SM3")
    writer.add_variable("FOPR", unit="SM3/DAY")
    writer.add_variable("WOPR", wgname="OP1", unit="SM3/DAY")
    days = 0
    for report in range(1, num_reports + 1):
        for _ in range(num_ministeps):
            tstep = writer.add_t_step(report, days)
            tstep["FOPT"] = 10.0 * days
            tstep["FOPR"] = 100.0 - days
            tstep["WOPR:OP1"] = float(days)
            days += 1
    writer.fwrite()
    return writer


ALL_FILE_LAYOUTS = pytest.mark.parametrize(
    "unified,fmt_output,lazy_load",
    [
        (unified, fmt_output, lazy_load)
        for unified in (True, False)
        for fmt_output in (True, False)
        for lazy_load in (True, False)
    ],
)


@pytest.mark.usefixtures("use_tmpdir")
@ALL_FILE_LAYOUTS
@pytest.mark.parametrize(
    "open_mode", (FileMode.DEFAULT, FileMode.CLOSE_STREAM, FileMode.WRITABLE)
)
def test_that_summary_roundtrips(unified, fmt_output, lazy_load, open_mode):
    _write_case("LAYOUT", unified=unified, fmt_output=fmt_output)

    summary = Summary("LAYOUT", lazy_load=lazy_load, file_options=open_mode)

    total = NUM_REPORTS * NUM_MINISTEPS
    assert len(summary) == total
    assert summary.days == [float(d) for d in range(total)]
    assert list(summary["FOPT"].values) == pytest.approx(
        [10.0 * d for d in range(total)]
    )
    assert list(summary["WOPR:OP1"].values) == pytest.approx(
        [float(d) for d in range(total)]
    )
    assert summary.report_step == [
        r for r in range(1, NUM_REPORTS + 1) for _ in range(NUM_MINISTEPS)
    ]
    assert summary.first_report == 1
    assert summary.last_report == NUM_REPORTS
    assert summary.start_date == datetime.date(2010, 1, 1)
    assert summary.end_date == datetime.date(2010, 1, 1) + datetime.timedelta(
        days=total - 1
    )
    assert summary.case == "LAYOUT"
    assert summary.base == "LAYOUT"
    assert summary.path is None
    assert summary.abs_path == os.getcwd()
    assert summary.unit("FOPR") == "SM3/DAY"
    assert summary.restart_case is None
    assert summary.restart_step == -1
    assert summary.length == NUM_REPORTS * NUM_MINISTEPS
    assert summary.first_value("FOPT") == 0.0
    assert summary.last_value("FOPT") == 110.0
    assert summary.get_last("FOPT").value == 110.0
    assert summary.get_key_index("FOPT") >= 0
    assert summary.iiget(0, summary.get_key_index("FOPT")) == 0.0
    assert summary.get_report_time(1) == datetime.date(2010, 1, 3)
    assert summary.get_from_report("FOPT", 1) == 20.0
    assert summary.report_index_list() == [2, 5, 8, 11]
    assert summary.get_report_step(report_only=True) == [1, 2, 3, 4]
    assert summary.get_days(report_only=True) == [2.0, 5.0, 8.0, 11.0]
    assert summary.get_dates(report_only=True) == [
        datetime.date(2010, 1, 3),
        datetime.date(2010, 1, 6),
        datetime.date(2010, 1, 9),
        datetime.date(2010, 1, 12),
    ]

    assert summary.get_report(days=0.5 + 2) == -1
    assert summary.get_report(days=NUM_MINISTEPS - 1) == 1
    assert summary.get_report(days=2 * NUM_MINISTEPS - 1) == 2
    assert summary.get_report(days=NUM_REPORTS * NUM_MINISTEPS - 1) == NUM_REPORTS
    assert summary.get_report(date=datetime.date(2010, 1, 3)) == 1
    assert summary.get_report(date=datetime.date(2010, 1, 4)) == -1
    with pytest.raises(ValueError, match="Must supply either"):
        summary.get_report(date=datetime.date(2010, 1, 3), days=2)

    expected_days = [NUM_MINISTEPS * r - 1.0 for r in range(1, NUM_REPORTS + 1)]
    expected_dates = [
        datetime.date(2010, 1, 1) + datetime.timedelta(days=d) for d in expected_days
    ]
    assert summary.get_report_step(report_only=True) == [1, 2, 3, 4]
    assert summary.get_days(report_only=True) == expected_days
    assert summary.report_dates == expected_dates
    assert summary.get_dates(report_only=True) == expected_dates
    assert summary.report_index_list() == [
        NUM_MINISTEPS * r - 1 for r in range(1, NUM_REPORTS + 1)
    ]
    assert list(summary.numpy_vector("FOPT", report_only=True)) == pytest.approx(
        [10.0 * d for d in expected_days]
    )
    assert len(summary.alloc_time_vector(True)) == NUM_REPORTS
    assert len(summary.alloc_time_vector(False)) == NUM_REPORTS * NUM_MINISTEPS
    for report, day in enumerate(expected_days, start=1):
        assert summary.get_report_time(report) == datetime.date(
            2010, 1, 1
        ) + datetime.timedelta(days=day)
        assert summary.get_from_report("FOPT", report) == pytest.approx(10.0 * day)

    for index in (0, 5, NUM_REPORTS * NUM_MINISTEPS - 1):
        assert summary.iget("FOPT", index) == pytest.approx(10.0 * index)
        assert summary.iget_days(index) == index
        assert summary.iget_date(index) == datetime.datetime(
            2010, 1, 1
        ) + datetime.timedelta(days=index)
        assert summary.iget_report(index) == index // NUM_MINISTEPS + 1
        key_index = summary.get_key_index("FOPR")
        assert summary.iiget(index, key_index) == pytest.approx(100.0 - index)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_first_gt_and_first_lt_index_are_minus_one_when_the_limit_is_never_crossed():
    _write_case("FIRST")
    summary = Summary("FIRST")
    last = NUM_REPORTS * NUM_MINISTEPS - 1

    assert summary.first_gt_index("FOPT", 25.0) == 3
    assert summary.first_gt_index("FOPT", 10.0 * last) == -1
    assert summary.first_lt_index("FOPR", 100.0 - last + 0.5) == last
    assert summary.first_lt_index("FOPR", 0.0) == -1
    assert summary.first_gt("FOPT", 25.0).days == 3
    assert summary.first_lt("FOPR", 90.5).days == 10


@pytest.mark.usefixtures("use_tmpdir")
@ALL_FILE_LAYOUTS
def test_that_interpolation_outside_the_simulated_range_raises_value_error(
    unified, fmt_output, lazy_load
):
    _write_case("INTERP", unified=unified, fmt_output=fmt_output)
    summary = Summary("INTERP", lazy_load=lazy_load)
    last = NUM_REPORTS * NUM_MINISTEPS - 1

    assert summary.get_interp("FOPT", days=2.5) == pytest.approx(25.0)
    assert summary.get_interp("FOPT", date=datetime.datetime(2010, 1, 2, 12)) == (
        pytest.approx(15.0)
    )
    assert list(summary.get_interp_vector("FOPT", days_list=[0.5, 1.5])) == (
        pytest.approx([5.0, 15.0])
    )
    assert list(
        summary.get_interp_vector("FOPT", date_list=[datetime.datetime(2010, 1, 2, 12)])
    ) == pytest.approx([15.0])
    with pytest.raises(ValueError, match="outside range"):
        summary.get_interp("FOPT", days=last + 1)
    with pytest.raises(ValueError, match="outside range"):
        summary.get_interp("FOPT", date=datetime.datetime(2011, 1, 1))
    with pytest.raises(ValueError, match="Invalid days"):
        summary.get_interp_vector("FOPT", days_list=[-1.0])
    with pytest.raises(ValueError, match="Invalid days"):
        summary.get_interp_vector("FOPT", days_list=[last + 1.0])
    with pytest.raises(ValueError, match="Invalid date"):
        summary.get_interp_vector("FOPT", date_list=[datetime.datetime(2011, 1, 1)])
    with pytest.raises(ValueError, match="Invalid date"):
        summary.get_interp_vector("FOPT", date_list=[datetime.datetime(2009, 1, 1)])
    with pytest.raises(ValueError):
        summary.get_interp_vector("FOPT")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_unified_file_is_ignored_when_the_extension_asks_for_multiple_files():
    _write_case("MIXED", unified=True)
    _write_case("MIXED", unified=False, num_reports=2)

    assert len(Summary("MIXED.S0001")) == 2 * NUM_MINISTEPS


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_newest_of_unified_and_multiple_files_is_loaded():
    _write_case("NEWEST", unified=False, num_reports=2)
    os.utime("NEWEST.S0001", (1, 1))
    os.utime("NEWEST.S0002", (1, 1))
    _write_case("NEWEST", unified=True, num_reports=3)

    assert len(Summary("NEWEST")) == 3 * NUM_MINISTEPS


@pytest.mark.usefixtures("use_tmpdir")
def test_that_multiple_files_are_loaded_when_they_are_newer_than_the_unified_file():
    _write_case("OLDUNIF", unified=True, num_reports=3)
    os.utime("OLDUNIF.UNSMRY", (1, 1))
    _write_case("OLDUNIF", unified=False, num_reports=2)

    assert len(Summary("OLDUNIF")) == 2 * NUM_MINISTEPS


@pytest.mark.usefixtures("use_tmpdir")
def test_that_loading_a_case_without_data_files_raises_os_error():
    _write_case("NODATA")
    os.remove("NODATA.UNSMRY")

    with pytest.raises(OSError, match="Failed to create summary"):
        Summary("NODATA")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_loading_with_an_unformatted_extension_ignores_a_formatted_only_case():
    _write_case("ONLYFMT", fmt_output=True)

    with pytest.raises(OSError):
        Summary("ONLYFMT.UNSMRY")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_loading_a_case_without_header_raises_os_error():
    with pytest.raises(OSError):
        Summary("DOES_NOT_EXIST")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_case_is_guessed_when_the_directory_has_a_single_data_file():
    os.makedirs("run")
    with pushd("run"):
        _write_case("GUESSED")
        open("GUESSED.DATA", "w").close()

    assert len(Summary("run/")) == NUM_REPORTS * NUM_MINISTEPS


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_case_cannot_be_guessed_when_the_directory_has_multiple_data_files():
    os.makedirs("run")
    with pushd("run"):
        _write_case("ONE")
        open("ONE.DATA", "w").close()
        open("TWO.DATA", "w").close()

    with pytest.raises(OSError):
        Summary("run/")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_case_cannot_be_guessed_when_the_directory_has_no_data_file():
    os.makedirs("run")
    with pushd("run"):
        _write_case("ONE")

    with pytest.raises(OSError):
        Summary("run/")


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_summary_load_reads_explicitly_named_header_and_data_files(lazy_load):
    _write_case("EXPLICIT", unified=False)

    summary = Summary.load("EXPLICIT.SMSPEC", "EXPLICIT.S0001")

    assert len(summary) == NUM_MINISTEPS


@pytest.mark.usefixtures("use_tmpdir")
def test_that_summary_load_reads_a_unified_file():
    _write_case("EXPLICIT_UNIF")

    summary = Summary.load("EXPLICIT_UNIF.SMSPEC", "EXPLICIT_UNIF.UNSMRY")

    assert len(summary) == NUM_REPORTS * NUM_MINISTEPS


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_a_day_month_year_summary_can_be_interpolated_and_sliced(lazy_load):
    case = "DMY_INTERP"
    dates = [
        datetime.datetime(2010, 1, 1),
        datetime.datetime(2010, 1, 11),
        datetime.datetime(2010, 1, 21),
    ]
    _write_day_month_year_case(case, dates, [0.0, 10.0, 20.0])
    summary = Summary(case, lazy_load=lazy_load)

    assert summary.get_interp("FOPT", date=datetime.datetime(2010, 1, 6)) == (
        pytest.approx(5.0)
    )
    assert summary.get_interp("FOPT", days=15) == pytest.approx(15.0)
    row = summary.get_interp_row(
        SummaryKeyWordVector(summary, add_keywords=True), datetime.datetime(2010, 1, 16)
    )
    assert list(row) == pytest.approx([16.0, 1.0, 2010.0, 15.0])
    assert summary.get_data_start_time() == datetime.datetime(2010, 1, 1)
    assert summary.get_end_time() == datetime.datetime(2010, 1, 21)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_unsmry_without_params_for_the_first_step_is_reported_as_error():
    case = "NO_PARAMS"
    dates = [datetime.datetime(2010, 1, 1)]
    _write_day_month_year_case(case, dates, [1.0])
    Unsmry(steps=[SummaryStep(seqnum=0, ministeps=[])]).to_file(f"{case}.UNSMRY")

    with pytest.raises(OSError, match="Failed to create summary"):
        Summary(case, lazy_load=False)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_keys_and_wells_are_listed_with_optional_patterns(lazy_load):
    _write_case("KEYS")
    summary = Summary("KEYS", lazy_load=lazy_load)

    assert sorted(summary.keys()) == ["FOPR", "FOPT", "WOPR:OP1"]
    assert summary.keys("FOP*") == ["FOPR", "FOPT"]
    assert summary.keys("NOMATCH*") == []
    assert summary.wells() == ["OP1"]
    assert summary.wells("OP*") == ["OP1"]
    assert summary.wells("XX*") == []
    assert summary.groups() == []
    assert summary.has_key("WOPR:OP1")
    assert not summary.has_key("WOPR:OP2")
    assert summary.unit_system == UnitSystem.METRIC


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_blocked_production_is_the_difference_of_a_total_over_time_ranges(
    lazy_load,
):
    _write_case("BLOCKED", num_reports=2)
    summary = Summary("BLOCKED", lazy_load=lazy_load)
    points = [datetime.datetime(2010, 1, d) for d in (1, 3, 5)]

    assert list(summary.blocked_production("FOPT", points)) == pytest.approx(
        [20.0, 20.0]
    )
    with pytest.raises(TypeError):
        summary.blocked_production("FOPR", points)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_time_range_samples_the_simulated_period(lazy_load):
    _write_case("TRANGE", num_reports=4, num_ministeps=5)
    summary = Summary("TRANGE", lazy_load=lazy_load)

    by_count = summary.time_range(num_timestep=5)
    assert [t.datetime() for t in by_count] == [
        datetime.datetime(2010, 1, 1, 0),
        datetime.datetime(2010, 1, 5, 18),
        datetime.datetime(2010, 1, 10, 12),
        datetime.datetime(2010, 1, 15, 6),
        datetime.datetime(2010, 1, 20, 0),
    ]
    daily = summary.time_range(interval="5d")
    assert [t.datetime() for t in daily] == [
        datetime.datetime(2010, 1, d) for d in (1, 6, 11, 16, 21)
    ]
    monthly = summary.time_range(interval="1m")
    assert [t.datetime() for t in monthly] == [
        datetime.datetime(2010, 1, 1),
        datetime.datetime(2010, 2, 1),
    ]
    yearly = summary.time_range(interval="1y", extend_end=False)
    assert [t.datetime() for t in yearly] == [datetime.datetime(2010, 1, 1)]


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_solve_days_finds_when_a_value_is_crossed(lazy_load):
    _write_case("SOLVE", num_reports=2)
    summary = Summary("SOLVE", lazy_load=lazy_load)

    assert summary.solve_days("FOPT", 25.0) == pytest.approx([2.5])
    assert summary.solve_days("FOPR", 97.5, rates_clamp_lower=False) == (
        pytest.approx([3.0])
    )


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_resampling_gives_values_at_the_requested_times(lazy_load):
    _write_case("SOURCE", num_reports=2)
    source = Summary("SOURCE", lazy_load=lazy_load)
    points = [datetime.datetime(2010, 1, 1), datetime.datetime(2010, 1, 3)]

    resampled = source.resample("RESAMPLED", points)

    assert list(resampled["FOPT"].values) == pytest.approx([0.0, 20.0])
    assert resampled.dates == points
    with pytest.raises(ValueError):
        source.resample("EMPTY", [])
    with pytest.raises(ValueError):
        source.resample("OUTSIDE", [datetime.datetime(2020, 1, 1)])


@pytest.mark.usefixtures("use_tmpdir")
def test_that_an_oversized_index_raises_value_error():
    _write_case("INDEX")
    summary = Summary("INDEX")

    with pytest.raises(ValueError, match="looking up index"):
        summary.iget("FOPT", 10_000)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_keyword_vector_can_be_built_from_a_summary_and_copied():
    _write_case("VEC")
    summary = Summary("VEC")
    vector = SummaryKeyWordVector(summary, add_keywords=True)

    assert list(vector) == ["FOPT", "FOPR", "WOPR:OP1"]
    copied = vector.copy(summary)
    assert list(copied) == list(vector)
    with pytest.raises(KeyError):
        vector.add_keyword("NOT_A_KEY")
    assert list(summary.get_interp_row(vector, datetime.datetime(2010, 1, 2, 12))) == (
        pytest.approx([15.0, 98.0, 2.0])
    )


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_solve_dates_and_first_crossing_find_the_same_point(lazy_load):
    _write_case("CROSS", num_reports=2)
    summary = Summary("CROSS", lazy_load=lazy_load)

    dates = summary.solve_dates("FOPT", 25.0)
    assert len(dates) == 1
    assert dates[0] == datetime.datetime(2010, 1, 3, 12)
    assert summary.first_gt_index("FOPT", 25.0) == 3
    assert summary.first_lt_index("FOPR", 97.5) == 3
    assert summary.first_gt("FOPT", 25.0).value == 30.0
    assert summary.first_lt("FOPR", 97.5).value == 97.0


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_summary_can_be_written_under_a_different_case_name():
    _write_case("ORIGINAL", num_reports=2)
    summary = Summary("ORIGINAL", lazy_load=False)

    summary.fwrite("COPY")

    copy = Summary("COPY")
    assert list(copy["FOPT"].values) == list(summary["FOPT"].values)
    assert copy.dates == summary.dates


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_interpolation_and_report_lookup_at_the_boundaries_are_exact(lazy_load):
    _write_case("EDGE", num_reports=2)
    summary = Summary("EDGE", lazy_load=lazy_load)

    assert summary.get_interp("FOPT", days=0) == 0.0
    assert summary.get_interp("FOPT", days=5) == 50.0
    assert summary.get_interp("FOPT", date=datetime.datetime(2010, 1, 6)) == 50.0
    assert summary.get_report(days=5) == 2


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize(
    "call, error",
    [
        (lambda s: s.iget("NOPE", 0), IndexError),
        (lambda s: s.iget("FOPT", 10**6), ValueError),
        (lambda s: s.iiget(10**6, 0), ValueError),
        (lambda s: s.get_report_time(100), ValueError),
        (lambda s: s.get_from_report("FOPT", 100), ValueError),
        (lambda s: s.iget_days(10**6), ValueError),
        (lambda s: s.iget_date(10**6), ValueError),
        (lambda s: s.iget_report(10**6), ValueError),
        (lambda s: s.alloc_data_vector(10**6, False), IndexError),
        (lambda s: s.first_gt_index("NOPE", 1), IndexError),
        (lambda s: s.get_interp("NOPE", days=1), KeyError),
        (lambda s: s.unit("NOPE"), KeyError),
        (lambda s: s.numpy_vector("NOPE"), KeyError),
        (lambda s: s.solve_days("NOPE", 1), KeyError),
        (lambda s: s.last_value("NOPE"), KeyError),
        (lambda s: s.smspec_node("NOPE"), KeyError),
        (lambda s: SummaryKeyWordVector(s).add_keyword("NOPE"), KeyError),
        (
            lambda s: s.get_interp("FOPT", date=datetime.datetime(2000, 1, 1)),
            ValueError,
        ),
        (
            lambda s: s.get_interp_row(["FOPT"], datetime.datetime(2010, 1, 2)),
            TypeError,
        ),
    ],
)
def test_that_invalid_keys_indices_and_times_raise_the_appropriate_exception(
    lazy_load, call, error
):
    _write_case("ERRORS")
    summary = Summary("ERRORS", lazy_load=lazy_load)

    with pytest.raises(error):
        call(summary)


def _corrupt_unsmry(case, corruption):
    _write_day_month_year_case(
        case, [datetime.datetime(2010, 1, 1), datetime.datetime(2010, 2, 1)], [1.0, 2.0]
    )
    path = f"{case}.UNSMRY"
    if corruption == "truncated":
        with open(path, "rb") as f:
            data = f.read()
        with open(path, "wb") as f:
            f.write(data[:-30])
    elif corruption == "garbage":
        with open(path, "wb") as f:
            f.write(b"garbage" * 50)
    elif corruption == "no_steps":
        Unsmry(steps=[]).to_file(path)
    elif corruption == "params_too_short":
        Unsmry(
            steps=[
                SummaryStep(
                    seqnum=0,
                    ministeps=[SummaryMiniStep(mini_step=0, params=[1.0, 1.0, 2010.0])],
                )
            ]
        ).to_file(path)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize("corruption", ["truncated", "garbage", "no_steps"])
def test_that_an_unreadable_data_file_makes_loading_fail_with_os_error(
    lazy_load, corruption
):
    _corrupt_unsmry("CORRUPT", corruption)

    with pytest.raises(OSError, match="Failed to create summary"):
        Summary("CORRUPT", lazy_load=lazy_load)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_step_with_fewer_params_than_the_header_declares_is_discarded():
    _corrupt_unsmry("SHORT", "params_too_short")

    with pytest.raises(OSError, match="Failed to create summary"):
        Summary("SHORT", lazy_load=False)


def _smspec_for(keywords, units, wells, nums, **kwargs):
    return Smspec(
        nx=2,
        ny=2,
        nz=2,
        restarted_from_step=0,
        num_keywords=len(keywords),
        restart=kwargs.pop("restart", "        "),
        keywords=keywords,
        well_names=wells,
        region_numbers=nums,
        units=units,
        start_date=Date(day=1, month=1, year=2010, hour=0, minutes=0, micro_seconds=0),
        intehead=SmspecIntehead(
            unit=ResfoUnitSystem.METRIC, simulator=Simulator.ECLIPSE_100
        ),
        **kwargs,
    )


def _write_header_case(case, smspec, drop=(), time_values=(0.0, 1.0)):
    """Writes `smspec` (without the keywords named in `drop`) and an UNSMRY
    with one ministep per entry in `time_values`; TIME is the first keyword
    and all other parameters are zero."""
    items = [kw for kw in smspec.to_smry() if kw[0].strip() not in drop]
    resfo.write(f"{case}.SMSPEC", items, resfo.Format.UNFORMATTED)
    Unsmry(
        steps=[
            SummaryStep(
                seqnum=i,
                ministeps=[
                    SummaryMiniStep(
                        mini_step=i,
                        params=[t] + [0.0] * (smspec.num_keywords - 1),
                    )
                ],
            )
            for i, t in enumerate(time_values)
        ]
    ).to_file(f"{case}.UNSMRY")


def _time_and_fopt(time_unit="DAYS", **kwargs):
    return _smspec_for(
        ["TIME    ", "FOPT    "],
        [time_unit.ljust(8), "SM3     "],
        [":+:+:+:+", "        "],
        [0, 0],
        **kwargs,
    )


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize(
    "missing", ["WGNAMES", "KEYWORDS", "STARTDAT", "UNITS", "DIMENS"]
)
def test_that_a_header_lacking_a_required_keyword_is_rejected(lazy_load, missing):
    _write_header_case("HEADER", _time_and_fopt(), drop=(missing,))

    with pytest.raises(OSError, match="Failed to create summary"):
        Summary("HEADER", lazy_load=lazy_load)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize("optional", ["INTEHEAD", "RESTART", "NUMS"])
def test_that_a_header_lacking_an_optional_keyword_is_still_loaded(lazy_load, optional):
    _write_header_case("HEADER", _time_and_fopt(), drop=(optional,))

    assert Summary("HEADER", lazy_load=lazy_load).keys() == ["FOPT"]


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_the_names_keyword_is_accepted_in_place_of_wgnames(lazy_load):
    _write_header_case("NAMES", _time_and_fopt(use_names=True))

    assert Summary("NAMES", lazy_load=lazy_load).keys() == ["FOPT"]


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize(
    "time_unit, seconds_per_unit", [("DAYS", 86400.0), ("HOURS", 3600.0)]
)
def test_that_the_time_unit_of_the_header_scales_the_time_values(
    lazy_load, time_unit, seconds_per_unit
):
    _write_header_case("TIMEUNIT", _time_and_fopt(time_unit), time_values=(0.0, 2.0))

    summary = Summary("TIMEUNIT", lazy_load=lazy_load)

    expected_end = datetime.datetime(2010, 1, 1) + datetime.timedelta(
        seconds=2.0 * seconds_per_unit
    )
    assert summary.dates[-1] == expected_end


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize("time_unit", ["YEARS", ""])
def test_that_an_unrecognized_time_unit_is_rejected(lazy_load, time_unit):
    _write_header_case("BADUNIT", _time_and_fopt(time_unit))

    with pytest.raises(ValueError, match="not recognized"):
        Summary("BADUNIT", lazy_load=lazy_load)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_lgr_variables_are_named_by_lgr_and_cell_index(lazy_load):
    smspec = _smspec_for(
        ["TIME    ", "LBPR    "],
        ["DAYS    ", "BARSA   "],
        [":+:+:+:+", "LGR1    "],
        [0, 1],
        lgrs=[":+:+:+:+", "LGR1    "],
        numlx=[0, 1],
        numly=[0, 2],
        numlz=[0, 3],
    )
    _write_header_case("LGR", smspec)

    assert Summary("LGR", lazy_load=lazy_load).keys() == ["LBPR:LGR1:1,2,3"]


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_lgrs_without_all_index_keywords_is_rejected(lazy_load):
    smspec = _time_and_fopt(lgrs=[":+:+:+:+", "LGR1    "])
    _write_header_case("LGR", smspec)

    with pytest.raises(ValueError, match="NUMLX, NUMLY, NUMLZ"):
        Summary("LGR", lazy_load=lazy_load)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
def test_that_an_lgr_variable_without_lgr_metadata_is_rejected(lazy_load):
    smspec = _smspec_for(
        ["TIME    ", "LBPR    "],
        ["DAYS    ", "BARSA   "],
        [":+:+:+:+", "LGR1    "],
        [0, 1],
    )
    _write_header_case("LGR", smspec)

    with pytest.raises(ValueError, match="LGR variable 'LBPR'"):
        Summary("LGR", lazy_load=lazy_load)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_case_restarting_from_itself_ignores_the_restart():
    _write_header_case("SELF", _time_and_fopt(restart="SELF"))

    assert Summary("SELF").restart_case is None


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_restart_case_is_found_when_a_file_with_the_case_name_exists():
    _write_case("HIST")
    with open("HIST", "w") as f:
        f.write("not a summary")
    _write_header_case("RESTARTED", _time_and_fopt(restart="HIST"))

    restarted = Summary("RESTARTED")

    assert isinstance(restarted.restart_case, Summary)
    assert restarted.restart_case.case == os.path.join(os.getcwd(), "HIST")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_restart_case_without_header_file_is_ignored():
    _write_header_case("RESTARTED", _time_and_fopt(restart="MISSING"))

    assert Summary("RESTARTED").restart_case is None


@pytest.mark.usefixtures("use_tmpdir")
def test_that_a_writer_with_time_in_hours_round_trips_the_dates():
    writer = Summary.writer(
        "HOURS", datetime.datetime(2010, 1, 1), 5, 5, 5, time_in_days=False
    )
    writer.add_variable("FOPT", unit="SM3")
    for days in range(3):
        writer.add_t_step(1, days)["FOPT"] = float(days)
    writer.fwrite()

    summary = Summary("HOURS")

    assert summary.dates == [datetime.datetime(2010, 1, d) for d in (1, 2, 3)]


def _keys_of_single_variable(case, keyword, well, num, unit, lazy_load=False):
    smspec = _smspec_for(
        ["TIME    ", keyword.ljust(8)],
        ["DAYS    ", unit.ljust(8)],
        [":+:+:+:+", well.ljust(8)],
        [0, num],
    )
    _write_header_case(case, smspec)
    return Summary(case, lazy_load=lazy_load).keys()


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize(
    "keyword, well, num, unit, expected",
    [
        ("NPR", "NODE1", 0, "BARSA", ["NPR:NODE1"]),
        ("FOPR", "", 0, "SM3/DAY", ["FOPR"]),
        ("FOO", "", 0, "SM3", ["FOO"]),
        ("LLINEAR", "", 0, "1", ["LLINEAR"]),
        ("GOPR", "G1", 0, "SM3/DAY", ["GOPR:G1"]),
        ("WBHP", "W1", 0, "BARSA", ["WBHP:W1"]),
        ("RPR", "", 3, "BARSA", ["RPR:3"]),
        ("RORFR", "", 3, "SM3", ["RORFR:3"]),
        ("AAQR", "", 1, "SM3", ["AAQR:1"]),
        ("SOFT", "W1", 2, "SM3", ["SOFT:W1:2"]),
        ("BPR", "", 3, "BARSA", ["BPR:1,2,1", "BPR:3"]),
        ("CWIR", "W1", 3, "SM3", ["CWIR:W1:1,2,1", "CWIR:W1:3"]),
        ("RGFR", "", 5, "SM3/DAY", ["RGFR:5", "RGFR:5--10"]),
        ("RGFT", "", 5, "SM3", ["RGFT:5", "RGFT:5--10"]),
        ("RNLF", "", 5, "SM3", ["RNLF:5", "RNLF:5--10"]),
        ("ROFT", "", 3 + 32768 * 11, "SM3", ["ROFT:3-1", "ROFT:360451"]),
    ],
)
def test_that_variables_are_keyed_by_their_type_specific_identifiers(
    lazy_load, keyword, well, num, unit, expected
):
    keys = _keys_of_single_variable("TYPES", keyword, well, num, unit, lazy_load)

    assert sorted(keys) == sorted(expected)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize(
    "keyword, well, num",
    [
        ("WOPR", "", 0),
        ("WOPR", ":+:+:+:+", 0),
        ("CWIR", "W1", -1),
        ("BPR", "", -1),
    ],
)
def test_that_variables_lacking_a_required_name_or_number_are_skipped(
    keyword, well, num
):
    assert _keys_of_single_variable("SKIP", keyword, well, num, "SM3") == []


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("keyword, well", [("BPR", ""), ("CWIR", "W1")])
@pytest.mark.parametrize("num", [0, 100])
def test_that_a_cell_number_outside_the_grid_raises_value_error(keyword, well, num):
    with pytest.raises(ValueError, match="Invalid NUMS value"):
        _keys_of_single_variable("BADNUM", keyword, well, num, "SM3")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_region_to_region_variables_are_total_or_rate_by_name():
    smspec = _smspec_for(
        ["TIME    ", "RGFR    ", "RGFT    ", "ROFT    "],
        ["DAYS    ", "SM3/DAY ", "SM3     ", "SM3     "],
        [":+:+:+:+"] * 4,
        [0, 5, 5, 3 + 32768 * 11],
    )
    _write_header_case("R2R", smspec)

    summary = Summary("R2R")

    assert summary.smspec_node("RGFT:5").is_total()
    assert not summary.smspec_node("RGFR:5").is_total()
    assert summary.smspec_node("ROFT:3-1").is_total()
    assert summary.smspec_node("RGFR:5").is_rate()


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize(
    "points",
    [
        [datetime.datetime(2010, 1, 3), datetime.datetime(2010, 1, 1)],
        [datetime.datetime(2009, 1, 1), datetime.datetime(2010, 1, 3)],
        [datetime.datetime(2010, 1, 1), datetime.datetime(2011, 1, 1)],
    ],
    ids=["unsorted", "before_start", "after_end"],
)
def test_that_resampling_without_extrapolation_rejects_invalid_time_points(points):
    _write_case("SOURCE", num_reports=2)

    with pytest.raises(ValueError, match="Failed to create new resampled case"):
        Summary("SOURCE").resample("RESAMPLED", points)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_resampling_with_extrapolation_clamps_to_the_first_and_last_value():
    _write_case("SOURCE", num_reports=2)
    source = Summary("SOURCE")
    before = datetime.datetime(2009, 1, 1)
    after = datetime.datetime(2011, 1, 1)
    inside = [datetime.datetime(2010, 1, 1), datetime.datetime(2010, 1, 3)]

    low = source.resample(
        "LOW", [before, *inside], lower_extrapolation=True, upper_extrapolation=False
    )
    high = source.resample("HIGH", [*inside, after], upper_extrapolation=True)

    assert list(low["FOPT"].values) == pytest.approx([0.0, 0.0, 20.0])
    assert list(high["FOPT"].values) == pytest.approx([0.0, 20.0, 50.0])


@pytest.mark.usefixtures("use_tmpdir")
def test_that_export_csv_creates_missing_parent_directories():
    _write_case("CSV_SRC", num_reports=2)

    Summary("CSV_SRC").export_csv(os.path.join("new", "sub", "out.csv"))

    assert os.path.isfile(os.path.join("new", "sub", "out.csv"))


@pytest.mark.usefixtures("use_tmpdir")
def test_that_export_csv_to_a_directory_raises_runtime_error():
    _write_case("CSV_SRC", num_reports=2)
    os.mkdir("a_directory")

    with pytest.raises(RuntimeError, match="Failed to open 'a_directory'"):
        Summary("CSV_SRC").export_csv("a_directory")


@pytest.mark.usefixtures("use_tmpdir")
def test_that_loading_from_a_nonexistent_directory_raises_os_error():
    with pytest.raises(OSError, match="Failed to create summary"):
        Summary(os.path.join("no_such_dir", "CASE"))


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("add", ["variable", "local_variable"])
def test_that_variables_cannot_be_added_after_the_first_time_step(add):
    writer = Summary.writer("WRITER", datetime.datetime(2010, 1, 1), 5, 5, 5)
    writer.add_variable("FOPT", unit="SM3")
    writer.add_t_step(1, 1)

    with pytest.raises(ValueError, match="Can not interchange"):
        if add == "variable":
            writer.add_variable("FOPR", unit="SM3/DAY")
        else:
            writer.add_variable(
                "LBPR", wgname="L1", lgr="L1", lgr_ijk=(1, 1, 1), unit="BARSA"
            )


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_case_is_guessed_from_a_lowercase_data_file():
    _write_case("CASE", num_reports=2)
    os.mkdir("run")
    for ext in ("SMSPEC", "UNSMRY"):
        shutil.move(f"CASE.{ext}", f"run/CASE.{ext}")
    with open("run/CASE.data", "w"):
        pass

    assert sorted(Summary("run/").keys()) == ["FOPR", "FOPT", "WOPR:OP1"]


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("formatted_is_newer", [True, False])
def test_that_a_case_with_both_formatted_and_unformatted_headers_can_be_loaded(
    formatted_is_newer,
):
    _write_case("BOTH", fmt_output=False, unified=True, num_reports=3)
    _write_case("BOTH", fmt_output=True, unified=True, num_reports=4)
    older, newer = ("BOTH.SMSPEC", "BOTH.FSMSPEC")
    if not formatted_is_newer:
        older, newer = newer, older
    os.utime(older, (1_000_000_000, 1_000_000_000))
    os.utime(newer, (1_100_000_000, 1_100_000_000))

    summary = Summary("BOTH")

    num_reports = 3 if newer == "BOTH.FSMSPEC" else 4

    assert list(summary["FOPT"].values) == [
        10.0 * d for d in range(num_reports * NUM_MINISTEPS)
    ]


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("lazy_load", [True, False])
@pytest.mark.parametrize("dtype", ["int32", "float64"])
def test_that_a_params_keyword_which_is_not_single_precision_float_is_rejected(
    lazy_load, dtype
):
    _write_case("WRONGTYPE", num_reports=2)
    contents = [
        (kw, np.asarray(array).astype(dtype) if kw.strip() == "PARAMS" else array)
        for kw, array in resfo.read("WRONGTYPE.UNSMRY")
    ]
    resfo.write("WRONGTYPE.UNSMRY", contents)

    with pytest.raises(ValueError, match="PARAMS"):
        Summary("WRONGTYPE", lazy_load=lazy_load)
