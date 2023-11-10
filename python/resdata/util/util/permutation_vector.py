from cwrap import BaseCClass
from resdata import ResdataPrototype


class PermutationVector(BaseCClass):
    TYPE_NAME = "rd_permutation_vector"
    _free = ResdataPrototype("void   perm_vector_free( rd_permutation_vector )")
    _size = ResdataPrototype("int    perm_vector_get_size( rd_permutation_vector )")
    _iget = ResdataPrototype("int    perm_vector_iget( rd_permutation_vector , int)")

    def __init__(self):
        raise NotImplementedError("Can not instantiate PermutationVector directly")

    def __len__(self):
        return self._size()

    def __str__(self):
        s = "("
        for index in self:
            s += " %d" % index
        return s + ")"

    def __getitem__(self, index):
        if index < 0:
            index += len(self)

        if 0 <= index < len(self):
            return self._iget(index)
        else:
            raise IndexError("Invalid index:%d" % index)

    def free(self):
        self._free()
