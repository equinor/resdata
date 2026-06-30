import resdata.util.util._double_vector as _double_vector

from .vector_template import VectorTemplate


class DoubleVector(VectorTemplate):
    TYPE_NAME = "rd_double_vector"
    default_format = "%8.4f"

    @staticmethod
    def _alloc(initial_size, default_value):
        return _double_vector._alloc(initial_size, default_value)

    def _alloc_copy(self):
        return DoubleVector.createPythonObject(_double_vector._alloc_copy(self))

    def _strided_copy(self, start, stop, stride):
        return DoubleVector.createPythonObject(
            _double_vector._strided_copy(self, start, stop, stride)
        )

    def _free(self):
        _double_vector._free(self)

    def _iget(self, index):
        return _double_vector._iget(self, index)

    def _safe_iget(self, index):
        return _double_vector._safe_iget(self, index)

    def _iset(self, index, value):
        _double_vector._iset(self, index, value)

    def _size(self):
        return _double_vector._size(self)

    def _append(self, value):
        _double_vector._append(self, value)

    def _idel_block(self, index, block_size):
        _double_vector._idel_block(self, index, block_size)

    def _idel(self, index):
        _double_vector._idel(self, index)

    def _pop(self):
        return _double_vector._pop(self)

    def _lshift(self, shift):
        _double_vector._lshift(self, shift)

    def _rshift(self, shift):
        _double_vector._rshift(self, shift)

    def _insert(self, index, value):
        _double_vector._insert(self, index, value)

    def _fprintf(self, stream, name, fmt):
        _double_vector._fprintf(self, stream, name, fmt)

    def _sort(self):
        _double_vector._sort(self)

    def _rsort(self):
        _double_vector._rsort(self)

    def _reset(self):
        _double_vector._reset(self)

    def _set_read_only(self, read_only):
        _double_vector._set_read_only(self, read_only)

    def _get_read_only(self):
        return _double_vector._get_read_only(self)

    def _get_max(self):
        return _double_vector._get_max(self)

    def _get_min(self):
        return _double_vector._get_min(self)

    def _get_max_index(self, reverse):
        return _double_vector._get_max_index(self, reverse)

    def _get_min_index(self, reverse):
        return _double_vector._get_min_index(self, reverse)

    def _shift(self, delta):
        _double_vector._shift(self, delta)

    def _scale(self, factor):
        _double_vector._scale(self, factor)

    def _div(self, divisor):
        _double_vector._div(self, divisor)

    def _inplace_add(self, delta):
        _double_vector._inplace_add(self, delta)

    def _inplace_mul(self, factor):
        _double_vector._inplace_mul(self, factor)

    def _assign(self, value):
        _double_vector._assign(self, value)

    def _memcpy(self, src):
        _double_vector._memcpy(self, src)

    def _set_default(self, default_value):
        _double_vector._set_default(self, default_value)

    def _get_default(self):
        return _double_vector._get_default(self)

    def _element_size(self):
        return _double_vector._element_size(self)

    def _permute(self, permutation_vector):
        _double_vector._permute(self, permutation_vector)

    def _sort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_double_vector._sort_perm(self))

    def _rsort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_double_vector._rsort_perm(self))

    def _contains(self, value):
        return _double_vector._contains(self, value)

    def _select_unique(self):
        _double_vector._select_unique(self)

    def _element_sum(self):
        return _double_vector._element_sum(self)

    def _get_data_ptr(self):
        return _double_vector._get_data_ptr(self)

    def _count_equal(self, value):
        return _double_vector._count_equal(self, value)

    def _init_range(self, min_value, max_value, delta):
        _double_vector._init_range(self, min_value, max_value, delta)

    def _init_linear(self, start_value, end_value, num_values):
        return _double_vector._init_linear(self, start_value, end_value, num_values)

    def _equal(self, other):
        return _double_vector._equal(self, other)

    def _first_eq(self, other, offset):
        return _double_vector._first_eq(self, other, offset)

    def _first_neq(self, other, offset):
        return _double_vector._first_neq(self, other, offset)

    def __init__(self, default_value=0, initial_size=0):
        super().__init__(default_value, initial_size)
