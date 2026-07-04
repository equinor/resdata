from cwrap import BaseCClass, BaseCEnum

import resdata.types._type as _rd_type


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


class ResDataType(BaseCClass):
    TYPE_NAME = "rd_data_type"

    def __init__(self, type_enum=None, element_size=None, type_name=None):
        self._assert_valid_arguments(type_enum, element_size, type_name)

        if type_name:
            c_ptr = _rd_type._create_from_name(type_name)
        elif element_size is None:
            c_ptr = _rd_type._create_from_type(type_enum)
        else:
            c_ptr = _rd_type._create(type_enum, element_size)

        super().__init__(c_ptr)

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
        return ResdataTypeEnum(_rd_type._get_type(self))

    @property
    def element_size(self):
        return _rd_type._get_sizeof_iotype(self)

    @property
    def type_name(self):
        return _rd_type._name(self)

    def free(self):
        _rd_type._free(self)

    def is_int(self):
        return _rd_type._is_int(self)

    def is_char(self):
        return _rd_type._is_char(self)

    def is_float(self):
        return _rd_type._is_float(self)

    def is_double(self):
        return _rd_type._is_double(self)

    def is_mess(self):
        return _rd_type._is_mess(self)

    def is_bool(self):
        return _rd_type._is_bool(self)

    def is_string(self):
        return _rd_type._is_string(self)

    def is_numeric(self):
        return _rd_type._is_numeric(self)

    def is_equal(self, other):
        return _rd_type._is_equal(self, other)

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
    class classproperty:
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
