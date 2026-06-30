from cwrap import BaseCClass

import resdata.util.util._permutation_vector as _permutation_vector


class PermutationVector(BaseCClass):
    TYPE_NAME = "rd_permutation_vector"

    def __init__(self):
        raise NotImplementedError("Can not instantiate PermutationVector directly")

    def __len__(self):
        return _permutation_vector._size(self)

    def __str__(self):
        s = "("
        for index in self:
            s += " %d" % index
        return s + ")"

    def __getitem__(self, index):
        if index < 0:
            index += len(self)

        if 0 <= index < len(self):
            return _permutation_vector._iget(self, index)
        else:
            raise IndexError("Invalid index:%d" % index)

    def free(self):
        _permutation_vector._free(self)
