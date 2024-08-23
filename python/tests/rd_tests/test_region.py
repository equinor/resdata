import numpy as np
import pytest
from resdata import ResDataType
from resdata.grid import Grid, ResdataRegion
from resdata.grid.faults import Layer
from resdata.resfile import ResdataKW
from resdata.util.util import IntVector

from tests import ResdataTest


class RegionTest(ResdataTest):
    def test_equal(self):
        grid = Grid.createRectangular((10, 10, 1), (1, 1, 1))
        kw_int = ResdataKW("INT", grid.getGlobalSize(), ResDataType.RD_INT)
        kw_float = ResdataKW("FLOAT", grid.getGlobalSize(), ResDataType.RD_FLOAT)

        kw_int[0:49] = 1
        region = ResdataRegion(grid, False)
        region.select_equal(kw_int, 1)
        glist = region.getGlobalList()
        for g in glist:
            self.assertEqual(kw_int[g], 1)

        with self.assertRaises(ValueError):
            region.select_equal(kw_float, 1)

    def test_sum(self):
        grid = Grid.createRectangular((10, 10, 1), (1, 1, 1))
        kw_mask = ResdataKW("INT", grid.getGlobalSize(), ResDataType.RD_INT)
        int_value = ResdataKW("INT", grid.getGlobalSize(), ResDataType.RD_INT)
        float_value = ResdataKW("FLOAT", grid.getGlobalSize(), ResDataType.RD_FLOAT)
        double_value = ResdataKW("DOUBLE", grid.getGlobalSize(), ResDataType.RD_DOUBLE)
        bool_value = ResdataKW("BOOL", grid.getGlobalSize(), ResDataType.RD_BOOL)

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
    return Grid.create_rectangular((10, 10, 1), (1, 1, 1), actnum=actnum)


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
    kw = ResdataKW("BOOL", grid.getGlobalSize(), ResDataType.RD_BOOL)
    for i in range(grid.getGlobalSize()):
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
