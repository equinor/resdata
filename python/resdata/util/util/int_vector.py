from resdata import ResdataPrototype
from resdata.util.util import VectorTemplate


class IntVector(VectorTemplate):
    TYPE_NAME = "rd_int_vector"
    default_format = "%d"

    _alloc = ResdataPrototype("void*  int_vector_alloc(int, int)", bind=False)
    _create_active_list = ResdataPrototype(
        "rd_int_vector_obj string_util_alloc_active_list( char*)", bind=False
    )
    _create_value_list = ResdataPrototype(
        "rd_int_vector_obj string_util_alloc_value_list( char*)", bind=False
    )
    _alloc_copy = ResdataPrototype(
        "rd_int_vector_obj int_vector_alloc_copy(rd_int_vector)"
    )
    _strided_copy = ResdataPrototype(
        "rd_int_vector_obj int_vector_alloc_strided_copy(rd_int_vector, int, int, int)"
    )
    _free = ResdataPrototype("void int_vector_free(rd_int_vector)")
    _iget = ResdataPrototype("int int_vector_iget(rd_int_vector, int)")
    _safe_iget = ResdataPrototype("int int_vector_safe_iget(rd_int_vector, int)")
    _iset = ResdataPrototype("int int_vector_iset(rd_int_vector, int, int)")
    _size = ResdataPrototype("int int_vector_size(rd_int_vector)")
    _append = ResdataPrototype("void int_vector_append(rd_int_vector, int)")
    _idel_block = ResdataPrototype(
        "void int_vector_idel_block(rd_int_vector, int, int)"
    )
    _pop = ResdataPrototype("int int_vector_pop(rd_int_vector)")
    _idel = ResdataPrototype("void int_vector_idel(rd_int_vector, int)")
    _insert = ResdataPrototype("void int_vector_insert(rd_int_vector, int, int)")
    _lshift = ResdataPrototype("void int_vector_lshift(rd_int_vector, int)")
    _rshift = ResdataPrototype("void int_vector_rshift(rd_int_vector, int)")
    _fprintf = ResdataPrototype(
        "void int_vector_fprintf(rd_int_vector, FILE, char*, char*)"
    )
    _sort = ResdataPrototype("void int_vector_sort(rd_int_vector)")
    _rsort = ResdataPrototype("void int_vector_rsort(rd_int_vector)")
    _reset = ResdataPrototype("void int_vector_reset(rd_int_vector)")
    _set_read_only = ResdataPrototype(
        "void int_vector_set_read_only(rd_int_vector, bool )"
    )
    _get_read_only = ResdataPrototype("bool int_vector_get_read_only(rd_int_vector)")
    _get_max = ResdataPrototype("int int_vector_get_max(rd_int_vector)")
    _get_min = ResdataPrototype("int int_vector_get_min(rd_int_vector)")
    _get_max_index = ResdataPrototype(
        "int int_vector_get_max_index(rd_int_vector, bool)"
    )
    _get_min_index = ResdataPrototype(
        "int int_vector_get_min_index(rd_int_vector, bool)"
    )
    _shift = ResdataPrototype("void int_vector_shift(rd_int_vector, int)")
    _scale = ResdataPrototype("void int_vector_scale(rd_int_vector, int)")
    _div = ResdataPrototype("void int_vector_div(rd_int_vector, int)")
    _inplace_add = ResdataPrototype(
        "void int_vector_inplace_add(rd_int_vector, rd_int_vector)"
    )
    _inplace_mul = ResdataPrototype(
        "void int_vector_inplace_mul(rd_int_vector, rd_int_vector)"
    )
    _assign = ResdataPrototype("void int_vector_set_all(rd_int_vector, int)")
    _memcpy = ResdataPrototype("void int_vector_memcpy(rd_int_vector, rd_int_vector)")
    _set_default = ResdataPrototype("void int_vector_set_default(rd_int_vector, int)")
    _get_default = ResdataPrototype("int int_vector_get_default(rd_int_vector)")
    _element_size = ResdataPrototype("int int_vector_element_size(rd_int_vector)")

    _permute = ResdataPrototype(
        "void int_vector_permute(rd_int_vector, rd_permutation_vector)"
    )
    _sort_perm = ResdataPrototype(
        "rd_permutation_vector_obj int_vector_alloc_sort_perm(rd_int_vector)"
    )
    _rsort_perm = ResdataPrototype(
        "rd_permutation_vector_obj int_vector_alloc_rsort_perm(rd_int_vector)"
    )
    _contains = ResdataPrototype("bool int_vector_contains(rd_int_vector, int)")
    _select_unique = ResdataPrototype("void int_vector_select_unique(rd_int_vector)")
    _element_sum = ResdataPrototype("int int_vector_sum(rd_int_vector)")
    _get_data_ptr = ResdataPrototype("int* int_vector_get_ptr(rd_int_vector)")
    _count_equal = ResdataPrototype("int int_vector_count_equal(rd_int_vector, int)")
    _init_range = ResdataPrototype(
        "void int_vector_init_range(rd_int_vector, int, int, int)"
    )
    _init_linear = ResdataPrototype(
        "bool int_vector_init_linear(rd_int_vector, int, int, int)"
    )
    _equal = ResdataPrototype("bool int_vector_equal(rd_int_vector, rd_int_vector)")
    _first_eq = ResdataPrototype(
        "int int_vector_first_equal(rd_int_vector, rd_int_vector, int)"
    )
    _first_neq = ResdataPrototype(
        "int int_vector_first_not_equal(rd_int_vector, rd_int_vector, int)"
    )

    def __init__(self, default_value=0, initial_size=0):
        super(IntVector, self).__init__(default_value, initial_size)

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

    def count(self, value):
        """@rtype: int"""
        return self._count_equal(value)
