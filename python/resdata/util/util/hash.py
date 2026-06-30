from ctypes import c_void_p

from cwrap import BaseCClass

import resdata.util.util._hash as _hash

from .stringlist import StringList


class Hash(BaseCClass):
    TYPE_NAME = "rd_hash"

    """
    Base hash class that supports string:void* values
    """

    def __init__(self):
        c_ptr = _hash._alloc()
        super().__init__(c_ptr)

    def __len__(self):
        return _hash._size(self)

    def __getitem__(self, key):
        if _hash._has_key(self, key):
            return _hash._get(self, key)
        else:
            raise KeyError("Hash does not have key: %s" % key)

    def __setitem__(self, key, value):
        if isinstance(value, c_void_p):
            _hash._insert_ref(self, key, value)
        else:
            raise ValueError("Hash does not support type: %s" % value.__class__)

    def __contains__(self, key) -> bool:
        return _hash._has_key(self, key)

    def __iter__(self):
        for key in self.keys():
            yield key

    def keys(self) -> StringList:
        return StringList.createPythonObject(_hash._keys(self))

    def free(self):
        _hash._free(self)

    def __str__(self):
        return str(["%s: %s" % (key, self[key]) for key in self.keys()])


class StringHash(Hash):
    TYPE_NAME = "rd_string_hash"

    def __init__(self):
        super().__init__()

    def __setitem__(self, key, value):
        if isinstance(value, str):
            _hash._insert_string(self, key, value)
        else:
            raise ValueError("StringHash does not support type: %s" % value.__class__)

    def __getitem__(self, key):
        if key in self:
            return _hash._get_string(self, key)
        else:
            raise KeyError("Hash does not have key: %s" % key)


class IntegerHash(Hash):
    TYPE_NAME = "rd_int_hash"

    def __init__(self):
        super().__init__()

    def __setitem__(self, key, value):
        if isinstance(value, int):
            _hash._insert_int(self, key, value)
        else:
            raise ValueError("IntegerHash does not support type: %s" % value.__class__)

    def __getitem__(self, key):
        if key in self:
            return _hash._get_int(self, key)
        else:
            raise KeyError("Hash does not have key: %s" % key)


class DoubleHash(Hash):
    TYPE_NAME = "rd_double_hash"

    def __init__(self):
        super().__init__()

    def __setitem__(self, key, value):
        if isinstance(value, int):
            value = float(value)

        if isinstance(value, float):
            _hash._insert_double(self, key, value)
        else:
            raise ValueError("DoubleHash does not support type: %s" % value.__class__)

    def __getitem__(self, key):
        if key in self:
            return _hash._get_double(self, key)
        else:
            raise KeyError("Hash does not have key: %s" % key)
