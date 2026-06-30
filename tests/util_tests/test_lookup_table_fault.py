import math

import pytest


def make_lookup_table(points=(), **kwargs):
    from resdata.util.util import LookupTable

    table = LookupTable(**kwargs)
    for x, y in points:
        table.append(x, y)
    return table


def test_size_and_len_are_python_int_after_multiple_appends():
    table = make_lookup_table([(2.0, 20.0), (0.0, 0.0), (1.0, 10.0)])

    assert len(table) == 3
    assert table.size == 3
    assert type(len(table)) is int
    assert type(table.size) is int


def test_interp_sorts_non_monotonic_appends_before_interpolating():
    table = make_lookup_table([(2.0, 20.0), (0.0, 0.0), (1.0, 10.0)])

    assert table.interp(0.0) == 0.0
    assert table.interp(1.0) == 10.0
    assert table.interp(1.5) == 15.0
    assert table.interp(2.0) == 20.0


def test_duplicate_x_exact_match_uses_last_duplicate_value():
    table = make_lookup_table([(0.0, 0.0), (1.0, 10.0), (1.0, 20.0), (2.0, 40.0)])

    assert table.interp(1.0) == 20.0


def test_duplicate_x_interpolates_from_last_duplicate_to_next_point():
    table = make_lookup_table([(0.0, 0.0), (1.0, 10.0), (1.0, 20.0), (2.0, 40.0)])

    assert table.interp(1.5) == 30.0


def test_interp_below_range_without_lower_limit_raises_value_error():
    table = make_lookup_table([(0.0, 0.0), (1.0, 10.0)])

    with pytest.raises(ValueError, match="outside valid interval"):
        table.interp(-0.25)


def test_interp_above_range_without_upper_limit_raises_value_error():
    table = make_lookup_table([(0.0, 0.0), (1.0, 10.0)])

    with pytest.raises(ValueError, match="outside valid interval"):
        table.interp(1.25)


def test_lower_limit_clamps_below_range_but_not_above_range():
    table = make_lookup_table([(0.0, 0.0), (1.0, 10.0)], lower_limit=-7.5)

    assert table.hasLowerLimit() is True
    assert table.hasUpperLimit() is False
    assert table.interp(-0.25) == -7.5
    with pytest.raises(ValueError):
        table.interp(1.25)


def test_upper_limit_clamps_above_range_but_not_below_range():
    table = make_lookup_table([(0.0, 0.0), (1.0, 10.0)], upper_limit=88.0)

    assert table.hasLowerLimit() is False
    assert table.hasUpperLimit() is True
    assert table.interp(1.25) == 88.0
    with pytest.raises(ValueError):
        table.interp(-0.25)


def test_min_max_values_and_args_are_sorted_independently():
    table = make_lookup_table([(10.0, -5.0), (-2.0, 3.0), (4.0, 100.0)])

    assert table.min == -5.0
    assert table.max == 100.0
    assert table.arg_min == -2.0
    assert table.arg_max == 10.0


def test_interp_return_type_and_extreme_values_match_ctypes_behavior():
    huge = make_lookup_table([(-1e300, -1e300), (1e300, 1e300)])
    tiny = make_lookup_table([(1e-300, 2e-300), (3e-300, 4e-300)])

    assert type(huge.interp(-1e300)) is float
    assert math.isnan(huge.interp(0.0))
    assert tiny.interp(2e-300) == 0.0
