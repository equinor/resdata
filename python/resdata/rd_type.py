from cwrap import BaseCClass, BaseCEnum
from resdata import ResdataPrototype


class ResdataTypeEnum(BaseCEnum):
    TYPE_NAME = "rd_type_enum"
    RD_CHAR_TYPE = None
    RD_FLOAT_TYPE = None
    RD_DOUBLE_TYPE = None
    RD_INT_TYPE = None
    RD_BOOL_TYPE = None
    RD_MESS_TYPE = None
    RD_STRING_TYPE = None


ResdataTypeEnum.addEnum("RD_CHAR_TYPE", 0)
ResdataTypeEnum.addEnum("RD_FLOAT_TYPE", 1)
ResdataTypeEnum.addEnum("RD_DOUBLE_TYPE", 2)
ResdataTypeEnum.addEnum("RD_INT_TYPE", 3)
ResdataTypeEnum.addEnum("RD_BOOL_TYPE", 4)
ResdataTypeEnum.addEnum("RD_MESS_TYPE", 5)
ResdataTypeEnum.addEnum("RD_STRING_TYPE", 7)

# -----------------------------------------------------------------


class ResDataType(BaseCClass):
    TYPE_NAME = "rd_data_type"

    _alloc = ResdataPrototype(
        "void* rd_type_alloc_python(rd_type_enum, size_t)", bind=False
    )
    _alloc_from_type = ResdataPrototype(
        "void* rd_type_alloc_from_type_python(rd_type_enum)", bind=False
    )
    _alloc_from_name = ResdataPrototype(
        "void* rd_type_alloc_from_name_python(char*)", bind=False
    )
    _free = ResdataPrototype("void rd_type_free_python(rd_data_type)")
    _get_type = ResdataPrototype("rd_type_enum rd_type_get_type_python(rd_data_type)")
    _get_element_size = ResdataPrototype(
        "size_t rd_type_get_sizeof_iotype_python(rd_data_type)"
    )
    _is_int = ResdataPrototype("bool rd_type_is_int_python(rd_data_type)")
    _is_char = ResdataPrototype("bool rd_type_is_char_python(rd_data_type)")
    _is_float = ResdataPrototype("bool rd_type_is_float_python(rd_data_type)")
    _is_double = ResdataPrototype("bool rd_type_is_double_python(rd_data_type)")
    _is_mess = ResdataPrototype("bool rd_type_is_mess_python(rd_data_type)")
    _is_bool = ResdataPrototype("bool rd_type_is_bool_python(rd_data_type)")
    _is_string = ResdataPrototype("bool rd_type_is_string_python(rd_data_type)")
    _get_name = ResdataPrototype("char* rd_type_alloc_name_python(rd_data_type)")
    _is_numeric = ResdataPrototype("bool rd_type_is_numeric_python(rd_data_type)")
    _is_equal = ResdataPrototype(
        "bool rd_type_is_equal_python(rd_data_type, rd_data_type)"
    )

    def __init__(self, type_enum=None, element_size=None, type_name=None):
        self._assert_valid_arguments(type_enum, element_size, type_name)

        if type_name:
            c_ptr = self._alloc_from_name(type_name)
        elif element_size is None:
            c_ptr = self._alloc_from_type(type_enum)
        else:
            c_ptr = self._alloc(type_enum, element_size)

        super(ResDataType, self).__init__(c_ptr)

    def _assert_valid_arguments(self, type_enum, element_size, type_name):
        if type_name is not None:
            if type_enum is not None or element_size is not None:
                err_msg = (
                    "Type name given (%s). Expected both "
                    + "type_enum and element_size to be None"
                )
                raise ValueError(err_msg % type_name)

        elif type_enum is None:
            raise ValueError("Both type_enum and type_name is None!")

        elif type_enum == ResdataTypeEnum.RD_STRING_TYPE:
            if element_size is None:
                raise ValueError(
                    "When creating an RD_STRING one must " + "provide an element size!"
                )

            if not (0 <= element_size <= 999):
                raise ValueError(
                    "Expected element_size to be in the range "
                    + "[0, 999], was: %d" % element_size
                )

    @property
    def type(self):
        return self._get_type()

    @property
    def element_size(self):
        return self._get_element_size()

    @property
    def type_name(self):
        return self._get_name()

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

    def is_string(self):
        return self._is_string()

    def is_numeric(self):
        return self._is_numeric()

    def is_equal(self, other):
        return self._is_equal(other)

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.is_equal(other)
        return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash((self.type, self.element_size))

    @classmethod
    def create_from_type_name(cls, name):
        return ResDataType(type_name=name)

    # Enables one to fetch a type as ResDataType.RD_XXXX
    class classproperty(object):
        def __init__(self, fget):
            self.fget = fget

        def __get__(self, owner_self, owner_cls):
            return self.fget(owner_cls)

    @classproperty
    def RD_INT(cls):
        return ResDataType(ResdataTypeEnum.RD_INT_TYPE)

    @classproperty
    def RD_FLOAT(cls):
        return ResDataType(ResdataTypeEnum.RD_FLOAT_TYPE)

    @classproperty
    def RD_DOUBLE(cls):
        return ResDataType(ResdataTypeEnum.RD_DOUBLE_TYPE)

    @classproperty
    def RD_BOOL(cls):
        return ResDataType(ResdataTypeEnum.RD_BOOL_TYPE)

    @classproperty
    def RD_MESS(cls):
        return ResDataType(ResdataTypeEnum.RD_MESS_TYPE)

    @classproperty
    def RD_CHAR(cls):
        return ResDataType(ResdataTypeEnum.RD_CHAR_TYPE)

    @classmethod
    def RD_STRING(cls, elem_size):
        return ResDataType(ResdataTypeEnum.RD_STRING_TYPE, elem_size)
