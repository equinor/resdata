import pytest
from resdata.util.util import BoolVector, DoubleVector, IntVector


def make_vector(vector_class, values):
    vector = vector_class()
    for value in values:
        vector.append(value)
    return vector


def test_int_permutation_len_items_negative_index_and_string():
    vector = make_vector(IntVector, [3, 1, 2])

    permutation = vector.permutationSort()

    assert len(permutation) == 3
    assert [permutation[0], permutation[1], permutation[2]] == [1, 2, 0]
    assert permutation[-1] == 0
    assert str(permutation) == "( 1 2 0)"


def test_reverse_int_permutation_lists_largest_items_first():
    vector = make_vector(IntVector, [3, 1, 2])

    permutation = vector.permutationSort(reverse=True)

    assert list(permutation) == [0, 2, 1]
    assert str(permutation) == "( 0 2 1)"


def test_ascending_int_permutation_keeps_equal_values_stable():
    vector = make_vector(IntVector, [2, 1, 2, 1])

    permutation = vector.permutationSort()

    assert list(permutation) == [1, 3, 0, 2]
    assert str(permutation) == "( 1 3 0 2)"


def test_descending_double_permutation_keeps_equal_values_stable():
    vector = make_vector(DoubleVector, [3.5, -1.0, 3.5, 0.0])

    permutation = vector.permutationSort(reverse=True)

    assert list(permutation) == [0, 2, 3, 1]
    assert str(permutation) == "( 0 2 3 1)"


def test_empty_vector_has_no_permutation_but_single_item_vector_does():
    empty_vector = IntVector()
    single_item_vector = make_vector(IntVector, [42])

    assert empty_vector.permutationSort() is None
    permutation = single_item_vector.permutationSort()
    assert len(permutation) == 1
    assert permutation[0] == 0
    assert str(permutation) == "( 0)"


def test_permutation_index_errors_match_sequence_bounds():
    vector = make_vector(IntVector, [3, 1, 2])
    permutation = vector.permutationSort()

    with pytest.raises(IndexError, match="Invalid index:3"):
        permutation[3]
    with pytest.raises(IndexError, match="Invalid index:-1"):
        permutation[-4]


def test_permutation_reorders_int_vector_without_sorting_source():
    source = make_vector(IntVector, [30, 10, 20])
    target = make_vector(IntVector, [300, 100, 200])

    permutation = source.permutationSort()
    target.permute(permutation)

    assert list(source) == [30, 10, 20]
    assert list(target) == [100, 200, 300]


def test_reverse_permutation_reorders_double_vector_descending():
    source = make_vector(DoubleVector, [1.5, 3.5, 2.5])
    target = make_vector(DoubleVector, [10.5, 30.5, 20.5])

    target.permute(source.permutationSort(reverse=True))

    assert list(target) == [30.5, 20.5, 10.5]


def test_longer_permutation_can_be_applied_to_shorter_vector():
    source = make_vector(IntVector, [2, 1, 3])
    target = make_vector(IntVector, [10, 20])

    target.permute(source.permutationSort())

    assert len(target) == 2
    assert list(target) == [20, 10]


def test_same_permutation_can_be_applied_repeatedly():
    source = make_vector(BoolVector, [True, False, True])
    target = make_vector(IntVector, [300, 100, 200])
    permutation = source.permutationSort()

    target.permute(permutation)
    assert list(target) == [100, 300, 200]

    target.permute(permutation)
    assert list(target) == [300, 100, 200]
