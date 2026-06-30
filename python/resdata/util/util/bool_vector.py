from __future__ import annotations

import resdata.util.util._bool_vector as _bool_vector

from .int_vector import IntVector
from .vector_template import VectorTemplate


class BoolVector(VectorTemplate):
    TYPE_NAME = "rd_bool_vector"
    default_format = "%8d"

    _init_linear = None

    @staticmethod
    def _alloc(initial_size, default_value):
        return _bool_vector._alloc(initial_size, default_value)

    @classmethod
    def _create_active_mask(cls, range_string):
        return cls.createPythonObject(_bool_vector._create_active_mask(range_string))

    def _active_list(self):
        return IntVector.createPythonObject(_bool_vector._active_list(self))

    def _alloc_copy(self):
        return BoolVector.createPythonObject(_bool_vector._alloc_copy(self))

    @staticmethod
    def _update_active_mask(range_string, active_mask):
        return _bool_vector._update_active_mask(range_string, active_mask)

    def _strided_copy(self, start, stop, stride):
        return BoolVector.createPythonObject(
            _bool_vector._strided_copy(self, start, stop, stride)
        )

    def _free(self):
        _bool_vector._free(self)

    def _iget(self, index):
        return _bool_vector._iget(self, index)

    def _safe_iget(self, index):
        return _bool_vector._safe_iget(self, index)

    def _iset(self, index, value):
        _bool_vector._iset(self, index, value)

    def _size(self):
        return _bool_vector._size(self)

    def _append(self, value):
        _bool_vector._append(self, value)

    def _idel_block(self, index, block_size):
        _bool_vector._idel_block(self, index, block_size)

    def _idel(self, index):
        _bool_vector._idel(self, index)

    def _pop(self):
        return _bool_vector._pop(self)

    def _lshift(self, shift):
        _bool_vector._lshift(self, shift)

    def _rshift(self, shift):
        _bool_vector._rshift(self, shift)

    def _insert(self, index, value):
        _bool_vector._insert(self, index, value)

    def _fprintf(self, stream, name, fmt):
        _bool_vector._fprintf(self, stream, name, fmt)

    def _sort(self):
        _bool_vector._sort(self)

    def _rsort(self):
        _bool_vector._rsort(self)

    def _reset(self):
        _bool_vector._reset(self)

    def _set_read_only(self, read_only):
        _bool_vector._set_read_only(self, read_only)

    def _get_read_only(self):
        return _bool_vector._get_read_only(self)

    def _get_max(self):
        return _bool_vector._get_max(self)

    def _get_min(self):
        return _bool_vector._get_min(self)

    def _get_max_index(self, reverse):
        return _bool_vector._get_max_index(self, reverse)

    def _get_min_index(self, reverse):
        return _bool_vector._get_min_index(self, reverse)

    def _shift(self, delta):
        _bool_vector._shift(self, delta)

    def _scale(self, factor):
        _bool_vector._scale(self, factor)

    def _div(self, divisor):
        _bool_vector._div(self, divisor)

    def _inplace_add(self, delta):
        _bool_vector._inplace_add(self, delta)

    def _inplace_mul(self, factor):
        _bool_vector._inplace_mul(self, factor)

    def _assign(self, value):
        _bool_vector._assign(self, value)

    def _memcpy(self, src):
        _bool_vector._memcpy(self, src)

    def _set_default(self, default_value):
        _bool_vector._set_default(self, default_value)

    def _get_default(self):
        return _bool_vector._get_default(self)

    def _element_size(self):
        return _bool_vector._element_size(self)

    def _permute(self, permutation_vector):
        _bool_vector._permute(self, permutation_vector)

    def _sort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_bool_vector._sort_perm(self))

    def _rsort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_bool_vector._rsort_perm(self))

    def _contains(self, value):
        return _bool_vector._contains(self, value)

    def _select_unique(self):
        _bool_vector._select_unique(self)

    def _element_sum(self):
        return _bool_vector._element_sum(self)

    def _get_data_ptr(self):
        return _bool_vector._get_data_ptr(self)

    def _count_equal(self, value):
        return _bool_vector._count_equal(self, value)

    def _equal(self, other):
        return _bool_vector._equal(self, other)

    def _first_eq(self, other, offset):
        return _bool_vector._first_eq(self, other, offset)

    def _first_neq(self, other, offset):
        return _bool_vector._first_neq(self, other, offset)

    def __init__(self, default_value=False, initial_size=0):
        super().__init__(default_value, initial_size)

    def count(self, value: bool = True) -> int:
        return self._count_equal(value)

    @classmethod
    def createActiveMask(cls, range_string) -> BoolVector:
        """
        Will create a BoolVector instance with the values from @range_string.

        The range_string input should be of the type "1,3-5,9,17",
        i.e. integer values separated by commas, and dashes to
        represent ranges. If the input string contains ANY invalid
        characters the returned active list will be empty:

           "1,4-7,10"  =>  {F,T,F,F,T,T,T,T,F,F,T}
           "1,4-7,10X" =>  {}

        The empty list will evaluate to false
        """
        return cls._create_active_mask(range_string)

    def updateActiveMask(self, range_string: str) -> bool:
        """
        Updates a bool vector based on a range string.
        """
        return self._update_active_mask(range_string, self)

    @classmethod
    def createFromList(cls, size, source_list) -> BoolVector:
        """
        Allocates a bool vector from a Python list of indexes
        """
        bool_vector = BoolVector(False, size)

        for index in source_list:
            index = int(index)
            bool_vector[index] = True

        return bool_vector

    def createActiveList(self) -> IntVector:
        return self._active_list()

    def _tostr(self, arr=None):
        if arr is None:
            arr = self
        return "".join(["1" if x else "0" for x in arr])

    def __repr__(self):
        """Will return BoolVector(size = 4, content = "0010") at 0x1729 and
        if size > 10, will return content = "0001...100", i.e., |content|<=10.
        """
        cnt = ""
        ls = len(self)
        if ls <= 20:
            cnt = self._tostr()
        else:
            a, b = self[:9], self[-8:]
            cnt = self._tostr(a)
            cnt += "..."
            cnt += self._tostr(b)
        return 'BoolVector(size = %d, content = "%s") %s' % (ls, cnt, self._ad_str())

    @classmethod
    def create_linear(cls, start_value, end_value, num_values):
        raise NotImplementedError(
            "The init_linear method does not make sense for bool vectors"
        )
