import pytest
from resdata import ResDataType
from resdata.grid import GridGenerator, ResdataRegion
from resdata.resfile import ResdataKW

BIG_C_INT_ZERO = 2**32


@pytest.fixture
def grid():
    return GridGenerator.create_rectangular((3, 3, 2), (1, 1, 1))


def selected_globals(region):
    return list(region.getGlobalList())


def test_select_islice_wraps_large_int_to_zero(grid):
    region = ResdataRegion(grid, False)

    region.select_islice(BIG_C_INT_ZERO, BIG_C_INT_ZERO)

    assert selected_globals(region) == [0, 3, 6, 9, 12, 15]


def test_select_jslice_wraps_large_int_to_zero(grid):
    region = ResdataRegion(grid, False)

    region.select_jslice(BIG_C_INT_ZERO, BIG_C_INT_ZERO)

    assert selected_globals(region) == [0, 1, 2, 9, 10, 11]


def test_select_kslice_wraps_large_int_to_zero(grid):
    region = ResdataRegion(grid, False)

    region.select_kslice(BIG_C_INT_ZERO, BIG_C_INT_ZERO)

    assert selected_globals(region) == list(range(9))


def test_select_box_wraps_large_corner_coordinates_to_origin(grid):
    region = ResdataRegion(grid, False)

    region.select_box(
        (BIG_C_INT_ZERO, BIG_C_INT_ZERO, BIG_C_INT_ZERO),
        (BIG_C_INT_ZERO, BIG_C_INT_ZERO, BIG_C_INT_ZERO),
    )

    assert selected_globals(region) == [0]


def test_deselect_islice_wraps_large_int_to_zero(grid):
    region = ResdataRegion(grid, True)

    region.deselect_islice(BIG_C_INT_ZERO, BIG_C_INT_ZERO)

    assert selected_globals(region) == [1, 2, 4, 5, 7, 8, 10, 11, 13, 14, 16, 17]


def test_select_equal_wraps_large_int_value_to_zero(grid):
    keyword = ResdataKW("INT", grid.get_global_size(), ResDataType.RD_INT)
    keyword[0 : len(keyword)] = 1
    keyword[0] = 0
    keyword[5] = 0
    region = ResdataRegion(grid, False)

    region.select_equal(keyword, BIG_C_INT_ZERO)

    assert selected_globals(region) == [0, 5]


def test_deselect_equal_wraps_large_int_value_to_zero(grid):
    keyword = ResdataKW("INT", grid.get_global_size(), ResDataType.RD_INT)
    keyword[0 : len(keyword)] = 1
    keyword[0] = 0
    keyword[5] = 0
    region = ResdataRegion(grid, True)

    region.deselect_equal(keyword, BIG_C_INT_ZERO)

    assert selected_globals(region) == [
        1,
        2,
        3,
        4,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
    ]


def test_contains_global_wraps_large_int_to_zero(grid):
    region = ResdataRegion(grid, False)
    region.select_box((0, 0, 0), (0, 0, 0))

    assert region.contains_global(BIG_C_INT_ZERO) is True


def test_contains_ijk_wraps_large_int_coordinates_to_origin(grid):
    region = ResdataRegion(grid, False)
    region.select_box((0, 0, 0), (0, 0, 0))

    assert region.contains_ijk(BIG_C_INT_ZERO, BIG_C_INT_ZERO, BIG_C_INT_ZERO) is True


def test_set_kw_wraps_large_int_value_to_zero_on_selected_cells(grid):
    keyword = ResdataKW("INT", grid.get_global_size(), ResDataType.RD_INT)
    keyword[0 : len(keyword)] = 7
    region = ResdataRegion(grid, False)
    region.select_islice(BIG_C_INT_ZERO, BIG_C_INT_ZERO)

    region.set_kw(keyword, BIG_C_INT_ZERO)

    assert [keyword[index] for index in range(len(keyword))] == [
        0,
        7,
        7,
        0,
        7,
        7,
        0,
        7,
        7,
        0,
        7,
        7,
        0,
        7,
        7,
        0,
        7,
        7,
    ]
