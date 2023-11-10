from ctypes import c_void_p

from cwrap import BaseCClass
from resdata import ResdataPrototype
from resdata.util.util import StringList


class Hash(BaseCClass):
    TYPE_NAME = "rd_hash"
    _alloc = ResdataPrototype("void* hash_alloc()", bind=False)
    _free = ResdataPrototype("void hash_free(rd_hash)")
    _size = ResdataPrototype("int hash_get_size(rd_hash)")
    _keys = ResdataPrototype("rd_stringlist_obj hash_alloc_stringlist(rd_hash)")
    _has_key = ResdataPrototype("bool hash_has_key(rd_hash, char*)")
    _get = ResdataPrototype("void* hash_get(rd_hash, char*)")
    _insert_ref = ResdataPrototype("void hash_insert_ref(rd_hash, char*, void*)")

    """
    Base hash class that supports string:void* values
    """

    def __init__(self):
        c_ptr = self._alloc()
        super(Hash, self).__init__(c_ptr)

    def __len__(self):
        return self._size()

    def __getitem__(self, key):
        if self._has_key(key):
            return self._get(key)
        else:
            raise KeyError("Hash does not have key: %s" % key)

    def __setitem__(self, key, value):
        if isinstance(value, c_void_p):
            self._insert_ref(key, value)
        else:
            raise ValueError("Hash does not support type: %s" % value.__class__)

    def __contains__(self, key):
        """@rtype: bool"""
        return self._has_key(key)

    def __iter__(self):
        for key in self.keys():
            yield key

    def keys(self):
        """@rtype: StringList"""
        return self._keys()

    def free(self):
        self._free()

    def __str__(self):
        return str(["%s: %s" % (key, self[key]) for key in self.keys()])


class StringHash(Hash):
    TYPE_NAME = "rd_string_hash"
    _get_string = ResdataPrototype("char* hash_get_string(rd_hash, char*)")
    _insert_string = ResdataPrototype("void hash_insert_string(rd_hash, char*, char*)")

    def __init__(self):
        super(StringHash, self).__init__()

    def __setitem__(self, key, value):
        if isinstance(value, str):
            self._insert_string(key, value)
        else:
            raise ValueError("StringHash does not support type: %s" % value.__class__)

    def __getitem__(self, key):
        if key in self:
            return self._get_string(key)
        else:
            raise KeyError("Hash does not have key: %s" % key)


class IntegerHash(Hash):
    TYPE_NAME = "rd_int_hash"
    _get_int = ResdataPrototype("int hash_get_int(rd_hash, char*)")
    _insert_int = ResdataPrototype("void hash_insert_int(rd_hash, char*, int)")

    def __init__(self):
        super(IntegerHash, self).__init__()

    def __setitem__(self, key, value):
        if isinstance(value, int):
            self._insert_int(key, value)
        else:
            raise ValueError("IntegerHash does not support type: %s" % value.__class__)

    def __getitem__(self, key):
        if key in self:
            return self._get_int(key)
        else:
            raise KeyError("Hash does not have key: %s" % key)


class DoubleHash(Hash):
    TYPE_NAME = "rd_double_hash"
    _get_double = ResdataPrototype("double hash_get_double(rd_hash, char*)")
    _insert_double = ResdataPrototype("void hash_insert_double(rd_hash, char*, double)")

    def __init__(self):
        super(DoubleHash, self).__init__()

    def __setitem__(self, key, value):
        if isinstance(value, int):
            value = float(value)

        if isinstance(value, float):
            self._insert_double(key, value)
        else:
            raise ValueError("DoubleHash does not support type: %s" % value.__class__)

    def __getitem__(self, key):
        if key in self:
            return self._get_double(key)
        else:
            raise KeyError("Hash does not have key: %s" % key)
