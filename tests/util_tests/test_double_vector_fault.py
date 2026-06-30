from resdata.util.util import DoubleVector

UINT32 = 2**32


def test_constructor_initial_size_wraps_to_empty_vector():
    vector = DoubleVector(default_value=7.25, initial_size=UINT32)

    assert len(vector) == 0
    assert list(vector) == []


def test_constructor_initial_size_wraps_to_one_default_element():
    vector = DoubleVector(default_value=7.25, initial_size=UINT32 + 1)

    assert len(vector) == 1
    assert list(vector) == [7.25]


def test_large_assignment_index_wraps_before_setting_value():
    vector = DoubleVector(default_value=1.5)

    vector[UINT32 + 2] = 5.5

    assert len(vector) == 3
    assert list(vector) == [1.5, 1.5, 5.5]


def test_safe_get_large_index_wraps_to_existing_element():
    vector = DoubleVector.create_linear(10.0, 12.0, 3)

    assert vector.safeGetByIndex(UINT32 + 1) == 11.0


def test_delete_block_large_start_index_wraps_to_second_element():
    vector = DoubleVector.create_linear(10.0, 13.0, 4)

    vector.deleteBlock(UINT32 + 1, 1)

    assert list(vector) == [10.0, 12.0, 13.0]


def test_delete_block_large_block_size_wraps_to_single_deletion():
    vector = DoubleVector.create_linear(10.0, 13.0, 4)

    vector.deleteBlock(1, UINT32 + 1)

    assert list(vector) == [10.0, 12.0, 13.0]


def test_right_shift_large_count_wraps_to_one_place():
    vector = DoubleVector.create_linear(1.0, 3.0, 3)

    shifted = vector >> (UINT32 + 1)

    assert list(vector) == [1.0, 2.0, 3.0]
    assert list(shifted) == [0.0, 1.0, 2.0, 3.0]


def test_inplace_right_shift_large_count_wraps_to_one_place():
    vector = DoubleVector.create_linear(1.0, 3.0, 3)

    vector >>= UINT32 + 1

    assert list(vector) == [0.0, 1.0, 2.0, 3.0]


def test_create_linear_large_num_values_wraps_to_three_values():
    vector = DoubleVector.create_linear(0.0, 1.0, UINT32 + 3)

    assert list(vector) == [0.0, 0.5, 1.0]


def test_first_eq_large_offset_wraps_before_searching():
    left = DoubleVector.create_linear(1.0, 3.0, 3)
    right = DoubleVector.create_linear(0.0, 3.0, 4)

    assert left.first_eq(right, UINT32 + 1) == -1
