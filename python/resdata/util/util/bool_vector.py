from resdata import ResdataPrototype
from resdata.util.util import VectorTemplate


class BoolVector(VectorTemplate):
    TYPE_NAME = "rd_bool_vector"
    default_format = "%8d"

    _alloc = ResdataPrototype("void* bool_vector_alloc(int, bool)", bind=False)
    _create_active_mask = ResdataPrototype(
        "rd_bool_vector_obj string_util_alloc_active_mask(char*)", bind=False
    )
    _active_list = ResdataPrototype(
        "rd_int_vector_obj bool_vector_alloc_active_list(rd_bool_vector)"
    )
    _alloc_copy = ResdataPrototype(
        "rd_bool_vector_obj bool_vector_alloc_copy(rd_bool_vector)"
    )
    _update_active_mask = ResdataPrototype(
        "bool string_util_update_active_mask(char*, rd_bool_vector)", bind=False
    )

    _strided_copy = ResdataPrototype(
        "rd_bool_vector_obj bool_vector_alloc_strided_copy(rd_bool_vector, int, int, int)"
    )
    _free = ResdataPrototype("void bool_vector_free(rd_bool_vector)")
    _iget = ResdataPrototype("bool bool_vector_iget(rd_bool_vector, int)")
    _safe_iget = ResdataPrototype("bool bool_vector_safe_iget(rd_bool_vector, int)")
    _iset = ResdataPrototype("void bool_vector_iset(rd_bool_vector, int, bool)")
    _size = ResdataPrototype("int bool_vector_size(rd_bool_vector)")
    _append = ResdataPrototype("void bool_vector_append(rd_bool_vector, bool)")
    _idel_block = ResdataPrototype(
        "void bool_vector_idel_block(rd_bool_vector, bool, bool)"
    )
    _idel = ResdataPrototype("void bool_vector_idel(rd_bool_vector, int)")
    _pop = ResdataPrototype("bool bool_vector_pop(rd_bool_vector)")
    _lshift = ResdataPrototype("void bool_vector_lshift(rd_bool_vector, int)")
    _rshift = ResdataPrototype("void bool_vector_rshift(rd_bool_vector, int)")
    _insert = ResdataPrototype("void bool_vector_insert(rd_bool_vector, int, bool)")
    _fprintf = ResdataPrototype(
        "void bool_vector_fprintf(rd_bool_vector, FILE, char*, char*)"
    )
    _sort = ResdataPrototype("void bool_vector_sort(rd_bool_vector)")
    _rsort = ResdataPrototype("void bool_vector_rsort(rd_bool_vector)")
    _reset = ResdataPrototype("void bool_vector_reset(rd_bool_vector)")
    _set_read_only = ResdataPrototype(
        "void bool_vector_set_read_only(rd_bool_vector, bool)"
    )
    _get_read_only = ResdataPrototype("bool bool_vector_get_read_only(rd_bool_vector)")
    _get_max = ResdataPrototype("bool bool_vector_get_max(rd_bool_vector)")
    _get_min = ResdataPrototype("bool bool_vector_get_min(rd_bool_vector)")
    _get_max_index = ResdataPrototype(
        "int bool_vector_get_max_index(rd_bool_vector, bool)"
    )
    _get_min_index = ResdataPrototype(
        "int bool_vector_get_min_index(rd_bool_vector, bool)"
    )
    _shift = ResdataPrototype("void bool_vector_shift(rd_bool_vector, bool)")
    _scale = ResdataPrototype("void bool_vector_scale(rd_bool_vector, bool)")
    _div = ResdataPrototype("void bool_vector_div(rd_bool_vector, bool)")
    _inplace_add = ResdataPrototype(
        "void bool_vector_inplace_add(rd_bool_vector, rd_bool_vector)"
    )
    _inplace_mul = ResdataPrototype(
        "void bool_vector_inplace_mul(rd_bool_vector, rd_bool_vector)"
    )
    _assign = ResdataPrototype("void bool_vector_set_all(rd_bool_vector, bool)")
    _memcpy = ResdataPrototype(
        "void bool_vector_memcpy(rd_bool_vector, rd_bool_vector)"
    )
    _set_default = ResdataPrototype(
        "void bool_vector_set_default(rd_bool_vector, bool)"
    )
    _get_default = ResdataPrototype("bool bool_vector_get_default(rd_bool_vector)")
    _element_size = ResdataPrototype("int bool_vector_element_size(rd_bool_vector)")

    _permute = ResdataPrototype(
        "void bool_vector_permute(rd_bool_vector, rd_permutation_vector)"
    )
    _sort_perm = ResdataPrototype(
        "rd_permutation_vector_obj bool_vector_alloc_sort_perm(rd_bool_vector)"
    )
    _rsort_perm = ResdataPrototype(
        "rd_permutation_vector_obj bool_vector_alloc_rsort_perm(rd_bool_vector)"
    )

    _contains = ResdataPrototype("bool bool_vector_contains(rd_bool_vector, bool)")
    _select_unique = ResdataPrototype("void bool_vector_select_unique(rd_bool_vector)")
    _element_sum = ResdataPrototype("bool bool_vector_sum(rd_bool_vector)")
    _get_data_ptr = ResdataPrototype("bool* bool_vector_get_ptr(rd_bool_vector)")
    _count_equal = ResdataPrototype("int bool_vector_count_equal(rd_bool_vector, bool)")
    _init_linear = None
    _equal = ResdataPrototype("bool bool_vector_equal(rd_bool_vector, rd_bool_vector)")
    _first_eq = ResdataPrototype(
        "int bool_vector_first_equal(rd_bool_vector, rd_bool_vector, int)"
    )
    _first_neq = ResdataPrototype(
        "int bool_vector_first_not_equal(rd_bool_vector, rd_bool_vector, int)"
    )

    def __init__(self, default_value=False, initial_size=0):
        super(BoolVector, self).__init__(default_value, initial_size)

    def count(self, value=True):
        """@rtype: int"""
        return self._count_equal(value)

    @classmethod
    def createActiveMask(cls, range_string):
        """
        Will create a BoolVector instance with the values from @range_string.

        The range_string input should be of the type "1,3-5,9,17",
        i.e. integer values separated by commas, and dashes to
        represent ranges. If the input string contains ANY invalid
        characters the returned active list will be empty:

           "1,4-7,10"  =>  {F,T,F,F,T,T,T,T,F,F,T}
           "1,4-7,10X" =>  {}

        The empty list will evaluate to false
        @rtype: BoolVector
        """
        return cls._create_active_mask(range_string)

    def updateActiveMask(self, range_string):
        """
        Updates a bool vector based on a range string.
        @type range_string: str
        @type bool_vector: BoolVector
        @rtype: bool
        """
        return self._update_active_mask(range_string, self)

    @classmethod
    def createFromList(cls, size, source_list):
        """
        Allocates a bool vector from a Python list of indexes
        @rtype: BoolVector
        """
        bool_vector = BoolVector(False, size)

        for index in source_list:
            index = int(index)
            bool_vector[index] = True

        return bool_vector

    def createActiveList(self):
        """@rtype: resdata.util.IntVector"""
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
