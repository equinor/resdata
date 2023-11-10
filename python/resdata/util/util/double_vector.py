from resdata import ResdataPrototype
from resdata.util.util import VectorTemplate


class DoubleVector(VectorTemplate):
    TYPE_NAME = "rd_double_vector"
    default_format = "%8.4f"

    _alloc = ResdataPrototype("void* double_vector_alloc(int, double)", bind=False)
    _alloc_copy = ResdataPrototype(
        "rd_double_vector_obj double_vector_alloc_copy(rd_double_vector)"
    )
    _strided_copy = ResdataPrototype(
        "rd_double_vector_obj double_vector_alloc_strided_copy(rd_double_vector, int, int, int)"
    )
    _free = ResdataPrototype("void double_vector_free(rd_double_vector)")
    _iget = ResdataPrototype("double double_vector_iget(rd_double_vector, int)")
    _safe_iget = ResdataPrototype(
        "double double_vector_safe_iget(rd_double_vector, int)"
    )
    _iset = ResdataPrototype("double double_vector_iset(rd_double_vector, int, double)")
    _size = ResdataPrototype("int double_vector_size(rd_double_vector)")
    _append = ResdataPrototype("void double_vector_append(rd_double_vector, double )")
    _idel_block = ResdataPrototype(
        "void double_vector_idel_block(rd_double_vector, int, int)"
    )
    _pop = ResdataPrototype("double double_vector_pop(rd_double_vector)")
    _idel = ResdataPrototype("void double_vector_idel(rd_double_vector, int)")
    _lshift = ResdataPrototype("void double_vector_lshift(rd_double_vector, int)")
    _rshift = ResdataPrototype("void double_vector_rshift(rd_double_vector, int)")
    _insert = ResdataPrototype(
        "void double_vector_insert(rd_double_vector, int, double)"
    )
    _fprintf = ResdataPrototype(
        "void double_vector_fprintf(rd_double_vector, FILE, char*, char*)"
    )
    _sort = ResdataPrototype("void double_vector_sort(rd_double_vector)")
    _rsort = ResdataPrototype("void double_vector_rsort(rd_double_vector)")
    _reset = ResdataPrototype("void double_vector_reset(rd_double_vector)")
    _get_read_only = ResdataPrototype(
        "bool double_vector_get_read_only(rd_double_vector)"
    )
    _set_read_only = ResdataPrototype(
        "void double_vector_set_read_only(rd_double_vector, bool)"
    )
    _get_max = ResdataPrototype("double double_vector_get_max(rd_double_vector)")
    _get_min = ResdataPrototype("double double_vector_get_min(rd_double_vector)")
    _get_max_index = ResdataPrototype(
        "int double_vector_get_max_index(rd_double_vector, bool)"
    )
    _get_min_index = ResdataPrototype(
        "int double_vector_get_min_index(rd_double_vector, bool)"
    )
    _shift = ResdataPrototype("void double_vector_shift(rd_double_vector, double)")
    _scale = ResdataPrototype("void double_vector_scale(rd_double_vector, double)")
    _div = ResdataPrototype("void double_vector_div(rd_double_vector, double)")
    _inplace_add = ResdataPrototype(
        "void double_vector_inplace_add(rd_double_vector, rd_double_vector)"
    )
    _inplace_mul = ResdataPrototype(
        "void double_vector_inplace_mul(rd_double_vector, rd_double_vector)"
    )
    _assign = ResdataPrototype("void double_vector_set_all(rd_double_vector, double)")
    _memcpy = ResdataPrototype(
        "void double_vector_memcpy(rd_double_vector, rd_double_vector)"
    )
    _set_default = ResdataPrototype(
        "void double_vector_set_default(rd_double_vector, double)"
    )
    _get_default = ResdataPrototype(
        "double double_vector_get_default(rd_double_vector)"
    )
    _element_size = ResdataPrototype("int double_vector_element_size(rd_double_vector)")

    _permute = ResdataPrototype(
        "void double_vector_permute(rd_double_vector, rd_permutation_vector)"
    )
    _sort_perm = ResdataPrototype(
        "rd_permutation_vector_obj double_vector_alloc_sort_perm(rd_double_vector)"
    )
    _rsort_perm = ResdataPrototype(
        "rd_permutation_vector_obj double_vector_alloc_rsort_perm(rd_double_vector)"
    )
    _contains = ResdataPrototype(
        "bool double_vector_contains(rd_double_vector, double)"
    )
    _select_unique = ResdataPrototype(
        "void double_vector_select_unique(rd_double_vector)"
    )
    _element_sum = ResdataPrototype("double double_vector_sum(rd_double_vector)")
    _get_data_ptr = ResdataPrototype("double* double_vector_get_ptr(rd_double_vector)")
    _count_equal = ResdataPrototype(
        "int double_vector_count_equal(rd_double_vector, double)"
    )
    _init_range = ResdataPrototype(
        "void double_vector_init_range(rd_double_vector, double, double, double)"
    )
    _init_linear = ResdataPrototype(
        "bool double_vector_init_linear(rd_double_vector, double, double, int)"
    )
    _equal = ResdataPrototype(
        "bool double_vector_equal(rd_double_vector, rd_double_vector)"
    )
    _first_eq = ResdataPrototype(
        "int double_vector_first_equal(rd_double_vector, rd_double_vector, int)"
    )
    _first_neq = ResdataPrototype(
        "int double_vector_first_not_equal(rd_double_vector, rd_double_vector, int)"
    )

    def __init__(self, default_value=0, initial_size=0):
        super(DoubleVector, self).__init__(default_value, initial_size)
