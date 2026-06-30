import re

import pytest
from resdata import ResDataType, ResdataTypeEnum


def assert_rejects_raw_type_enum(value, element_size=None):
    with pytest.raises(TypeError):
        if element_size is None:
            ResDataType(value)
        else:
            ResDataType(value, element_size)


def test_char_type_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_CHAR
    assert data_type.type is ResdataTypeEnum.RD_CHAR_TYPE
    assert int(data_type.type) == 0
    assert data_type.element_size == 8
    assert data_type.is_char()
    assert not data_type.is_string()
    assert re.fullmatch(r"ResDataType\(\) at 0x[0-9a-f]+", repr(data_type))
    assert_rejects_raw_type_enum(0)


def test_float_type_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_FLOAT
    assert data_type.type is ResdataTypeEnum.RD_FLOAT_TYPE
    assert int(data_type.type) == 1
    assert data_type.element_size == 4
    assert data_type.is_float()
    assert data_type.is_numeric()
    assert not data_type.is_double()
    assert_rejects_raw_type_enum(1)


def test_double_type_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_DOUBLE
    assert data_type.type is ResdataTypeEnum.RD_DOUBLE_TYPE
    assert int(data_type.type) == 2
    assert data_type.element_size == 8
    assert data_type.is_double()
    assert data_type.is_numeric()
    assert not data_type.is_float()
    assert_rejects_raw_type_enum(2)


def test_int_type_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_INT
    assert data_type.type is ResdataTypeEnum.RD_INT_TYPE
    assert int(data_type.type) == 3
    assert data_type.element_size == 4
    assert data_type.is_int()
    assert data_type.is_numeric()
    assert not data_type.is_bool()
    assert_rejects_raw_type_enum(3)


def test_bool_type_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_BOOL
    assert data_type.type is ResdataTypeEnum.RD_BOOL_TYPE
    assert int(data_type.type) == 4
    assert data_type.element_size == 4
    assert data_type.is_bool()
    assert not data_type.is_numeric()
    assert not data_type.is_int()
    assert_rejects_raw_type_enum(4)


def test_mess_type_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_MESS
    assert data_type.type is ResdataTypeEnum.RD_MESS_TYPE
    assert int(data_type.type) == 5
    assert data_type.element_size == 0
    assert data_type.is_mess()
    assert not data_type.is_string()
    assert not data_type.is_numeric()
    assert_rejects_raw_type_enum(5)


def test_zero_length_string_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_STRING(0)
    assert data_type.type is ResdataTypeEnum.RD_STRING_TYPE
    assert int(data_type.type) == 7
    assert data_type.element_size == 0
    assert data_type.type_name == "C000"
    assert data_type.is_string()
    assert_rejects_raw_type_enum(7, 0)


def test_variable_length_string_requires_enum_object_not_raw_int():
    data_type = ResDataType.RD_STRING(42)
    assert data_type.type is ResdataTypeEnum.RD_STRING_TYPE
    assert int(data_type.type) == 7
    assert data_type.element_size == 42
    assert data_type.type_name == "C042"
    assert data_type == ResDataType.create_from_type_name("C042")
    assert_rejects_raw_type_enum(7, 42)


def test_equivalent_types_compare_equal_but_raw_int_constructor_is_rejected():
    from_factory = ResDataType.RD_INT
    from_enum = ResDataType(ResdataTypeEnum.RD_INT_TYPE)
    from_name = ResDataType.create_from_type_name("INTE")
    assert from_factory == from_enum == from_name
    assert from_factory.is_equal(from_enum)
    assert from_factory != ResDataType.RD_FLOAT
    assert hash(from_factory) == hash(from_enum)
    assert_rejects_raw_type_enum(3, 4)


def test_bool_values_are_not_accepted_as_type_enums():
    data_type = ResDataType.RD_BOOL
    assert data_type.type is ResdataTypeEnum.RD_BOOL_TYPE
    assert data_type.element_size == 4
    assert data_type.is_bool()
    assert not data_type.is_int()
    assert_rejects_raw_type_enum(True)
    assert_rejects_raw_type_enum(False)
