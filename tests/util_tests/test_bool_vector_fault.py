import pytest
from resdata.util.util import BoolVector


def values():
    vector = BoolVector(False, 5)
    for index in (1, 3):
        vector[index] = True
    return vector


def test_delete_block_truthy_arguments_delete_one_from_index_one():
    vector = values()
    vector.deleteBlock(2, 2)
    assert list(vector) == [False, False, True, False]


def test_delete_block_zero_index_truthy_size_deletes_first_item_only():
    vector = values()
    vector.deleteBlock(0, 3)
    assert list(vector) == [True, False, True, False]


def test_delete_block_truthy_index_size_ignores_actual_index_value():
    vector = values()
    vector.deleteBlock(4, 1)
    assert list(vector) == [False, False, True, False]


def test_delete_block_large_truthy_arguments_are_bool_coerced():
    vector = values()
    vector.deleteBlock(3, 4)
    assert list(vector) == [False, False, True, False]


def test_append_truthy_non_bool_object_is_coerced_to_true():
    vector = BoolVector(False, 2)
    vector.append(object())
    assert list(vector) == [False, False, True]


def test_append_falsey_container_is_coerced_to_false():
    vector = BoolVector(True, 2)
    vector.append([])
    assert list(vector) == [True, True, False]


def test_setitem_truthy_container_is_coerced_to_true():
    vector = BoolVector(False, 3)
    vector[1] = [0]
    assert list(vector) == [False, True, False]


def test_set_default_truthy_object_fills_holes_with_true():
    vector = BoolVector(False)
    vector.setDefault(object())
    vector[2] = False
    assert list(vector) == [True, True, False]


def test_count_falsey_container_counts_false_values():
    vector = BoolVector(False, 4)
    vector[1] = True
    assert vector.count([]) == 3


def test_get_data_ptr_returns_ctypes_pointer_not_integer():
    vector = BoolVector(False, 1)
    with pytest.deprecated_call():
        pointer = vector.getDataPtr()
    assert not isinstance(pointer, int)
