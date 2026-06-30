from resdata.util.util import IntVector


def test_constructor_wraps_large_default_value():
    vector = IntVector(default_value=2**31, initial_size=2)

    assert len(vector) == 2
    assert vector.getDefault() == -(2**31)
    assert list(vector) == [-(2**31), -(2**31)]


def test_constructor_wraps_too_negative_default_value():
    vector = IntVector(default_value=-(2**31) - 1, initial_size=2)

    assert len(vector) == 2
    assert vector.getDefault() == 2**31 - 1
    assert list(vector) == [2**31 - 1, 2**31 - 1]


def test_append_wraps_value_at_uint32_boundary():
    vector = IntVector()

    vector.append(2**32)

    assert list(vector) == [0]


def test_append_wraps_value_below_int32_minimum():
    vector = IntVector()

    vector.append(-(2**31) - 1)

    assert list(vector) == [2**31 - 1]


def test_setitem_wraps_large_value_while_preserving_default_holes():
    vector = IntVector(default_value=7)

    vector[2] = 2**31

    assert list(vector) == [7, 7, -(2**31)]


def test_set_default_wraps_value_used_for_later_holes():
    vector = IntVector()

    vector.setDefault(2**31 + 3)
    vector[2] = 9

    assert vector.getDefault() == -(2**31) + 3
    assert list(vector) == [-(2**31) + 3, -(2**31) + 3, 9]


def test_assign_wraps_scalar_for_all_existing_elements():
    vector = IntVector(initial_size=3)

    vector.assign(2**32 - 2)

    assert list(vector) == [-2, -2, -2]


def test_addition_wraps_large_scalar_shift():
    vector = IntVector()
    vector.append(1)

    shifted = vector + 2**31

    assert list(shifted) == [-(2**31) + 1]
    assert list(vector) == [1]


def test_contains_and_count_wrap_large_lookup_value():
    vector = IntVector()
    vector.append(-1)

    assert 2**32 - 1 in vector
    assert vector.count(2**32 - 1) == 1
    assert vector.countEqual(2**32 - 1) == 1


def test_create_linear_wraps_large_endpoints():
    vector = IntVector.create_linear(2**31, 2**31 + 2, 3)

    assert list(vector) == [-(2**31), -(2**31) + 1, -(2**31) + 2]
