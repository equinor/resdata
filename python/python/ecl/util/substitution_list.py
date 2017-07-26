from cwrap import BaseCClass
from ecl.util import UtilPrototype


class SubstitutionList(BaseCClass):
    TYPE_NAME = "subst_list"

    _alloc = UtilPrototype("void* subst_list_alloc(void*)" , bind = False)
    _free = UtilPrototype("void subst_list_free(subst_list)")
    _size = UtilPrototype("int subst_list_get_size(subst_list)")
    _iget_key = UtilPrototype("char* subst_list_iget_key(subst_list, int)")
    _iget_value = UtilPrototype("char* subst_list_iget_value(subst_list, int)")
    _get_value = UtilPrototype("char* subst_list_get_value(subst_list, char*)")
    _has_key = UtilPrototype("bool subst_list_has_key(subst_list, char*)")
    _get_doc = UtilPrototype("char* subst_list_get_doc_string(subst_list, char*)")
    _append_copy = UtilPrototype("void subst_list_append_copy(subst_list, char*, char*, char*)")


    def __init__(self):
        c_ptr = self._alloc(0)
        super(SubstitutionList, self).__init__(c_ptr)

    def __len__(self):
        return self._size()

    def add_item(self, key, value, doc_string=""):
        self._append_copy(key, value, doc_string)


    def keys(self):
        key_list = []
        for i in range(len(self)):
            key_list.append( self._iget_key( i ))
        return key_list


    def __iter__(self):
        index = 0
        keys = self.keys( )
        for index in range(len(self)):
            key = keys[index]
            yield (key , self[key], self.doc(key))

    def __contains__(self, key):
        if not isinstance(key, str):
            return False
        return self._has_key( key )

    def __getitem__(self, key):
        if key in self:
            return self._get_value( key )
        else:
            raise KeyError("No such key:%s" % key)


    def get(self, key, default=None):
        return self[key] if key in self else default

    def doc(self,key):
        if key in self:
            return self._get_doc( key )
        else:
            raise KeyError("No such key:%s" % key)


    def index_for_key(self, key):
        if not key in self:
            raise KeyError("Key '%s' not in substitution list!" % key)

        for index, key_val_doc in enumerate(self):
            if key == key_val_doc[0]:
                return index

        return None  # Should never happen!

    def free(self):
        self._free()


    def __repr__(self):
        return self._create_repr('len=%d' % len(self))

    def __str__(self):
        return 'SubstitutionList{%s}' % ", ".join(map(str, self.keys()))


monkey_the_camel(SubstitutionList, 'addItem', SubstitutionList.add_item)
monkey_the_camel(SubstitutionList, 'indexForKey', SubstitutionList.index_for_key)
