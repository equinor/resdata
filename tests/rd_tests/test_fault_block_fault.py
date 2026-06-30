import math

import pytest
from resdata import ResDataType
from resdata.grid import GridGenerator
from resdata.grid.faults import FaultBlockLayer
from resdata.resfile import ResdataKW


def make_layer_and_block():
    grid = GridGenerator.create_rectangular((4, 3, 2), (2, 3, 4))
    keyword = ResdataKW("FAULTBLK", grid.get_global_size(), ResDataType.RD_INT)
    keyword.assign(0)
    nx = grid.get_nx()
    ny = grid.get_ny()
    for i, j in [(0, 0), (1, 0), (1, 1), (3, 2)]:
        keyword[i + j * nx + nx * ny] = 7
    keyword[2 + nx * ny] = 8
    layer = FaultBlockLayer(grid, 1)
    layer.load_keyword(keyword)
    return layer, layer.get_block(7)


def make_empty_block():
    grid = GridGenerator.create_rectangular((2, 2, 1), (1, 1, 1))
    layer = FaultBlockLayer(grid, 0)
    return layer.add_block(123)


def test_block_identity_len_centroid_and_unsigned_region_wrap():
    layer, block = make_layer_and_block()

    assert block.getBlockID() == 7
    assert len(block) == 4
    assert block.getCentroid() == (3.5, 3.75)
    assert block.getParentLayer() == layer

    block.assignToRegion(2**32)
    assert list(block.getRegionList()) == [0]


def test_negative_index_cell_coordinates_and_large_positive_region_wrap():
    unused_layer, block = make_layer_and_block()

    cell = block[-1]
    assert (cell.i, cell.j, cell.k) == (3, 2, 1)
    assert (cell.x, cell.y, cell.z) == (7.0, 7.5, 6.0)
    assert str(cell) == "(3,2)"

    block.assignToRegion(2**40 + 123)
    assert list(block.getRegionList()) == [123]


def test_global_index_list_order_and_signed_region_wrap():
    unused_layer, block = make_layer_and_block()

    assert list(block.getGlobalIndexList()) == [12, 13, 17, 23]

    block.assignToRegion(2**31)
    assert list(block.getRegionList()) == [-2147483648]


def test_region_list_is_sorted_unique_after_wrapped_assignments():
    unused_layer, block = make_layer_and_block()

    block.assignToRegion(2**40 + 3)
    block.assignToRegion(2**32 + 1)
    block.assignToRegion(2**40 + 3)

    assert list(block.getRegionList()) == [1, 3]


def test_negative_region_overflow_wraps_to_max_int():
    unused_layer, block = make_layer_and_block()

    block.assignToRegion(-(2**31) - 1)

    assert list(block.getRegionList()) == [2147483647]


def test_empty_block_collections_and_huge_region_wrap():
    block = make_empty_block()

    assert len(block) == 0
    assert list(block.getGlobalIndexList()) == []
    assert list(block.getRegionList()) == []
    assert len(block.getEdgePolygon()) == 0
    assert block.getNeighbours() == []

    block.assignToRegion(2**100 + 1)
    assert list(block.getRegionList()) == [1]


def test_empty_block_centroid_nan_and_negative_huge_region_wrap():
    block = make_empty_block()

    xc, yc = block.getCentroid()
    assert math.isnan(xc)
    assert math.isnan(yc)

    block.assignToRegion(-(2**100) + 2)
    assert list(block.getRegionList()) == [2]


def test_out_of_range_indices_raise_index_error_before_region_wrap():
    unused_layer, block = make_layer_and_block()

    with pytest.raises(IndexError, match="out of range"):
        block[len(block)]
    with pytest.raises(IndexError, match="out of range"):
        block[-len(block) - 1]

    block.assignToRegion(2**63 - 1)
    assert list(block.getRegionList()) == [-1]


def test_bool_index_uses_integer_indexing_and_min_int_region_wrap():
    unused_layer, block = make_layer_and_block()

    cell = block[True]
    assert (cell.i, cell.j, cell.k) == (1, 0, 1)
    assert (cell.x, cell.y, cell.z) == (3.0, 1.5, 6.0)

    block.assignToRegion(-(2**63))
    assert list(block.getRegionList()) == [0]


def test_neighbour_block_ids_and_very_large_region_wrap():
    unused_layer, block = make_layer_and_block()

    assert [neighbour.getBlockID() for neighbour in block.getNeighbours()] == [8]

    block.assignToRegion(2**64 + 9)
    assert list(block.getRegionList()) == [9]
