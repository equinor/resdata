"""
Module used to select cells based on many different criteria.

This module implements the class ResdataRegion which can be used to select
cells in a grid matching a criteria. A wide range of different
criteria are supported. Many of the special functions for implementing
mathematical operations are implemented, so that regions can be
combined e.g. with logical &.

When the selection process is complete the region instance can be
queried for the corresponding list of indices.
"""
from functools import wraps
import ctypes

from cwrap import BaseCClass

import resdata
from resdata.util.util import monkey_the_camel
from resdata.util.util import IntVector

from resdata import ResdataPrototype
from resdata.grid.faults import Layer
from resdata import ResDataType
from resdata.resfile import ResdataKW
from resdata.geometry import CPolyline


def select_method(select):
    """
    The select_method method decorator is applied to all the
    select_xxx() methods. The purpose of this decorator is to
    allow the select_xxx() methods to have an optional argument
    @intersect. If the @intersect argument is True the results of
    the current select method will be and'ed with the current
    selection, instead of or'ed which is the default.

    Consider this example:

       region = ResdataRegion( grid , False )
       region.select_islice(0, 5)     # Selects all cells with i[0:5]
       region.select_jslice(0, 5)     # Selects all cells with j[0:5]

    When these two calls have completed selection will contain all
    the cells which are either in i-interval [0:5] or in
    j-interval [0:5]. If we supply the @intersect argument in the
    second call the j selection will only be applied to the cells
    in i[0:5] interval:

       region = ResdataRegion( grid , False )

       # Select all cells with i[0:5]:
       region.select_islice(0, 5)

       # Select all cells with j[0:5] AND i[0:5]:
       region.select_jslice(0, 5, intersect=True)
    """

    @wraps(select)
    def select_wrapper(self, *args, **kwargs):
        intersect = "intersect" in kwargs and kwargs["intersect"]
        if intersect:
            new_region = ResdataRegion(self.grid, False)
            select(new_region, *args)

            self &= new_region
        else:
            select(self, *args)

    return select_wrapper


class ResdataRegion(BaseCClass):
    TYPE_NAME = "rd_region"
    _alloc = ResdataPrototype("void* rd_region_alloc( rd_grid , bool )", bind=False)
    _alloc_copy = ResdataPrototype("rd_region_obj rd_region_alloc_copy( rd_region )")

    _set_kw_int = ResdataPrototype(
        "void rd_region_set_kw_int( rd_region , rd_kw , int, bool) "
    )
    _set_kw_float = ResdataPrototype(
        "void rd_region_set_kw_float( rd_region , rd_kw , float, bool ) "
    )
    _set_kw_double = ResdataPrototype(
        "void rd_region_set_kw_double( rd_region , rd_kw , double , bool) "
    )
    _shift_kw_int = ResdataPrototype(
        "void rd_region_shift_kw_int( rd_region , rd_kw , int, bool) "
    )
    _shift_kw_float = ResdataPrototype(
        "void rd_region_shift_kw_float( rd_region , rd_kw , float, bool ) "
    )
    _shift_kw_double = ResdataPrototype(
        "void rd_region_shift_kw_double( rd_region , rd_kw , double , bool) "
    )
    _scale_kw_int = ResdataPrototype(
        "void rd_region_scale_kw_int( rd_region , rd_kw , int, bool) "
    )
    _scale_kw_float = ResdataPrototype(
        "void rd_region_scale_kw_float( rd_region , rd_kw , float, bool ) "
    )
    _scale_kw_double = ResdataPrototype(
        "void rd_region_scale_kw_double( rd_region , rd_kw , double , bool) "
    )
    _sum_kw_int = ResdataPrototype(
        "int rd_region_sum_kw_int( rd_region , rd_kw , bool) "
    )
    _sum_kw_float = ResdataPrototype(
        "float rd_region_sum_kw_float( rd_region , rd_kw , bool ) "
    )
    _sum_kw_double = ResdataPrototype(
        "double rd_region_sum_kw_double( rd_region , rd_kw , bool) "
    )
    _sum_kw_bool = ResdataPrototype(
        "int rd_region_sum_kw_int( rd_region , rd_kw , bool) "
    )

    _free = ResdataPrototype("void rd_region_free( rd_region )")
    _reset = ResdataPrototype("void rd_region_reset( rd_region )")
    _select_all = ResdataPrototype("void rd_region_select_all( rd_region )")
    _deselect_all = ResdataPrototype("void rd_region_deselect_all( rd_region )")
    _select_equal = ResdataPrototype(
        "void rd_region_select_equal( rd_region , rd_kw , int )"
    )
    _deselect_equal = ResdataPrototype(
        "void rd_region_deselect_equal( rd_region , rd_kw , int)"
    )
    _select_less = ResdataPrototype(
        "void rd_region_select_smaller( rd_region , rd_kw , float )"
    )
    _deselect_less = ResdataPrototype(
        "void rd_region_deselect_smaller( rd_region , rd_kw , float )"
    )
    _select_more = ResdataPrototype(
        "void rd_region_select_larger( rd_region , rd_kw , float )"
    )
    _deselect_more = ResdataPrototype(
        "void rd_region_deselect_larger( rd_region , rd_kw , float )"
    )
    _select_in_interval = ResdataPrototype(
        "void rd_region_select_in_interval( rd_region, rd_kw , float , float )"
    )
    _deselect_in_interval = ResdataPrototype(
        "void rd_region_deselect_in_interval( rd_region, rd_kw, float , float )"
    )
    _invert_selection = ResdataPrototype("void rd_region_invert_selection( rd_region )")

    _select_box = ResdataPrototype(
        "void rd_region_select_from_ijkbox(rd_region , int , int , int , int , int , int)"
    )
    _deselect_box = ResdataPrototype(
        "void rd_region_deselect_from_ijkbox(rd_region , int , int , int , int , int , int)"
    )
    _imul_kw = ResdataPrototype(
        "void  rd_region_kw_imul( rd_region , rd_kw , rd_kw , bool)"
    )
    _idiv_kw = ResdataPrototype(
        "void  rd_region_kw_idiv( rd_region , rd_kw , rd_kw , bool)"
    )
    _iadd_kw = ResdataPrototype(
        "void  rd_region_kw_iadd( rd_region , rd_kw , rd_kw , bool)"
    )
    _isub_kw = ResdataPrototype(
        "void  rd_region_kw_isub( rd_region , rd_kw , rd_kw , bool)"
    )
    _copy_kw = ResdataPrototype(
        "void  rd_region_kw_copy( rd_region , rd_kw , rd_kw , bool)"
    )
    _intersect = ResdataPrototype(
        "void rd_region_intersection( rd_region , rd_region )"
    )
    _combine = ResdataPrototype("void rd_region_union( rd_region , rd_region )")
    _subtract = ResdataPrototype("void rd_region_subtract( rd_region , rd_region )")
    _xor = ResdataPrototype("void rd_region_xor( rd_region , rd_region )")
    _get_kw_index_list = ResdataPrototype(
        "rd_int_vector_ref rd_region_get_kw_index_list( rd_region , rd_kw , bool )"
    )
    _get_active_list = ResdataPrototype(
        "rd_int_vector_ref rd_region_get_active_list( rd_region )"
    )
    _get_global_list = ResdataPrototype(
        "rd_int_vector_ref rd_region_get_global_list( rd_region )"
    )
    _get_active_global = ResdataPrototype(
        "rd_int_vector_ref rd_region_get_global_active_list( rd_region )"
    )
    _select_cmp_less = ResdataPrototype(
        "void rd_region_cmp_select_less( rd_region , rd_kw , rd_kw)"
    )
    _select_cmp_more = ResdataPrototype(
        "void rd_region_cmp_select_more( rd_region , rd_kw , rd_kw)"
    )
    _deselect_cmp_less = ResdataPrototype(
        "void rd_region_cmp_deselect_less( rd_region , rd_kw , rd_kw)"
    )
    _deselect_cmp_more = ResdataPrototype(
        "void rd_region_cmp_deselect_more( rd_region , rd_kw , rd_kw)"
    )
    _select_islice = ResdataPrototype(
        "void rd_region_select_i1i2( rd_region , int , int )"
    )
    _deselect_islice = ResdataPrototype(
        "void rd_region_deselect_i1i2( rd_region , int , int )"
    )
    _select_jslice = ResdataPrototype(
        "void rd_region_select_j1j2( rd_region , int , int )"
    )
    _deselect_jslice = ResdataPrototype(
        "void rd_region_deselect_j1j2( rd_region , int , int )"
    )
    _select_kslice = ResdataPrototype(
        "void rd_region_select_k1k2( rd_region , int , int )"
    )
    _deselect_kslice = ResdataPrototype(
        "void rd_region_deselect_k1k2( rd_region , int , int )"
    )
    _select_deep_cells = ResdataPrototype(
        "void rd_region_select_deep_cells( rd_region , double )"
    )
    _deselect_deep_cells = ResdataPrototype(
        "void rd_region_select_deep_cells( rd_region , double )"
    )
    _select_shallow_cells = ResdataPrototype(
        "void rd_region_select_shallow_cells( rd_region , double )"
    )
    _deselect_shallow_cells = ResdataPrototype(
        "void rd_region_select_shallow_cells( rd_region , double )"
    )
    _select_small = ResdataPrototype(
        "void rd_region_select_small_cells( rd_region , double )"
    )
    _deselect_small = ResdataPrototype(
        "void rd_region_deselect_small_cells( rd_region , double )"
    )
    _select_large = ResdataPrototype(
        "void rd_region_select_large_cells( rd_region , double )"
    )
    _deselect_large = ResdataPrototype(
        "void rd_region_deselect_large_cells( rd_region , double )"
    )
    _select_thin = ResdataPrototype(
        "void rd_region_select_thin_cells( rd_region , double )"
    )
    _deselect_thin = ResdataPrototype(
        "void rd_region_deselect_thin_cells( rd_region , double )"
    )
    _select_thick = ResdataPrototype(
        "void rd_region_select_thick_cells( rd_region , double )"
    )
    _deselect_thick = ResdataPrototype(
        "void rd_region_deselect_thick_cells( rd_region , double )"
    )
    _select_active = ResdataPrototype("void rd_region_select_active_cells( rd_region )")
    _select_inactive = ResdataPrototype(
        "void rd_region_select_inactive_cells( rd_region )"
    )
    _deselect_active = ResdataPrototype(
        "void rd_region_deselect_active_cells( rd_region )"
    )
    _deselect_inactive = ResdataPrototype(
        "void rd_region_deselect_inactive_cells( rd_region )"
    )
    _select_above_plane = ResdataPrototype(
        "void rd_region_select_above_plane( rd_region  , double* , double* )"
    )
    _select_below_plane = ResdataPrototype(
        "void rd_region_select_below_plane( rd_region  , double* , double* )"
    )
    _deselect_above_plane = ResdataPrototype(
        "void rd_region_deselect_above_plane( rd_region, double* , double* )"
    )
    _deselect_below_plane = ResdataPrototype(
        "void rd_region_deselect_below_plane( rd_region, double* , double* )"
    )
    _select_inside_polygon = ResdataPrototype(
        "void rd_region_select_inside_polygon( rd_region , rd_geo_polygon)"
    )
    _select_outside_polygon = ResdataPrototype(
        "void rd_region_select_outside_polygon( rd_region , rd_geo_polygon)"
    )
    _deselect_inside_polygon = ResdataPrototype(
        "void rd_region_deselect_inside_polygon( rd_region , rd_geo_polygon)"
    )
    _deselect_outside_polygon = ResdataPrototype(
        "void rd_region_deselect_outside_polygon( rd_region , rd_geo_polygon)"
    )
    _set_name = ResdataPrototype("void rd_region_set_name( rd_region , char*)")
    _get_name = ResdataPrototype("char* rd_region_get_name( rd_region )")
    _contains_ijk = ResdataPrototype(
        "void rd_region_contains_ijk( rd_region , int , int , int)"
    )
    _contains_global = ResdataPrototype(
        "void rd_region_contains_global( rd_region, int )"
    )
    _contains_active = ResdataPrototype(
        "void rd_region_contains_active( rd_region , int )"
    )
    _equal = ResdataPrototype("bool rd_region_equal( rd_region , rd_region )")
    _select_true = ResdataPrototype("void rd_region_select_true( rd_region , rd_kw)")
    _select_false = ResdataPrototype("void rd_region_select_false( rd_region , rd_kw)")
    _deselect_true = ResdataPrototype(
        "void rd_region_deselect_true( rd_region , rd_kw)"
    )
    _deselect_false = ResdataPrototype(
        "void rd_region_deselect_false( rd_region , rd_kw)"
    )
    _select_from_layer = ResdataPrototype(
        "void rd_region_select_from_layer( rd_region , rd_layer , int , int)"
    )
    _deselect_from_layer = ResdataPrototype(
        "void rd_region_deselect_from_layer( rd_region , rd_layer , int , int)"
    )

    def __init__(self, grid, preselect):
        """
        Create a new region selector for cells in @grid.

        Will create a new region selector to select and deselect the
        cells in the grid given by @grid. The input argument @grid
        should be a Grid instance. You can start with either all
        cells, or no cells, selected, depending on the value of
        @preselect.
        """

        self.grid = grid
        self.active_index = False
        c_ptr = self._alloc(grid, preselect)
        super(ResdataRegion, self).__init__(c_ptr)

    def free(self):
        self._free()

    def __eq__(self, other):
        return self._equal(other)

    def __hash__(self):
        return hash(hash(self.grid) + hash(self.active_index))

    def __deep_copy__(self, memo):
        """
        Creates a deep copy of the current region.
        """
        return self._alloc_copy()

    def __nonzero__(self):
        global_list = self.get_global_list()
        return len(global_list) > 0

    def __bool__(self):
        return self.__nonzero__()

    def __iand__(self, other):
        """
        Will perform set intersection operation inplace.

        Will update the current region selection so that the elements
        selected in self are also selected in @other. Bound to the
        inplace & operator, i.e.

           reg1 &= reg2

        will eventually call this method.
        """
        if isinstance(other, ResdataRegion):
            self._intersect(other)
        else:
            raise TypeError(
                "Resdata region can only intersect with other ResdataRegion instances"
            )

        return self

    def __isub__(self, other):
        """
        Inplace "subtract" one selection from another.

        Bound to reg -= reg2
        """
        if isinstance(other, ResdataRegion):
            self._subtract(other)
        else:
            raise TypeError(
                "Resdata region can only subtract with other ResdataRegion instances"
            )

        return self

    def __ior__(self, other):
        """
        Will perform set operation union in place.

        The current region selection will be updated to contain all
        the elements which are selected either in the current region,
        or in @other; bound to to inplace | operator, so you can write e.g.

            reg1 |= reg2

        to update reg1 with the selections from reg2.
        """
        if isinstance(other, ResdataRegion):
            self._combine(other)
        else:
            raise TypeError(
                "Resdata region can only be combined with other ResdataRegion instances"
            )

        return self

    def __iadd__(self, other):
        """
        Combines to regions - see __ior__().
        """
        return self.__ior__(other)

    def __or__(self, other):
        """
        Creates a new region which is the union of @self and other.

        The method will create a new region which selection status is
        given by the logical or of regions @self and @other; the two
        initial regions will not be modified. Bound to the unary |
        operator:

            new_reg = reg1 | reg2

        """
        new_region = self.copy()
        new_region.__ior__(other)
        return new_region

    def __and__(self, other):
        """
        Creates a new region which is the intersection of @self and other.

        The method will create a new region which selection status is
        given by the logical and of regions @self and @other; the two
        initial regions will not be modified. Bound to the unary &
        operator:

            new_reg = reg1 & reg2
        """
        new_region = self.copy()
        new_region.__iand__(other)
        return new_region

    def __add__(self, other):
        """
        Unary add operator for two regions - implemented by __or__().
        """
        return self.__or__(other)

    def __sub__(self, other):
        """
        Unary del operator for two regions.
        """
        new_region = self.copy()
        new_region.__isub__(other)
        return new_region

    def union_with(self, other):
        """
        Will update self with the union of @self and @other.

        See doscumentation of __ior__().
        """
        return self.__ior__(other)

    def intersect_with(self, other):
        """
        Will update self with the intersection of @self and @other.

        See doscumentation of __iand__().
        """
        return self.__iand__(other)

    def copy(self):
        return self.__deep_copy__({})

    def reset(self):
        """
        Clear selections according to constructor argument @preselect.

        Will clear all selections, depending on the value of the
        constructor argument @preselect. If @preselect is true
        everything will be selected after calling reset(), otherwise
        no cells will be selected after calling reset().
        """
        self._reset()

    ##################################################################

    @select_method
    def select_more(self, rd_kw, limit, intersect=False):
        """
        Select all cells where keyword @rd_kw is above @limit.

        This method is used to select all the cells where an arbitrary
        field, contained in @rd_kw, is above a limiting value
        @limit. The ResdataKW instance must have either nactive or
        nx*ny*nz elements; if this is not satisfied method will fail
        hard. The datatype of @rd_kw must be numeric,
        i.e. RD_INT_TYPE, RD_DOUBLE_TYPE or RD_FLOAT_TYPE. In the
        example below we select all the cells with water saturation
        above 0.85:

           >>> restart_file = ResdataFile( "CASE.X0067" )
           >>> swat_kw = restart_file["SWAT"][0]
           >>> grid = Grid( "CASE.EGRID" )
           >>> region = ResdataRegion( grid , False )

           >>> region.select_more( swat_kw , 0.85 )

        """
        self._select_more(rd_kw, limit)

    def deselect_more(self, rd_kw, limit):
        """
        Deselects cells with value above limit.

        See select_more() for further documentation.
        """
        self._deselect_more(rd_kw, limit)

    @select_method
    def select_less(self, rd_kw, limit, intersect=False):
        """
        Select all cells where keyword @rd_kw is below @limit.

        See select_more() for further documentation.
        """
        self._select_less(rd_kw, limit)

    def deselect_less(self, rd_kw, limit):
        """
        Deselect all cells where keyword @rd_kw is below @limit.

        See select_more() for further documentation.
        """
        self._deselect_less(rd_kw, limit)

    @select_method
    def select_equal(self, rd_kw, value, intersect=False):
        """
        Select all cells where @rd_kw is equal to @value.

        The ResdataKW instance @rd_kw must be of size nactive or
        nx*ny*nz, and it must be of integer type; testing for equality
        is not supported for floating point numbers. In the example
        below we select all the cells in PVT regions 2 and 4:

           >>> init_file = ResdataFile( "CASE.INIT" )
           >>> pvtnum_kw = init_file.iget_named_kw( "PVTNUM" , 0 )
           >>> grid = Grid( "CASE.GRID" )
           >>> region = ResdataRegion( grid , False )

           >>> region.select_equal( pvtnum_kw , 2 )
           >>> region.select_equal( pvtnum_kw , 4 )

        """
        if not rd_kw.data_type.is_int():
            raise ValueError(
                "The select_equal method must have an integer valued keyword - got:%s"
                % rd_kw.typeName()
            )
        self._select_equal(rd_kw, value)

    def deselect_equal(self, rd_kw, value):
        """
        Select all cells where @rd_kw is equal to @value.

        See select_equal() for further documentation.
        """
        if not rd_kw.data_type.is_int():
            raise ValueError(
                "The select_equal method must have an integer valued keyword - got:%s"
                % rd_kw.typeName()
            )
        self._deselect_equal(rd_kw, value)

    @select_method
    def select_in_range(self, rd_kw, lower_limit, upper_limit, select=False):
        """
        Select all cells where @rd_kw is in the half-open interval [ , ).

        Will select all the cells where ResdataKW instance @rd_kw has
        value in the half-open interval [@lower_limit ,
        @upper_limit). The input argument @rd_kw must have size
        nactive or nx*ny*nz, and it must be of type RD_FLOAT_TYPE.

        The following example will select all cells with porosity in
        the range [0.15,0.20):

           >>> init_file = resdata.ResdataFile( "CASE.INIT" )
           >>> poro_kw = init_file.iget_named_kw( "PORO" , 0 )
           >>> grid = resdata.Grid( "CASE.GRID" )
           >>> region = resdata.ResdataRegion( grid , False )

           >>> region.select_in_range( poro_kw , 0.15, 0.20 )

        """
        self._select_in_interval(rd_kw, lower_limit, upper_limit)

    def deselect_in_range(self, rd_kw, lower_limit, upper_limit):
        """
        Deselect all cells where @rd_kw is in the half-open interval [ , ).

        See select_in_range() for further documentation.
        """
        self._deselect_in_interval(rd_kw, lower_limit, upper_limit)

    @select_method
    def select_cmp_less(self, kw1, kw2, intersect=False):
        """
        Will select all cells where kw2 < kw1.

        Will compare the keywords @kw1 and @kw2, and select
        all the cells where the numerical value of @kw1 is less than
        the numerical value of @kw2. The keywords @kw1 and
        @kw2 must both be of the same size, nactive or nx*ny*nz. In
        addition they must both be of type type RD_FLOAT_TYPE. In the
        example below we select all the cells where the pressure has
        dropped:

           >>> restart_file = resdata.ResdataFile("CASE.UNRST")
           >>> pressure1 = restart_file.iget_named_kw( "PRESSURE" , 0)
           >>> pressure2 = restart_file.iget_named_kw( "PRESSURE" , 100)

           >>> region.select_cmp_less( pressure2 , pressure1)

        """
        self._select_cmp_less(kw1, kw2)

    def deselect_cmp_less(self, kw1, kw2):
        """
        Will deselect all cells where kw2 < kw1.

        See select_cmp_less() for further documentation.
        """
        self._deselect_cmp_less(kw1, kw2)

    @select_method
    def select_cmp_more(self, kw1, kw2, intersect=False):
        """
        Will select all cells where kw2 > kw1.

        See select_cmp_less() for further documentation.
        """
        self._select_cmp_more(kw1, kw2)

    def deselect_cmp_more(self, kw1, kw2):
        """
        Will deselect all cells where kw2 > kw1.

        See select_cmp_less() for further documentation.
        """
        self._deselect_cmp_more(kw1, kw2)

    @select_method
    def select_active(self, intersect=False):
        """
        Will select all the active grid cells.
        """
        self._select_active()

    def deselect_active(self):
        """
        Will deselect all the active grid cells.
        """
        self._deselect_active()

    @select_method
    def select_inactive(self, intersect=False):
        """
        Will select all the inactive grid cells.
        """
        self._select_inactive()

    def deselect_inactive(self):
        """
        Will deselect all the inactive grid cells.
        """
        self._deselect_inactive()

    def select_all(self):
        """
        Will select all the cells.
        """
        self._select_all()

    def deselect_all(self):
        """
        Will deselect all the cells.
        """
        self._deselect_all()

    def clear(self):
        """
        Will deselect all cells.
        """
        self.deselect_all()

    @select_method
    def select_deep(self, depth, intersect=False):
        """
        Will select all cells below @depth.
        """
        self._select_deep_cells(depth)

    def deselect_deep(self, depth):
        """
        Will deselect all cells below @depth.
        """
        self._deselect_deep_cells(depth)

    @select_method
    def select_shallow(self, depth, intersect=False):
        """
        Will select all cells above @depth.
        """
        self._select_shallow_cells(depth)

    def deselect_shallow(self, depth):
        """
        Will deselect all cells above @depth.
        """
        self._deselect_shallow_cells(depth)

    @select_method
    def select_small(self, size_limit, intersect=False):
        """
        Will select all cells smaller than @size_limit.
        """
        self._select_small(size_limit)

    def deselect_small(self, size_limit):
        """
        Will deselect all cells smaller than @size_limit.
        """
        self._deselect_small(size_limit)

    @select_method
    def select_large(self, size_limit, intersect=False):
        """
        Will select all cells larger than @size_limit.
        """
        self._select_large(size_limit)

    def deselect_large(self, size_limit):
        """
        Will deselect all cells larger than @size_limit.
        """
        self._deselect_large(size_limit)

    @select_method
    def select_thin(self, size_limit, intersect=False):
        """
        Will select all cells thinner than @size_limit.
        """
        self._select_thin(size_limit)

    def deselect_thin(self, size_limit):
        """
        Will deselect all cells thinner than @size_limit.
        """
        self._deselect_thin(size_limit)

    @select_method
    def select_thick(self, size_limit, intersect=False):
        """
        Will select all cells thicker than @size_limit.
        """
        self._select_thick(size_limit)

    def deselect_thick(self, size_limit):
        """
        Will deselect all cells thicker than @size_limit.
        """
        self._deselect_thick(size_limit)

    @select_method
    def select_box(self, ijk1, ijk2, intersect=False):
        """
        Will select all cells in box.

        Will select all the the cells in the box given by @ijk1 and
        @ijk2. The two arguments @ijk1 and @ijk2 are tuples (1,j,k)
        representing two arbitrary - diagonally opposed corners - of a
        box. All the elements in @ijk1 and @ijk2 are inclusive, i.e.

           select_box( (10,12,8) , (8 , 16,4) )

        will select the box defined by [8,10] x [12,16] x [4,8].
        """
        self._select_box(ijk1[0], ijk2[0], ijk1[1], ijk2[1], ijk1[2], ijk2[2])

    def deselect_box(self, ijk1, ijk2):
        """
        Will deselect all elements in box.

        See select_box() for further documentation.
        """
        self._deselect_box(ijk1[0], ijk2[0], ijk1[1], ijk2[1], ijk1[2], ijk2[2])

    @select_method
    def select_islice(self, i1, i2, intersect=False):
        """
        Will select all cells with i in [@i1, @i2]. @i1 and @i2 are zero offset.
        """
        self._select_islice(i1, i2)

    def deselect_islice(self, i1, i2):
        """
        Will deselect all cells with i in [@i1, @i2]. @i1 and @i2 are zero offset.
        """
        self._deselect_islice(i1, i2)

    @select_method
    def select_jslice(self, j1, j2, intersect=False):
        """
        Will select all cells with j in [@j1, @j2]. @i1 and @i2 are zero offset.
        """
        self._select_jslice(j1, j2)

    def deselect_jslice(self, j1, j2):
        """
        Will deselect all cells with j in [@j1, @j2]. @i1 and @i2 are zero offset.
        """
        self._deselect_jslice(j1, j2)

    @select_method
    def select_kslice(self, k1, k2, intersect=False):
        """
        Will select all cells with k in [@k1, @k2]. @i1 and @i2 are zero offset.
        """
        self._select_kslice(k1, k2)

    def deselect_kslice(self, k1, k2):
        """
        Will deselect all cells with k in [@k1, @k2]. @i1 and @i2 are zero offset.
        """
        self._deselect_kslice(k1, k2)

    def invert(self):
        """
        Will invert the current selection.
        """
        self._invert_selection()

    def __init_plane_select(self, n, p):
        n_vec = ctypes.cast((ctypes.c_double * 3)(), ctypes.POINTER(ctypes.c_double))
        p_vec = ctypes.cast((ctypes.c_double * 3)(), ctypes.POINTER(ctypes.c_double))
        for i in range(3):
            n_vec[i] = n[i]
            p_vec[i] = p[i]
        return (n_vec, p_vec)

    @select_method
    def select_above_plane(self, n, p, intersect=False):
        """
        Will select all the cells 'above' the plane defined by n & p.

        @n is the surface normal vector of the plane in question and
        @p is a point on the plane surface. The point @p should be
        given in (utm_x , utm_y , tvd) coordinates. The term 'above'
        means that the cell center has a positive distance to the
        plain; correspondingly 'below' means that the cell center has
        a negative disatnce to the plane.
        """
        (n_vec, p_vec) = self.__init_plane_select(n, p)
        self._select_above_plane(n_vec, p_vec)

    @select_method
    def select_below_plane(self, n, p, interscet=False):
        """
        Will select all the cells 'below' the plane defined by n & p.

        See method 'select_above_plane' for further documentation.
        """
        (n_vec, p_vec) = self.__init_plane_select(n, p)
        self._select_below_plane(n_vec, p_vec)

    def deselect_above_plane(self, n, p):
        """
        Will deselect all the cells 'above' the plane defined by n & p.

        See method 'select_above_plane' for further documentation.
        """
        (n_vec, p_vec) = self.__init_plane_select(n, p)
        self._deselect_above_plane(n_vec, p_vec)

    def deselect_below_plane(self, n, p):
        """
        Will deselect all the cells 'below' the plane defined by n & p.

        See method 'select_above_plane' for further documentation.
        """
        (n_vec, p_vec) = self.__init_plane_select(n, p)
        self._deselect_below_plane(n_vec, p_vec)

    @select_method
    def select_inside_polygon(self, points, intersect=False):
        """
        Will select all points inside polygon.

        Will select all points inside polygon specified by input
        variable @points. Points should be a list of two-element
        tuples (x,y). So to select all the points within the rectangle
        bounded by the lower left rectangle (0,0) and upper right
        (100,100) the @points list should be:

           points = [(0,0) , (0,100) , (100,100) ,  (100,0)]

        The elements in the points list should be (utm_x, utm_y)
        values. These values will be compared with the centerpoints of
        the cells in the grid. The selection is based the top k=0
        layer, and then extending this selection to all k values; this
        implies that the selection polygon will effectively be
        translated if the pillars are not vertical.
        """
        self._select_inside_polygon(CPolyline(init_points=points))

    @select_method
    def select_outside_polygon(self, points, intersect=False):
        """
        Will select all points outside polygon.

        See select_inside_polygon for more docuemntation.
        """
        self._select_outside_polygon(CPolyline(init_points=points))

    def deselect_inside_polygon(self, points):
        """
        Will select all points outside polygon.

        See select_inside_polygon for more docuemntation.
        """
        self._deselect_inside_polygon(CPolyline(init_points=points))

    def deselect_outside_polygon(self, points):
        """
        Will select all points outside polygon.

        See select_inside_polygon for more docuemntation.
        """
        self._deselect_outside_polygon(CPolyline(init_points=points))

    @select_method
    def select_true(self, rd_kw, intersect=False):
        """
        Assume that input rd_kw is a boolean mask.
        """
        self._select_true(rd_kw)

    @select_method
    def select_false(self, rd_kw, intersect=False):
        """
        Assume that input rd_kw is a boolean mask.
        """
        self._select_false(rd_kw)

    @select_method
    def select_from_layer(self, layer, k, value, intersect=False):
        """Will select all the cells in in @layer with value @value - at
        vertical coordinate @k.

        The input @layer should be of type Layer - from the
        ecl.ecl.faults.layer module. The k value must in the range
        [0,grid.nz) and the dimensions of the layer must correspond
        exactly to nx,ny of the grid.
        """
        grid = self.grid
        if k < 0 or k >= grid.getNZ():
            raise ValueError(
                "Invalid k value:%d - must be in range [0,%d)" % (k, grid.getNZ())
            )

        if grid.getNX() != layer.getNX():
            raise ValueError(
                "NX dimension mismatch. Grid:%d  layer:%d"
                % (grid.getNX(), layer.getNX())
            )

        if grid.getNY() != layer.getNY():
            raise ValueError(
                "NY dimension mismatch. Grid:%d  layer:%d"
                % (grid.getNY(), layer.getNY())
            )

        self._select_from_layer(layer, k, value)

    #################################################################

    def scalar_apply_kw(self, target_kw, scalar, func_dict, force_active=False):
        """
        Helper function to apply a function with one scalar arg on target_kw.
        """
        data_type = target_kw.data_type
        if data_type in func_dict:
            func = func_dict[data_type]
            func(target_kw, scalar, force_active)
        else:
            raise Exception("scalar_apply_kw() only supported for INT/FLOAT/DOUBLE")

    def iadd_kw(self, target_kw, delta_kw, force_active=False):
        """
        The functions iadd_kw(), copy_kw(), set_kw(), scale_kw() and
        shift_kw() are not meant to be used as methods of the
        ResdataRegion class (altough that is of course perfectly OK) -
        rather a ResdataRegion instance is passed as an argument to an
        ResdataKW method, and then that method "flips things around" and
        calls one of these methods with the ResdataKW instance as
        argument. This applies to all the ResdataKW methods which take an
        optional "mask" argument.
        """
        if isinstance(delta_kw, ResdataKW):
            if target_kw.assert_binary(delta_kw):
                self._iadd_kw(target_kw, delta_kw, force_active)
            else:
                raise TypeError("Type mismatch")
        else:
            self.shift_kw(target_kw, delta_kw, force_active=force_active)

    def shift_kw(self, rd_kw, shift, force_active=False):
        """
        See usage documentation on iadd_kw().
        """
        self.scalar_apply_kw(
            rd_kw,
            shift,
            {
                ResDataType.RD_INT: self._shift_kw_int,
                ResDataType.RD_FLOAT: self._shift_kw_float,
                ResDataType.RD_DOUBLE: self._shift_kw_double,
            },
            force_active,
        )

    def isub_kw(self, target_kw, delta_kw, force_active=False):
        if isinstance(delta_kw, ResdataKW):
            if target_kw.assert_binary(delta_kw):
                self._isub_kw(target_kw, delta_kw, force_active)
            else:
                raise TypeError("Type mismatch")
        else:
            self.shift_kw(target_kw, -delta_kw, force_active=force_active)

    def scale_kw(self, rd_kw, scale, force_active=False):
        """
        See usage documentation on iadd_kw().
        """
        self.scalar_apply_kw(
            rd_kw,
            scale,
            {
                ResDataType.RD_INT: self._scale_kw_int,
                ResDataType.RD_FLOAT: self._scale_kw_float,
                ResDataType.RD_DOUBLE: self._scale_kw_double,
            },
            force_active,
        )

    def imul_kw(self, target_kw, other, force_active=False):
        if isinstance(other, ResdataKW):
            if target_kw.assert_binary(other):
                self._imul_kw(target_kw, other)
            else:
                raise TypeError("Type mismatch")
        else:
            self.scale_kw(target_kw, other, force_active)

    def idiv_kw(self, target_kw, other, force_active=False):
        if isinstance(other, ResdataKW):
            if target_kw.assert_binary(other):
                self._idiv_kw(target_kw, other)
            else:
                raise TypeError("Type mismatch")
        else:
            if target_kw.data_type.is_int():
                scale = 1 // other
            else:
                scale = 1.0 / other
            self.scale_kw(target_kw, scale, force_active)

    def copy_kw(self, target_kw, src_kw, force_active=False):
        """
        See usage documentation on iadd_kw().
        """
        if target_kw.assert_binary(src_kw):
            self._copy_kw(target_kw, src_kw, force_active)
        else:
            raise TypeError("Type mismatch")

    def set_kw(self, rd_kw, value, force_active=False):
        """
        See usage documentation on iadd_kw().
        """
        self.scalar_apply_kw(
            rd_kw,
            value,
            {
                ResDataType.RD_INT: self._set_kw_int,
                ResDataType.RD_FLOAT: self._set_kw_float,
                ResDataType.RD_DOUBLE: self._set_kw_double,
            },
            force_active,
        )

    def sum_kw(self, kw, force_active=False):
        data_type = kw.data_type
        if data_type == ResDataType.RD_FLOAT:
            return self._sum_kw_float(kw, force_active)

        if data_type == ResDataType.RD_INT:
            return self._sum_kw_int(kw, force_active)

        if data_type == ResDataType.RD_DOUBLE:
            return self._sum_kw_double(kw, force_active)

        if data_type == ResDataType.RD_BOOL:
            return self._sum_kw_bool(kw, force_active)

        raise ValueError("sum_kw only supported for; INT/FLOAT/DOUBLE/BOOL")

    #################################################################

    def rd_region_instance(self):
        """
        Helper function (attribute) to support run-time typechecking.
        """
        return True

    def active_size(self):
        return len(self._get_active_list())

    def global_size(self):
        return len(self._get_global_list())

    def get_active_list(self):
        """
        IntVector instance with active indices in the region.
        """
        active_list = self._get_active_list()
        active_list.setParent(self)
        return active_list

    def get_global_list(self):
        """
        IntVector instance with global indices in the region.
        """
        global_list = self._get_global_list()
        global_list.setParent(self)
        return global_list

    def get_ijk_list(self):
        """
        WIll return a Python list of (ij,k) tuples for the region.
        """
        global_list = self.getGlobalList()
        ijk_list = []
        for g in global_list:
            ijk_list.append(self.grid.get_ijk(global_index=g))

        return ijk_list

    def contains_ijk(self, i, j, k):
        """
        Will check if the cell given by i,j,k is part of the region.
        """
        return self._contains_ijk(i, j, k)

    def contains_global(self, global_index):
        """
        Will check if the cell given by @global_index is part of the region.
        """
        return self._contains_global(global_index)

    def contains_active(self, active_index):
        """
        Will check if the cell given by @active_index is part of the region.
        """
        return self._contains_active(active_index)

    def kw_index_list(self, rd_kw, force_active):
        c_ptr = self._get_kw_index_list(rd_kw, force_active)
        index_list = IntVector.createCReference(c_ptr, self)
        return index_list

    @property
    def name(self):
        return self._get_name()

    def get_name(self):
        return self._get_name()

    def set_name(self, name):
        self._set_name(name)


monkey_the_camel(ResdataRegion, "selectTrue", ResdataRegion.select_true)
monkey_the_camel(ResdataRegion, "selectFalse", ResdataRegion.select_false)
monkey_the_camel(ResdataRegion, "selectFromLayer", ResdataRegion.select_from_layer)
monkey_the_camel(ResdataRegion, "getActiveList", ResdataRegion.get_active_list)
monkey_the_camel(ResdataRegion, "getGlobalList", ResdataRegion.get_global_list)
monkey_the_camel(ResdataRegion, "getIJKList", ResdataRegion.get_ijk_list)
monkey_the_camel(ResdataRegion, "getName", ResdataRegion.get_name)
monkey_the_camel(ResdataRegion, "setName", ResdataRegion.set_name)
