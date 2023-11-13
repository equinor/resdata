from resdata import ResdataPrototype

from .vector_template import VectorTemplate


class DoubleVector(VectorTemplate):
    default_format = "%8.4f"

    _alloc = ResdataPrototype("void*  double_vector_alloc( int , double )", bind=False)
    _alloc_copy = ResdataPrototype(
        "double_vector_obj   double_vector_alloc_copy( double_vector )"
    )
    _strided_copy = ResdataPrototype(
        "double_vector_obj   double_vector_alloc_strided_copy( double_vector , int , int , int)"
    )
    _free = ResdataPrototype("void   double_vector_free( double_vector )")
    _iget = ResdataPrototype("double double_vector_iget( double_vector , int )")
    _safe_iget = ResdataPrototype(
        "double double_vector_safe_iget(double_vector , int )"
    )
    _iset = ResdataPrototype("double double_vector_iset( double_vector , int , double)")
    _size = ResdataPrototype("int    double_vector_size( double_vector )")
    _append = ResdataPrototype("void   double_vector_append( double_vector , double )")
    _idel_block = ResdataPrototype(
        "void   double_vector_idel_block( double_vector , int , int )"
    )
    _pop = ResdataPrototype("double double_vector_pop( double_vector )")
    _idel = ResdataPrototype("void   double_vector_idel( double_vector , int )")
    _lshift = ResdataPrototype("void   double_vector_lshift( double_vector , int )")
    _rshift = ResdataPrototype("void   double_vector_rshift( double_vector , int )")
    _insert = ResdataPrototype(
        "void   double_vector_insert( double_vector , int , double)"
    )
    _fprintf = ResdataPrototype(
        "void   double_vector_fprintf( double_vector , FILE , char* , char*)"
    )
    _sort = ResdataPrototype("void   double_vector_sort( double_vector )")
    _rsort = ResdataPrototype("void   double_vector_rsort( double_vector )")
    _reset = ResdataPrototype("void   double_vector_reset( double_vector )")
    _get_read_only = ResdataPrototype(
        "bool   double_vector_get_read_only( double_vector )"
    )
    _set_read_only = ResdataPrototype(
        "void   double_vector_set_read_only( double_vector , bool )"
    )
    _get_max = ResdataPrototype("double double_vector_get_max( double_vector )")
    _get_min = ResdataPrototype("double double_vector_get_min( double_vector )")
    _get_max_index = ResdataPrototype(
        "int    double_vector_get_max_index( double_vector , bool)"
    )
    _get_min_index = ResdataPrototype(
        "int    double_vector_get_min_index( double_vector , bool)"
    )
    _shift = ResdataPrototype("void   double_vector_shift( double_vector , double )")
    _scale = ResdataPrototype("void   double_vector_scale( double_vector , double )")
    _div = ResdataPrototype("void   double_vector_div( double_vector , double )")
    _inplace_add = ResdataPrototype(
        "void   double_vector_inplace_add( double_vector , double_vector )"
    )
    _inplace_mul = ResdataPrototype(
        "void   double_vector_inplace_mul( double_vector , double_vector )"
    )
    _assign = ResdataPrototype("void   double_vector_set_all( double_vector , double)")
    _memcpy = ResdataPrototype(
        "void   double_vector_memcpy(double_vector , double_vector )"
    )
    _set_default = ResdataPrototype(
        "void   double_vector_set_default( double_vector , double)"
    )
    _get_default = ResdataPrototype("double double_vector_get_default( double_vector )")
    _element_size = ResdataPrototype(
        "int    double_vector_element_size( double_vector )"
    )

    _permute = ResdataPrototype(
        "void double_vector_permute(double_vector, permutation_vector)"
    )
    _sort_perm = ResdataPrototype(
        "permutation_vector_obj double_vector_alloc_sort_perm(double_vector)"
    )
    _rsort_perm = ResdataPrototype(
        "permutation_vector_obj double_vector_alloc_rsort_perm(double_vector)"
    )
    _contains = ResdataPrototype("bool double_vector_contains(double_vector, double)")
    _select_unique = ResdataPrototype("void double_vector_select_unique(double_vector)")
    _element_sum = ResdataPrototype("double double_vector_sum(double_vector)")
    _get_data_ptr = ResdataPrototype("double* double_vector_get_ptr(double_vector)")
    _count_equal = ResdataPrototype(
        "int double_vector_count_equal(double_vector, double)"
    )
    _init_range = ResdataPrototype(
        "void double_vector_init_range(double_vector, double , double , double)"
    )
    _init_linear = ResdataPrototype(
        "bool double_vector_init_linear(double_vector, double, double, int)"
    )
    _equal = ResdataPrototype("bool double_vector_equal(double_vector, double_vector)")
    _first_eq = ResdataPrototype(
        "int double_vector_first_equal(double_vector, double_vector, int)"
    )
    _first_neq = ResdataPrototype(
        "int double_vector_first_not_equal(double_vector, double_vector, int)"
    )

    def __init__(self, default_value=0, initial_size=0):
        super(DoubleVector, self).__init__(default_value, initial_size)
