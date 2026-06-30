import datetime

import pytest
from resdata.summary import Summary, SummaryTStep
from resdata.util.util import CTime


def make_tstep():
    writer = Summary.writer(
        "TSTEP_FAULT", datetime.datetime(2000, 1, 1, 12, 30, 15), 1, 1, 1
    )
    fopt = writer.add_variable("FOPT", unit="SM3")
    fopr = writer.add_variable("FOPR", unit="SM3/DAY")
    tstep = writer.add_t_step(7, 2.5)
    tstep[fopt.get_key1()] = 123.5
    tstep[fopr.get_key1()] = 7.25
    return tstep


def test_camel_case_methods_are_public_class_attributes():
    assert callable(SummaryTStep.getSimDays)
    assert callable(SummaryTStep.getReport)
    assert callable(SummaryTStep.getMiniStep)
    assert callable(SummaryTStep.getSimTime)


def test_camel_case_methods_are_listed_on_instances():
    names = dir(make_tstep())
    assert "getSimDays" in names
    assert "getReport" in names
    assert "getMiniStep" in names
    assert "getSimTime" in names


def test_get_sim_days_camel_alias_returns_float_value():
    tstep = make_tstep()
    with pytest.deprecated_call(match="getSimDays.*get_sim_days"):
        sim_days = tstep.getSimDays()
    assert isinstance(sim_days, float)
    assert sim_days == pytest.approx(2.5)


def test_get_report_camel_alias_returns_int_value():
    tstep = make_tstep()
    with pytest.deprecated_call(match="getReport.*get_report"):
        report = tstep.getReport()
    assert isinstance(report, int)
    assert report == 7


def test_get_mini_step_camel_alias_returns_int_value():
    tstep = make_tstep()
    with pytest.deprecated_call(match="getMiniStep.*get_mini_step"):
        mini_step = tstep.getMiniStep()
    assert isinstance(mini_step, int)
    assert mini_step == 0


def test_get_sim_time_camel_alias_returns_ctime_value():
    tstep = make_tstep()
    with pytest.deprecated_call(match="getSimTime.*get_sim_time"):
        sim_time = tstep.getSimTime()
    assert isinstance(sim_time, CTime)
    assert sim_time.datetime() == datetime.datetime(2000, 1, 4, 0, 30, 15)


def test_camel_aliases_are_deprecated_wrappers_not_snake_methods():
    assert SummaryTStep.getSimDays is not SummaryTStep.get_sim_days
    assert SummaryTStep.getReport is not SummaryTStep.get_report
    assert SummaryTStep.getMiniStep is not SummaryTStep.get_mini_step
    assert SummaryTStep.getSimTime is not SummaryTStep.get_sim_time


def test_repeated_camel_get_sim_days_calls_warn_each_time():
    tstep = make_tstep()
    with pytest.deprecated_call(match="getSimDays.*get_sim_days"):
        first = tstep.getSimDays()
    with pytest.deprecated_call(match="getSimDays.*get_sim_days"):
        second = tstep.getSimDays()
    assert first == second == pytest.approx(2.5)


def test_camel_aliases_preserve_tstep_values_after_setitem_roundtrip():
    tstep = make_tstep()
    tstep["FOPT"] = 456.75
    assert tstep["FOPT"] == pytest.approx(456.75)
    with pytest.deprecated_call(match="getReport.*get_report"):
        assert tstep.getReport() == 7
    with pytest.deprecated_call(match="getSimDays.*get_sim_days"):
        assert tstep.getSimDays() == pytest.approx(2.5)


def test_camel_sim_time_matches_snake_case_sim_time():
    tstep = make_tstep()
    with pytest.deprecated_call(match="getSimTime.*get_sim_time"):
        camel_time = tstep.getSimTime()
    assert camel_time == tstep.get_sim_time()
