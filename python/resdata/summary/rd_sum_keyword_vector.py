# Observe that there is some convention conflict with the C code
# regarding order of arguments: The C code generally takes the time
# index as the first argument and the key/key_index as second
# argument. In the python code this order has been reversed.

from cwrap import BaseCClass

import resdata.summary._rd_sum_keyword_vector as _rd_sum_keyword_vector


class SummaryKeyWordVector(BaseCClass):
    TYPE_NAME = "rd_sum_vector"

    def __init__(self, rd_sum, add_keywords=False):
        c_pointer = _rd_sum_keyword_vector._alloc(rd_sum, add_keywords)
        super().__init__(c_pointer)

    def __getitem__(self, index):
        if index < 0:
            index += len(self)

        if index >= len(self):
            raise IndexError("Out of range")

        return _rd_sum_keyword_vector._iget_key(self, index)

    def __len__(self):
        return _rd_sum_keyword_vector._get_size(self)

    def free(self):
        _rd_sum_keyword_vector._free(self)

    def add_keyword(self, keyword):
        success = _rd_sum_keyword_vector._add(self, keyword)
        if not success:
            raise KeyError("Failed to add keyword to vector")

    def add_keywords(self, keyword_pattern):
        _rd_sum_keyword_vector._add_multiple(self, keyword_pattern)

    def __repr__(self):
        return self._create_repr("len=%d" % len(self))

    def copy(self, rd_sum):
        return SummaryKeyWordVector.createPythonObject(
            _rd_sum_keyword_vector._alloc_copy(self, rd_sum)
        )
