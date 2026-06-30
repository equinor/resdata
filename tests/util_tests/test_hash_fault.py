from ctypes import c_void_p

import pytest
from resdata.util.util import DoubleHash, Hash, IntegerHash, StringHash
from resdata.util.util.stringlist import StringList


def test_base_hash_returns_plain_int_for_pointer_value():
    hash_obj = Hash()

    hash_obj["pointer"] = c_void_p(512)

    value = hash_obj["pointer"]
    assert value == 512
    assert type(value) is int


def test_base_hash_returns_none_for_null_pointer_value():
    hash_obj = Hash()

    hash_obj["null"] = c_void_p(None)

    assert hash_obj["null"] is None


def test_base_hash_getitem_result_is_not_c_void_p_wrapper():
    hash_obj = Hash()
    pointer = c_void_p(4096)

    hash_obj["pointer"] = pointer

    value = hash_obj["pointer"]
    assert value == pointer.value
    assert not isinstance(value, c_void_p)


def test_base_hash_returned_pointer_supports_integer_arithmetic():
    hash_obj = Hash()

    hash_obj["base"] = c_void_p(1000)

    assert hash_obj["base"] + 24 == 1024


def test_base_hash_overwritten_value_returns_latest_plain_address():
    hash_obj = Hash()

    hash_obj["same"] = c_void_p(111)
    hash_obj["same"] = c_void_p(222)

    value = hash_obj["same"]
    assert len(hash_obj) == 1
    assert value == 222
    assert type(value) is int


def test_base_hash_str_uses_plain_pointer_addresses():
    hash_obj = Hash()

    hash_obj["alpha"] = c_void_p(1)
    hash_obj["beta"] = c_void_p(2)

    rendered = str(hash_obj)
    assert "alpha: 1" in rendered
    assert "beta: 2" in rendered
    assert "c_void_p" not in rendered


def test_base_hash_iteration_values_are_plain_addresses():
    hash_obj = Hash()
    expected = {"first": 10, "second": 20, "third": 30}

    for key, value in expected.items():
        hash_obj[key] = c_void_p(value)

    assert list(hash_obj) == list(hash_obj.keys())
    values = [hash_obj[key] for key in hash_obj]
    assert sorted(values) == [10, 20, 30]
    assert all(type(value) is int for value in values)


def test_typed_hashes_round_trip_and_base_hash_still_returns_int():
    string_hash = StringHash()
    integer_hash = IntegerHash()
    double_hash = DoubleHash()
    base_hash = Hash()

    string_hash["name"] = "resdata"
    integer_hash["answer"] = 42
    double_hash["ratio"] = 1.25
    base_hash["pointer"] = c_void_p(1234)

    assert string_hash["name"] == "resdata"
    assert integer_hash["answer"] == 42
    assert double_hash["ratio"] == 1.25
    assert base_hash["pointer"] == 1234
    assert type(base_hash["pointer"]) is int


def test_wrong_value_types_raise_value_error_and_base_hash_remains_usable():
    base_hash = Hash()
    string_hash = StringHash()
    integer_hash = IntegerHash()
    double_hash = DoubleHash()

    with pytest.raises(ValueError):
        base_hash["bad"] = 5
    with pytest.raises(ValueError):
        string_hash["bad"] = 5
    with pytest.raises(ValueError):
        integer_hash["bad"] = 1.5
    with pytest.raises(ValueError):
        double_hash["bad"] = "1.5"

    base_hash["good"] = c_void_p(55)
    assert base_hash["good"] == 55
    assert type(base_hash["good"]) is int


def test_missing_keys_empty_hash_keys_type_and_base_hash_int_return():
    base_hash = Hash()

    assert len(base_hash) == 0
    assert isinstance(base_hash.keys(), StringList)
    with pytest.raises(KeyError):
        base_hash["missing"]

    base_hash["present"] = c_void_p(77)
    assert "present" in base_hash
    assert "missing" not in base_hash
    assert base_hash["present"] == 77
    assert type(base_hash["present"]) is int
