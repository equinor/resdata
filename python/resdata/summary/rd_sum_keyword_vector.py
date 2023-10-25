import numpy
import datetime

# Observe that there is some convention conflict with the C code
# regarding order of arguments: The C code generally takes the time
# index as the first argument and the key/key_index as second
# argument. In the python code this order has been reversed.

from cwrap import BaseCClass
from resdata.util.util import monkey_the_camel
from resdata import ResdataPrototype


class SummaryKeyWordVector(BaseCClass):
    TYPE_NAME = "rd_sum_vector"
    _alloc = ResdataPrototype("void* rd_sum_vector_alloc(rd_sum, bool)", bind=False)
    _alloc_copy = ResdataPrototype(
        "rd_sum_vector_obj rd_sum_vector_alloc_layout_copy(rd_sum_vector, rd_sum)"
    )
    _free = ResdataPrototype("void rd_sum_vector_free(rd_sum_vector)")
    _add = ResdataPrototype("bool rd_sum_vector_add_key(rd_sum_vector,  char*)")
    _add_multiple = ResdataPrototype(
        "void rd_sum_vector_add_keys(rd_sum_vector,  char*)"
    )
    _get_size = ResdataPrototype("int rd_sum_vector_get_size(rd_sum_vector)")
    _iget_key = ResdataPrototype("char* rd_sum_vector_iget_key(rd_sum_vector, int)")

    def __init__(self, rd_sum, add_keywords=False):
        c_pointer = self._alloc(rd_sum, add_keywords)
        super(SummaryKeyWordVector, self).__init__(c_pointer)

    def __getitem__(self, index):
        if index < 0:
            index += len(self)

        if index >= len(self):
            raise IndexError("Out of range")

        return self._iget_key(index)

    def __len__(self):
        return self._get_size()

    def free(self):
        self._free()

    def add_keyword(self, keyword):
        success = self._add(keyword)
        if not success:
            raise KeyError("Failed to add keyword to vector")

    def add_keywords(self, keyword_pattern):
        self._add_multiple(keyword_pattern)

    def __repr__(self):
        return self._create_repr("len=%d" % len(self))

    def copy(self, rd_sum):
        return self._alloc_copy(rd_sum)


monkey_the_camel(SummaryKeyWordVector, "addKeyword", SummaryKeyWordVector.add_keyword)
monkey_the_camel(SummaryKeyWordVector, "addKeywords", SummaryKeyWordVector.add_keywords)
