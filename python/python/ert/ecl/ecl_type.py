#  Copyright (C) 2017  Statoil ASA, Norway. 
#   
#  The file 'ecl_type.py' is part of ERT - Ensemble based Reservoir Tool. 
#   
#  ERT is free software: you can redistribute it and/or modify 
#  it under the terms of the GNU General Public License as published by 
#  the Free Software Foundation, either version 3 of the License, or 
#  (at your option) any later version. 
#   
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#  FITNESS FOR A PARTICULAR PURPOSE.   
#   
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
#  for more details.

from cwrap import BaseCClass
from ert.ecl import EclTypeEnum, EclUtil, EclPrototype

class EclDataType(BaseCClass):

    TYPE_NAME = "ecl_data_type"

    _alloc            = EclPrototype("void* python_ecl_type_alloc(ecl_type_enum, size_t)", bind=False)
    _alloc_from_type  = EclPrototype("void* python_ecl_type_alloc_from_type(ecl_type_enum)", bind=False)
    _alloc_from_name  = EclPrototype("void* python_ecl_type_alloc_from_name(char*)", bind = False)
    _free             = EclPrototype("void python_ecl_type_free(ecl_data_type)")
    _get_type         = EclPrototype("ecl_type_enum python_ecl_type_get_type(ecl_data_type)")
    _get_element_size = EclPrototype("size_t python_ecl_type_get_element_size(ecl_data_type)")
    _is_int           = EclPrototype("bool python_ecl_type_is_int(ecl_data_type)")
    _is_char          = EclPrototype("bool python_ecl_type_is_char(ecl_data_type)")
    _is_float         = EclPrototype("bool python_ecl_type_is_float(ecl_data_type)")
    _is_double        = EclPrototype("bool python_ecl_type_is_double(ecl_data_type)")
    _is_mess          = EclPrototype("bool python_ecl_type_is_mess(ecl_data_type)")
    _is_bool          = EclPrototype("bool python_ecl_type_is_bool(ecl_data_type)")
    _get_type_name    = EclPrototype("char* python_ecl_type_get_type_name(ecl_data_type)")
    _is_numeric       = EclPrototype("bool python_ecl_type_is_numeric(ecl_data_type)")
    _is_equal         = EclPrototype("bool python_ecl_type_is_equal(ecl_data_type, ecl_data_type)")

    def __init__(self, type_enum = None, element_size = None, type_name = None):
        self._assert_valid_arguments(type_enum, element_size, type_name)

        if type_name:
            c_ptr = self._alloc_from_name(type_name)
        elif not element_size:
            c_ptr = self._alloc_from_type(type_enum)
        else:
            c_ptr = self._alloc(type_enum, element_size)

        super(EclDataType, self).__init__(c_ptr)

    def _assert_valid_arguments(self, type_enum, element_size, type_name):
        if type_name is not None:
            if type_enum is not None or element_size is not None:
                raise ValueError("Type name given (%s). Expected both type_enum and element_size to be None")
        elif type_enum is None:
            raise ValueError("Both type_enum and type_name is None!")

    def get_type(self):
        return self._get_type()

    def get_element_size(self):
        return self._get_element_size()

    def free(self):
        self._free()

    def is_int(self):
        return self._is_int()

    def is_char(self):
        return self._is_char()

    def is_float(self):
        return self._is_float()

    def is_double(self):
        return self._is_double()

    def is_mess(self):
        return self._is_mess()

    def is_bool(self):
        return self._is_bool()

    def get_type_name(self):
        return self._get_type_name()

    def is_numeric(self):
        return self._is_numeric()

    def is_equal(self, other):
        return self._is_equal(other)

    @classmethod
    def create_from_type_name(cls, name):
        return EclDataType(type_name = name)
