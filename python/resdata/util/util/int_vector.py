import resdata.util.util._int_vector as _int_vector

from .vector_template import VectorTemplate


class IntVector(VectorTemplate):
    TYPE_NAME = "rd_int_vector"
    default_format = "%d"

    @staticmethod
    def _alloc(initial_size, default_value):
        return _int_vector._alloc(initial_size, default_value)

    @classmethod
    def _create_active_list(cls, range_string):
        return cls.createPythonObject(_int_vector._create_active_list(range_string))

    @classmethod
    def _create_value_list(cls, range_string):
        return cls.createPythonObject(_int_vector._create_value_list(range_string))

    def _alloc_copy(self):
        return IntVector.createPythonObject(_int_vector._alloc_copy(self))

    def _strided_copy(self, start, stop, stride):
        return IntVector.createPythonObject(
            _int_vector._strided_copy(self, start, stop, stride)
        )

    def _free(self):
        _int_vector._free(self)

    def _iget(self, index):
        return _int_vector._iget(self, index)

    def _safe_iget(self, index):
        return _int_vector._safe_iget(self, index)

    def _iset(self, index, value):
        _int_vector._iset(self, index, value)

    def _size(self):
        return _int_vector._size(self)

    def _append(self, value):
        _int_vector._append(self, value)

    def _idel_block(self, index, block_size):
        _int_vector._idel_block(self, index, block_size)

    def _pop(self):
        return _int_vector._pop(self)

    def _idel(self, index):
        _int_vector._idel(self, index)

    def _insert(self, index, value):
        _int_vector._insert(self, index, value)

    def _lshift(self, shift):
        _int_vector._lshift(self, shift)

    def _rshift(self, shift):
        _int_vector._rshift(self, shift)

    def _fprintf(self, stream, name, fmt):
        _int_vector._fprintf(self, stream, name, fmt)

    def _sort(self):
        _int_vector._sort(self)

    def _rsort(self):
        _int_vector._rsort(self)

    def _reset(self):
        _int_vector._reset(self)

    def _set_read_only(self, read_only):
        _int_vector._set_read_only(self, read_only)

    def _get_read_only(self):
        return _int_vector._get_read_only(self)

    def _get_max(self):
        return _int_vector._get_max(self)

    def _get_min(self):
        return _int_vector._get_min(self)

    def _get_max_index(self, reverse):
        return _int_vector._get_max_index(self, reverse)

    def _get_min_index(self, reverse):
        return _int_vector._get_min_index(self, reverse)

    def _shift(self, delta):
        _int_vector._shift(self, delta)

    def _scale(self, factor):
        _int_vector._scale(self, factor)

    def _div(self, divisor):
        _int_vector._div(self, divisor)

    def _inplace_add(self, delta):
        _int_vector._inplace_add(self, delta)

    def _inplace_mul(self, factor):
        _int_vector._inplace_mul(self, factor)

    def _assign(self, value):
        _int_vector._assign(self, value)

    def _memcpy(self, src):
        _int_vector._memcpy(self, src)

    def _set_default(self, default_value):
        _int_vector._set_default(self, default_value)

    def _get_default(self):
        return _int_vector._get_default(self)

    def _element_size(self):
        return _int_vector._element_size(self)

    def _permute(self, permutation_vector):
        _int_vector._permute(self, permutation_vector)

    def _sort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_int_vector._sort_perm(self))

    def _rsort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_int_vector._rsort_perm(self))

    def _contains(self, value):
        return _int_vector._contains(self, value)

    def _select_unique(self):
        _int_vector._select_unique(self)

    def _element_sum(self):
        return _int_vector._element_sum(self)

    def _get_data_ptr(self):
        return _int_vector._get_data_ptr(self)

    def _count_equal(self, value):
        return _int_vector._count_equal(self, value)

    def _init_range(self, min_value, max_value, delta):
        _int_vector._init_range(self, min_value, max_value, delta)

    def _init_linear(self, start_value, end_value, num_values):
        return _int_vector._init_linear(self, start_value, end_value, num_values)

    def _equal(self, other):
        return _int_vector._equal(self, other)

    def _first_eq(self, other, offset):
        return _int_vector._first_eq(self, other, offset)

    def _first_neq(self, other, offset):
        return _int_vector._first_neq(self, other, offset)

    def __init__(self, default_value=0, initial_size=0):
        super().__init__(default_value, initial_size)

    @classmethod
    def active_list(cls, range_string):
        """Will create a IntVector instance with the values from @range_string.

        The range_string input should be of the type "1,3-5,9,17",
        i.e. integer values separated by commas, and dashes to
        represent ranges. If the input string contains ANY invalid
        characters the returned active list will be empty:

           "1,4-7,10"  =>  {1,4,5,6,7,10}
           "1,4-7,10X" =>  {}

        The empty list will evaluate to false. The values in the input
        string are meant to indicate "active values", i.e. the output
        values are sorted and repeated values are only counted once:

           "1,1,7,2" => {1,2,7}

        """
        return cls._create_active_list(range_string)

    @classmethod
    def valueList(cls, range_string):
        """Will create a IntVecter of all the values in the @range_string.

        Will not sort the values, and not uniquiefy - in contrast to
        the active_list() method.

        """
        return cls._create_value_list(range_string)

    def count(self, value: int) -> int:
        return self._count_equal(value)
