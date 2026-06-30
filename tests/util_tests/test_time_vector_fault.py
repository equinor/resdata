from datetime import date, datetime

import pytest
from resdata.util.util import TimeVector


def test_setitem_rejects_plain_integer():
    vector = TimeVector(initial_size=1)

    with pytest.raises(TypeError):
        vector[0] = 1


def test_setitem_rejects_date_value():
    vector = TimeVector(initial_size=1)

    with pytest.raises(TypeError):
        vector[0] = date(1970, 1, 2)


def test_setitem_rejects_datetime_value():
    vector = TimeVector(initial_size=1)

    with pytest.raises(TypeError):
        vector[0] = datetime(1970, 1, 1, 0, 0, 1)


def test_negative_setitem_rejects_plain_integer():
    vector = TimeVector()
    vector.append(0)

    with pytest.raises(TypeError):
        vector[-1] = 1


def test_slice_assignment_rejects_plain_integer():
    vector = TimeVector(initial_size=3)

    with pytest.raises(TypeError):
        vector[0:2] = 1


def test_set_default_rejects_plain_integer():
    vector = TimeVector()

    with pytest.raises(TypeError):
        vector.setDefault(1)


def test_count_equal_rejects_plain_integer():
    vector = TimeVector()
    vector.append(0)

    with pytest.raises(TypeError):
        vector.countEqual(0)


def test_init_range_rejects_plain_integers():
    vector = TimeVector()

    with pytest.raises(TypeError):
        vector.initRange(0, 2, 1)


def test_create_linear_rejects_plain_integers():
    with pytest.raises(TypeError):
        TimeVector.create_linear(0, 2, 2)


def test_inplace_add_rejects_plain_integer():
    vector = TimeVector()
    vector.append(0)

    with pytest.raises(TypeError):
        vector += 1
