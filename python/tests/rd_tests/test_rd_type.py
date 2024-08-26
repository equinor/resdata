from resdata.util.test import TestAreaContext
from tests import ResdataTest

from resdata import ResDataType, ResdataTypeEnum


def get_const_size_types():
    return ResdataTypeEnum.enums()[:-1:]


class ResDataTypeTest(ResdataTest):
    # All of the below should list their elements in the same order as
    # ResdataTypeEnum!
    # [char, float, double, int, bool, mess]

    CONST_SIZES = [8, 4, 8, 4, 4, 0]

    CONST_VERIFIERS = [
        ResDataType.is_char,
        ResDataType.is_float,
        ResDataType.is_double,
        ResDataType.is_int,
        ResDataType.is_bool,
        ResDataType.is_mess,
    ]

    CONST_NAMES = ["CHAR", "REAL", "DOUB", "INTE", "LOGI", "MESS"]

    STRING_NAMES = ["C000", "C010", "C020", "C042", "C999"]

    STRING_SIZES = [0, 10, 20, 42, 999]

    TYPES = get_const_size_types() + len(STRING_SIZES) * [
        ResdataTypeEnum.RD_STRING_TYPE
    ]

    SIZES = CONST_SIZES + STRING_SIZES

    NAMES = CONST_NAMES + STRING_NAMES

    def test_alloc_from_type(self):
        types, sizes = get_const_size_types(), self.CONST_SIZES
        for rd_type, element_size in zip(types, sizes):
            data_type = ResDataType(rd_type)
            self.assertEqual(rd_type, data_type.type)
            self.assertEqual(element_size, data_type.element_size)

    def test_invalid_string_alloc(self):
        with self.assertRaises(ValueError):
            data_type = ResDataType(ResdataTypeEnum.RD_STRING_TYPE)

        with self.assertRaises(ValueError):
            data_type = ResDataType(ResdataTypeEnum.RD_STRING_TYPE, -1)

        with self.assertRaises(ValueError):
            data_type = ResDataType(ResdataTypeEnum.RD_STRING_TYPE, 1000)

    def test_alloc(self):
        for rd_type, element_size in zip(self.TYPES, self.SIZES):
            data_type = ResDataType(rd_type, element_size)
            self.assertEqual(rd_type, data_type.type)
            self.assertEqual(element_size, data_type.element_size)

    def test_type_verifiers(self):
        test_base = zip(self.TYPES, self.SIZES, self.CONST_VERIFIERS)
        for rd_type, elem_size, verifier in test_base:
            data_type = ResDataType(rd_type, elem_size)
            self.assertTrue(verifier(data_type))

    def test_get_type_name(self):
        test_base = zip(self.TYPES, self.SIZES, self.NAMES)
        for rd_type, elem_size, type_name in test_base:
            data_type = ResDataType(rd_type, elem_size)
            self.assertEqual(type_name, data_type.type_name)

    def test_initialization_validation(self):
        invalid_args = [
            (None, 0, self.CONST_NAMES[0]),
            (1, None, self.CONST_NAMES[0]),
            (1, 0, self.CONST_NAMES[0]),
            (None, None, None),
            (None, 12, None),
        ]

        for inv_arg in invalid_args:
            with self.assertRaises(ValueError):
                ResDataType(inv_arg[0], inv_arg[1], inv_arg[2])

    def test_create_from_type_name(self):
        test_base = zip(self.TYPES, self.SIZES, self.NAMES)
        for rd_type, elem_size, type_name in test_base:
            data_type = ResDataType.create_from_type_name(type_name)
            self.assertEqual(rd_type, data_type.type)
            self.assertEqual(elem_size, data_type.element_size)
            self.assertEqual(type_name, data_type.type_name)

    def test_is_numeric(self):
        numeric_types = [
            ResdataTypeEnum.RD_INT_TYPE,
            ResdataTypeEnum.RD_FLOAT_TYPE,
            ResdataTypeEnum.RD_DOUBLE_TYPE,
        ]

        for rd_type in numeric_types:
            self.assertTrue(ResDataType(rd_type).is_numeric())

        for rd_type in set(get_const_size_types()) - set(numeric_types):
            self.assertFalse(ResDataType(rd_type).is_numeric())

        for elem_size in self.STRING_SIZES:
            data_type = ResDataType(ResdataTypeEnum.RD_STRING_TYPE, elem_size)
            self.assertFalse(data_type.is_numeric())

    def test_equals(self):
        test_base = zip(self.TYPES, self.SIZES)
        for rd_type, elem_size in test_base:
            a = ResDataType(rd_type, elem_size)
            b = ResDataType(rd_type, elem_size)

            self.assertTrue(a.is_equal(b))
            self.assertEqual(a, b)

            for otype, osize in set(test_base) - {rd_type, elem_size}:
                self.assertFalse(a.is_equal(ResDataType(otype, osize)))
                self.assertNotEqual(a, ResDataType(otype, osize))

    def test_hash(self):
        all_types = set()
        test_base = list(zip(self.TYPES, self.SIZES))

        for index, (rd_type, elem_size) in enumerate(test_base):
            all_types.add(ResDataType(rd_type, elem_size))
            self.assertEqual(index + 1, len(all_types))

        for _index, (rd_type, elem_size) in enumerate(test_base):
            all_types.add(ResDataType(rd_type, elem_size))

        for _index, rd_type in enumerate(get_const_size_types()):
            all_types.add(ResDataType(rd_type))

        self.assertEqual(len(test_base), len(all_types))
