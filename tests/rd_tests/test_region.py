import numpy as np
import pytest
from resdata import ResDataType
from resdata.grid import Grid, GridGenerator, ResdataRegion
from resdata.grid.faults import Layer
from resdata.resfile import ResdataKW
from resdata.util.util import IntVector

from tests import ResdataTest


class RegionTest(ResdataTest):
    def test_equal(self):
        grid = GridGenerator.create_rectangular((10, 10, 1), (1, 1, 1))
        kw_int = ResdataKW("INT", grid.get_global_size(), ResDataType.RD_INT)
        kw_float = ResdataKW("FLOAT", grid.get_global_size(), ResDataType.RD_FLOAT)

        kw_int[0:49] = 1
        region = ResdataRegion(grid, False)
        region.select_equal(kw_int, 1)
        glist = region.get_global_list()
        for g in glist:
            self.assertEqual(kw_int[g], 1)

        ijk_list = region.get_ijk_list()
        self.assertEqual(len(ijk_list), len(glist))

        with self.assertRaises(ValueError):
            region.select_equal(kw_float, 1)

        with pytest.raises(
            ValueError,
            match="The select_equal method must have an integer valued keyword",
        ):
            region.deselect_equal(kw_float, 2)

    def test_sum(self):
        grid = GridGenerator.create_rectangular((10, 10, 1), (1, 1, 1))
        kw_mask = ResdataKW("INT", grid.get_global_size(), ResDataType.RD_INT)
        int_value = ResdataKW("INT", grid.get_global_size(), ResDataType.RD_INT)
        float_value = ResdataKW("FLOAT", grid.get_global_size(), ResDataType.RD_FLOAT)
        double_value = ResdataKW(
            "DOUBLE", grid.get_global_size(), ResDataType.RD_DOUBLE
        )
        bool_value = ResdataKW("BOOL", grid.get_global_size(), ResDataType.RD_BOOL)

        kw_mask[0:50] = 1

        for i in range(len(int_value)):
            float_value[i] = i
            double_value[i] = i
            int_value[i] = i
            bool_value[i] = True

        region = ResdataRegion(grid, False)
        region.select_equal(kw_mask, 1)

        self.assertEqual(int_value.sum(), 99 * 100 / 2)
        self.assertEqual(int_value.sum(mask=region), 49 * 50 / 2)
        self.assertEqual(double_value.sum(mask=region), 1.0 * 49 * 50 / 2)
        self.assertEqual(float_value.sum(mask=region), 1.0 * 49 * 50 / 2)
        self.assertEqual(bool_value.sum(mask=region), 50)


@pytest.fixture
def actnum():
    actnum = IntVector(initial_size=100, default_value=0)
    actnum[0:50] = 1
    return actnum


@pytest.fixture
def grid(actnum):
    return GridGenerator.create_rectangular((10, 10, 1), (1, 1, 1), actnum=actnum)


@pytest.fixture
def empty_region(grid):
    return ResdataRegion(grid, False)


def test_empty_region_has_size_zero(empty_region):
    assert not empty_region
    assert empty_region.active_size() == 0
    assert empty_region.global_size() == 0


@pytest.fixture
def full_region(grid):
    return ResdataRegion(grid, True)


def test_full_region_has_total_size(grid, full_region):
    assert full_region
    assert full_region.active_size() == grid.get_num_active()
    assert full_region.global_size() == grid.get_global_size()


def test_full_region_has_selected_all(empty_region, full_region):
    empty_region.select_all()
    assert empty_region == full_region


def test_empty_region_has_deselected_all(empty_region, full_region):
    full_region.deselect_all()
    assert empty_region == full_region


@pytest.fixture
def inactive_region(empty_region):
    inactive_region = empty_region.copy()
    inactive_region.select_inactive()
    return inactive_region


def test_inactive_region_size_agrees_with_grid(inactive_region, grid):
    assert inactive_region.active_size() == 0
    assert inactive_region.global_size() == len(grid) - grid.get_num_active()


@pytest.fixture
def active_region(empty_region):
    inactive_region = empty_region.copy()
    inactive_region.select_active()
    return inactive_region


def test_and_is_symmertic(active_region):
    region = active_region
    regionc = region.copy()

    assert region & region == region
    region &= region
    assert regionc == region


def test_or_is_symmertic(active_region):
    region = active_region
    regionc = region.copy()

    assert region | region == region

    region |= region
    assert regionc == region


def test_union_is_symetric(active_region):
    region = active_region
    regionc = region.copy()

    region.union_with(region)
    assert regionc == region


def test_intersection_is_symetric(active_region):
    region = active_region
    regionc = region.copy()

    region.intersect_with(region)
    assert regionc == region


def test_minus_self_gives_empty(active_region, empty_region):
    region = active_region
    assert region - region == empty_region
    region -= region
    assert region == empty_region


def test_reset(active_region, empty_region):
    empty_region.select_active()
    assert empty_region == active_region
    empty_region.reset()
    assert empty_region != active_region
    assert empty_region.global_size() == 0


def test_plus_is_symmetric(active_region):
    region = active_region
    regionc = region.copy()

    assert region + region == region
    region += region
    assert region == regionc


@pytest.fixture
def actnum_kw(grid):
    return grid.export_ACTNUM_kw()


def test_select_more_actnum_is_active_region(active_region, empty_region, actnum_kw):
    empty_region.select_more(actnum_kw, 0)
    assert empty_region == active_region


def test_select_equal_actnum_is_active_region(active_region, empty_region, actnum_kw):
    empty_region.select_equal(actnum_kw, 1)
    assert empty_region == active_region


def test_select_less_actnum_is_inactive_region(
    inactive_region, empty_region, actnum_kw
):
    empty_region.select_less(actnum_kw, 1)
    assert empty_region == inactive_region


def test_deselect_more_actnum_is_inactive_region(
    inactive_region, full_region, actnum_kw
):
    full_region.deselect_more(actnum_kw, 0)
    assert full_region == inactive_region


def test_deselect_less_actnum_is_active_region(active_region, full_region, actnum_kw):
    full_region.deselect_less(actnum_kw, 1)
    assert full_region == active_region


def test_deselect_equal_actnum_is_inactive_region(
    inactive_region, full_region, actnum_kw
):
    full_region.deselect_equal(actnum_kw, 1)
    assert full_region == inactive_region


@pytest.fixture
def poro(grid):
    return grid.create_kw(
        np.ones((grid.nx, grid.ny, grid.nz), dtype=np.float32), "PORO", True
    )


@pytest.fixture
def poro_global(grid):
    return grid.create_kw(
        np.ones((grid.nx, grid.ny, grid.nz), dtype=np.float32), "PORO", False
    )


@pytest.fixture
def poro_int(grid):
    return grid.create_kw(
        np.ones((grid.nx, grid.ny, grid.nz), dtype=np.int32), "PORO", True
    )


@pytest.fixture
def poro_double(grid):
    return grid.create_kw(
        np.ones((grid.nx, grid.ny, grid.nz), dtype=np.float64), "PORO", True
    )


@pytest.fixture
def poro_double_global(grid):
    return grid.create_kw(
        np.ones((grid.nx, grid.ny, grid.nz), dtype=np.float64), "PORO", False
    )


def test_select_in_range(empty_region, active_region, poro):
    empty_region.select_in_range(poro, 0.9, 1.1)
    assert empty_region == active_region


def test_deselect_in_range(full_region, inactive_region, poro):
    full_region.deselect_in_range(poro, 0.9, 1.1)
    assert full_region == inactive_region


def test_less_than_self_is_empty(empty_region, poro):
    cmp_less_region = empty_region.copy()
    cmp_less_region.select_cmp_less(poro, poro)
    assert cmp_less_region == empty_region


def test_deselect_less_than_self_is_same(active_region, poro):
    cmp_less_region = active_region.copy()
    cmp_less_region.deselect_cmp_less(poro, poro)
    assert cmp_less_region == active_region


def test_more_than_self_is_all_active(empty_region, poro, active_region):
    cmp_more_region = empty_region.copy()
    cmp_more_region.select_cmp_more(poro, poro)
    assert cmp_more_region == active_region


def test_deselect_more_than_self_is_same(empty_region, poro, active_region):
    cmp_more_region = active_region.copy()
    cmp_more_region.deselect_cmp_more(poro, poro)
    assert cmp_more_region == empty_region


def test_deselect_active_is_inactive(full_region, inactive_region):
    full_region.deselect_active()
    assert full_region == inactive_region


def test_deselect_inactive_is_inactive(full_region, active_region):
    full_region.deselect_inactive()
    assert full_region == active_region


def test_select_deep(empty_region, full_region):
    empty_region.select_deep(0)
    assert empty_region == full_region


def test_deselect_deep(empty_region, full_region):
    full_region.deselect_deep(-1.0)
    assert empty_region == full_region


def test_select_shallow(empty_region, full_region):
    empty_region.select_shallow(1.0)
    assert empty_region == full_region


def test_deselect_shallow(empty_region, full_region):
    full_region.deselect_shallow(1.0)
    assert empty_region == full_region


def test_select_small(empty_region, full_region):
    empty_region.select_small(1.1)
    assert empty_region == full_region


def test_deselect_small(empty_region, full_region):
    full_region.deselect_small(1.1)
    assert empty_region == full_region


def test_select_large(empty_region, full_region):
    empty_region.select_large(0.9)
    assert empty_region == full_region


def test_deselect_large(empty_region, full_region):
    full_region.deselect_large(0.9)
    assert empty_region == full_region


def test_select_thin(empty_region, full_region):
    empty_region.select_thin(1.1)
    assert empty_region == full_region


def test_deselect_thin(empty_region, full_region):
    full_region.deselect_thin(1.1)
    assert empty_region == full_region


def test_select_thick(empty_region, full_region):
    empty_region.select_thick(0.9)
    assert empty_region == full_region


def test_deselect_thick(empty_region, full_region):
    full_region.deselect_thick(0.9)
    assert empty_region == full_region


def test_select_box(empty_region, active_region):
    empty_region.select_box((0, 0, 0), (9, 9, 0))
    assert empty_region == active_region


def test_deselect_box(full_region, inactive_region):
    full_region.deselect_box((0, 0, 0), (9, 9, 0))
    assert full_region == inactive_region


def test_select_islice(empty_region, full_region):
    empty_region.select_islice(0, 9)
    assert empty_region == full_region


def test_deselect_islice(full_region, empty_region):
    full_region.deselect_islice(0, 9)
    assert full_region == empty_region


def test_select_jslice(empty_region, full_region):
    empty_region.select_jslice(0, 9)
    assert empty_region == full_region


def test_deselect_jslice(full_region, empty_region):
    full_region.deselect_jslice(0, 9)
    assert full_region == empty_region


def test_select_kslice(empty_region, full_region):
    empty_region.select_kslice(0, 9)
    assert empty_region == full_region


def test_deselect_kslice(full_region, empty_region):
    full_region.deselect_kslice(0, 9)
    assert full_region == empty_region


def test_select_above_plane(empty_region, full_region):
    empty_region.select_above_plane((0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
    assert empty_region == full_region


def test_deselect_above_plane(empty_region, full_region):
    full_region.deselect_above_plane((0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
    assert empty_region == full_region


def test_select_below_plane(empty_region):
    below = empty_region.copy()
    below.select_below_plane((0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
    assert below == empty_region


def test_deselect_below_plane(full_region):
    above = full_region
    above.deselect_below_plane((0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
    assert above == full_region


def test_that_select_below_plane_selects_cells_strictly_on_negative_side(
    empty_region, grid
):
    # The plane x=5 (normal (1,0,0)) splits the 10x10x1 grid so that the
    # 5x10 slab of cells with i in [0,4] lies strictly below (x<5).
    region = empty_region.copy()
    region.select_below_plane((1.0, 0.0, 0.0), (5.0, 0.0, 0.0))
    assert region.global_size() == 50


@pytest.fixture
def circumference_polygon():
    return [(0.0, 0.0), (0.0, 10.0), (10.0, 10.0), (10.0, 0.0)]


def test_select_inside_polygon(circumference_polygon, empty_region, full_region):
    empty_region.select_inside_polygon(circumference_polygon)
    assert empty_region == full_region


def test_deselect_inside_polygon(circumference_polygon, empty_region, full_region):
    full_region.deselect_inside_polygon(circumference_polygon)
    assert empty_region == full_region


@pytest.fixture
def all_true_kw(grid):
    kw = ResdataKW("BOOL", grid.get_global_size(), ResDataType.RD_BOOL)
    for i in range(grid.get_global_size()):
        kw[i] = True
    return kw


def test_select_true(empty_region, full_region, all_true_kw):
    empty_region.select_true(all_true_kw)
    assert empty_region == full_region


def test_select_false(empty_region, all_true_kw):
    false_region = empty_region.copy()
    false_region.select_false(all_true_kw)
    assert empty_region == false_region


def test_select_layer(empty_region, full_region):
    layer = Layer(10, 10)
    empty_region.select_from_layer(layer, 0, 0)
    assert empty_region == full_region


def test_that_select_outside_polygon_leaves_only_cells_outside_polygon(
    circumference_polygon, empty_region, grid
):
    # A polygon covering only the [0,5]x[0,5] quadrant so that the
    # "outside" selection selects the remaining 75 cells (25 are inside).
    small_polygon = [(0.0, 0.0), (0.0, 5.0), (5.0, 5.0), (5.0, 0.0)]
    empty_region.select_outside_polygon(small_polygon)
    assert empty_region.global_size() == 75


def test_that_deselect_outside_polygon_only_removes_cells_outside_polygon(
    circumference_polygon, full_region
):
    small_polygon = [(0.0, 0.0), (0.0, 5.0), (5.0, 5.0), (5.0, 0.0)]
    full_region.deselect_outside_polygon(small_polygon)
    assert full_region.global_size() == 25


def test_that_invert_toggles_selection_of_every_cell(empty_region, full_region):
    empty_region.invert()
    assert empty_region == full_region
    full_region.invert()
    assert full_region.global_size() == 0


@pytest.fixture
def active_int_kw(grid):
    kw = ResdataKW("ACTINT", grid.get_num_active(), ResDataType.RD_INT)
    for i in range(len(kw)):
        kw[i] = 1
    return kw


def test_that_select_equal_supports_active_sized_int_kw(
    empty_region, active_region, active_int_kw
):
    empty_region.select_equal(active_int_kw, 1)
    assert empty_region == active_region


def test_that_deselect_equal_supports_active_sized_int_kw(
    full_region, inactive_region, active_int_kw
):
    full_region.deselect_equal(active_int_kw, 1)
    assert full_region == inactive_region


@pytest.fixture
def active_bool_kw(grid):
    kw = ResdataKW("ACTBOOL", grid.get_num_active(), ResDataType.RD_BOOL)
    for i in range(len(kw)):
        kw[i] = True
    return kw


def test_that_select_true_supports_active_sized_bool_kw(
    empty_region, active_region, active_bool_kw
):
    empty_region.select_true(active_bool_kw)
    assert empty_region == active_region


def test_that_select_false_supports_active_sized_bool_kw(empty_region, active_bool_kw):
    false_region = empty_region.copy()
    false_region.select_false(active_bool_kw)
    assert false_region == empty_region


def test_that_select_in_range_supports_global_sized_kw(
    empty_region, full_region, poro_global
):
    empty_region.select_in_range(poro_global, 0.9, 1.1)
    assert empty_region == full_region


def test_that_deselect_in_range_supports_global_sized_kw(full_region, poro_global):
    full_region.deselect_in_range(poro_global, 0.9, 1.1)
    assert full_region.global_size() == 0


def test_that_select_more_supports_active_sized_float_kw(
    empty_region, active_region, poro
):
    empty_region.select_more(poro, 0.5)
    assert empty_region == active_region


def test_that_select_less_supports_active_sized_float_kw(
    empty_region, active_region, poro
):
    below = empty_region.copy()
    below.select_less(poro, 0.5)
    assert below.global_size() == 0
    full = active_region.copy()
    full.select_less(poro, 1.5)
    assert full == active_region


def test_that_select_more_supports_global_sized_float_kw(
    empty_region, full_region, poro_global
):
    empty_region.select_more(poro_global, 0.5)
    assert empty_region == full_region


def test_that_select_less_supports_global_sized_float_kw(
    empty_region, full_region, poro_global
):
    below = empty_region.copy()
    below.select_less(poro_global, 0.5)
    assert below.global_size() == 0
    above = empty_region.copy()
    above.select_less(poro_global, 1.5)
    assert above == full_region


def test_that_select_more_supports_active_sized_double_kw(
    empty_region, active_region, poro_double
):
    empty_region.select_more(poro_double, 0.5)
    assert empty_region == active_region


def test_that_select_less_supports_active_sized_double_kw(
    empty_region, active_region, poro_double
):
    below = empty_region.copy()
    below.select_less(poro_double, 0.5)
    assert below.global_size() == 0
    full = active_region.copy()
    full.select_less(poro_double, 1.5)
    assert full == active_region


def test_that_select_more_supports_global_sized_double_kw(
    empty_region, full_region, poro_double_global
):
    empty_region.select_more(poro_double_global, 0.5)
    assert empty_region == full_region


def test_that_select_less_supports_global_sized_double_kw(
    empty_region, full_region, poro_double_global
):
    below = empty_region.copy()
    below.select_less(poro_double_global, 0.5)
    assert below.global_size() == 0
    above = empty_region.copy()
    above.select_less(poro_double_global, 1.5)
    assert above == full_region


def test_that_select_more_supports_active_sized_int_kw(
    empty_region, active_region, poro_int
):
    empty_region.select_more(poro_int, 0)
    assert empty_region == active_region


def test_that_select_less_supports_active_sized_int_kw(
    empty_region, active_region, poro_int
):
    below = empty_region.copy()
    below.select_less(poro_int, 1)
    assert below.global_size() == 0
    full = active_region.copy()
    full.select_less(poro_int, 2)
    assert full == active_region


@pytest.fixture
def lo_hi_global(grid):
    lo = ResdataKW("LO", grid.get_global_size(), ResDataType.RD_FLOAT)
    hi = ResdataKW("HI", grid.get_global_size(), ResDataType.RD_FLOAT)
    for i in range(len(lo)):
        lo[i] = 0.0
        hi[i] = 1.0
    return lo, hi


def test_that_select_cmp_less_supports_global_sized_kw(
    empty_region, full_region, lo_hi_global
):
    lo, hi = lo_hi_global
    empty_region.select_cmp_less(lo, hi)
    assert empty_region == full_region


def test_that_select_cmp_more_supports_global_sized_kw(
    empty_region, full_region, lo_hi_global
):
    lo, hi = lo_hi_global
    empty_region.select_cmp_more(hi, lo)
    assert empty_region == full_region


@pytest.fixture
def lo_hi_active(grid):
    lo = ResdataKW("LO", grid.get_num_active(), ResDataType.RD_FLOAT)
    hi = ResdataKW("HI", grid.get_num_active(), ResDataType.RD_FLOAT)
    for i in range(len(lo)):
        lo[i] = 0.0
        hi[i] = 1.0
    return lo, hi


def test_that_select_cmp_less_supports_active_sized_kw(
    empty_region, active_region, lo_hi_active
):
    lo, hi = lo_hi_active
    empty_region.select_cmp_less(lo, hi)
    assert empty_region == active_region


def test_that_set_kw_sets_every_cell_for_float_kw(full_region, poro, grid):
    target = grid.create_kw(
        np.zeros((grid.nx, grid.ny, grid.nz), dtype=np.float32), "PORO", True
    )
    full_region.set_kw(target, 3.0)
    assert list(target) == [3.0] * len(target)


def test_that_set_kw_sets_every_cell_for_double_kw(full_region, poro_double, grid):
    target = grid.create_kw(
        np.zeros((grid.nx, grid.ny, grid.nz), dtype=np.float64), "PORO", True
    )
    full_region.set_kw(target, 3.0)
    assert list(target) == [3.0] * len(target)


def test_that_shift_kw_adds_value_to_every_cell_for_int_kw(full_region, poro_int):
    poro_int_copy = poro_int.copy()
    full_region.shift_kw(poro_int, 2)
    assert list(poro_int) == [v + 2 for v in poro_int_copy]


def test_that_shift_kw_adds_value_to_every_cell_for_double_kw(full_region, poro_double):
    poro_double_copy = poro_double.copy()
    full_region.shift_kw(poro_double, 2.0)
    assert list(poro_double) == [v + 2.0 for v in poro_double_copy]


def test_that_scale_kw_scales_every_cell_for_double_kw(full_region, poro_double):
    poro_double_copy = poro_double.copy()
    full_region.scale_kw(poro_double, 2.0)
    assert list(poro_double) == [v * 2.0 for v in poro_double_copy]


def test_that_isub_kw_with_kw_delta_subtracts_elementwise(full_region, poro):
    poro_copy = poro.copy()
    delta = poro.copy()
    full_region.isub_kw(poro, delta)
    assert list(poro) == [0.0] * len(poro)
    assert poro_copy == delta


def test_iadd_kw_empty(empty_region, poro):
    poro_copy = poro.copy()
    poro.add(poro, mask=empty_region)
    assert poro == poro_copy


def test_iadd_kw_full(full_region, poro):
    poro_copy = poro.copy()
    poro.add(2.0, mask=full_region)
    assert poro == poro_copy + 2


def test_isub_kw_full(full_region, poro):
    poro_copy = poro.copy()
    poro.sub(2.0, mask=full_region)
    assert poro == poro_copy - 2


def test_imul_kw_full(full_region, poro):
    poro_copy = poro.copy()
    poro += 1.0
    poro.mul(2.0, mask=full_region)
    assert poro == (poro_copy + 1.0) * 2.0


def test_idiv_kw_full_scalar(full_region, poro):
    poro_copy = poro.copy()
    poro += 1.0
    poro.div(2.0, mask=full_region)
    assert poro == (poro_copy + 1.0) * 0.5


def test_idiv_kw_full(full_region, poro):
    poro += 1.0
    poro.div(poro, mask=full_region)
    assert list(poro) == [1.0] * len(poro)


def test_idiv_kw_int(full_region, poro_int):
    poro_int += 1
    full_region.idiv_kw(poro_int, 10)
    assert list(poro_int) == [0] * len(poro_int)  # ???


def test_mul_kw_full(full_region, poro):
    poro += 1.0
    poro.mul(poro, mask=full_region)
    assert list(poro) == [4.0] * len(poro)


def test_copy_kw(full_region, poro, grid):
    poro_copy = grid.create_kw(
        np.zeros((grid.nx, grid.ny, grid.nz), dtype=np.float32), "PORO", True
    )
    full_region.copy_kw(poro_copy, poro)
    assert poro_copy == poro


def test_get_active_list(full_region, active_region):
    assert full_region.get_active_list() == active_region.get_active_list()


def test_contains_ijk(full_region):
    assert full_region.contains_ijk(0, 0, 0)


def test_contains_global(full_region):
    assert full_region.contains_global(0)


def test_get_set_name(full_region):
    full_region.set_name("full")
    assert full_region.get_name() == full_region.name
    assert full_region.get_name() == "full"


def test_contains_active(full_region):
    assert full_region.contains_active(0)


def test_kw_index_list(grid, full_region):
    kw_int = ResdataKW("INT", grid.get_global_size(), ResDataType.RD_INT)
    kw_float = ResdataKW("FLOAT", grid.get_global_size(), ResDataType.RD_FLOAT)
    full_region.kw_index_list(kw_int, False)
    full_region.kw_index_list(kw_float, True)
