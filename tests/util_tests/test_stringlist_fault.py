import pytest
from resdata.util.util import StringList


def assert_old_string_argument_error(action):
    with pytest.raises(TypeError, match="CStringHelper"):
        action()


def assert_old_integer_argument_error(action):
    with pytest.raises(TypeError, match=r"ctypes\.c_int"):
        action()


def test_constructor_from_list_len_index_and_non_string_contains():
    strings = StringList(["alpha", "beta", "gamma"])

    assert len(strings) == 3
    assert strings[0] == "alpha"
    assert strings[1] == "beta"
    assert strings[2] == "gamma"
    assert "beta" in strings
    assert_old_string_argument_error(lambda: 12 in strings)


def test_append_iadd_and_float_contains_conversion_error():
    strings = StringList(["start"])
    strings.append(123)
    strings += ["tail", None]

    assert strings.strings == ["start", "123", "tail", "None"]
    assert_old_string_argument_error(lambda: 1.5 in strings)


def test_negative_and_out_of_range_getitem_then_list_contains_error():
    strings = StringList(["first", "middle", "last"])

    assert strings[-1] == "last"
    assert strings[-3] == "first"
    with pytest.raises(IndexError):
        strings[-4]
    with pytest.raises(IndexError):
        strings[3]
    assert_old_string_argument_error(lambda: [] in strings)


def test_setitem_empty_string_bytes_and_bytearray_contains_error():
    strings = StringList(["left", "middle", "right"])

    strings[-1] = b"bytes"
    strings[1] = ""

    assert strings.strings == ["left", "", "bytes"]
    assert "" in strings
    assert_old_string_argument_error(lambda: bytearray(b"bytes") in strings)


def test_slicing_rejected_and_float_sort_flag_conversion_error():
    strings = StringList(["a", "b", "c"])

    with pytest.raises(TypeError, match="Index should be integer type"):
        strings[1:]
    assert_old_integer_argument_error(lambda: strings.sort(0.0))


def test_pop_front_last_and_string_sort_flag_conversion_error():
    strings = StringList(["", "front", "last"])

    assert strings.front() == ""
    assert strings.last == "last"
    assert strings.back() == "last"
    assert strings.pop() == "last"
    assert strings.pop() == "front"
    assert strings.pop() == ""
    with pytest.raises(IndexError):
        strings.pop()
    assert_old_integer_argument_error(lambda: StringList(["b", "a"]).sort("0"))


def test_unicode_very_long_iteration_and_none_sort_flag_error():
    very_long = "x" * 10000
    strings = StringList(["å", "東京", "😀", very_long])

    assert list(strings) == ["å", "東京", "😀", very_long]
    assert strings.index("東京") == 1
    assert_old_integer_argument_error(lambda: strings.sort(None))


def test_sort_ordering_and_list_sort_flag_conversion_error():
    strings = StringList(["b10", "b2", "A", "", "a"])

    strings.sort()

    assert strings.strings == ["", "A", "a", "b10", "b2"]
    assert_old_integer_argument_error(lambda: strings.sort([]))


def test_index_missing_item_and_object_contains_error():
    strings = StringList(["repeat", "unique", "repeat"])

    assert strings.index("repeat") == 0
    assert strings.index("missing") == -1
    with pytest.raises(KeyError):
        strings.index(123)
    assert_old_string_argument_error(lambda: object() in strings)


def test_equality_with_stringlist_and_object_sort_flag_error():
    strings = StringList(["same", "values"])
    same = StringList(["same", "values"])
    different = StringList(["same", "other"])

    assert strings == same
    assert strings == ["same", "values"]
    assert not (strings == different)
    assert not (strings == ["same"])
    assert_old_integer_argument_error(lambda: strings.sort(object()))
