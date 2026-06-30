import pytest
from resdata.geometry import CPolyline
from resdata.grid import GridGenerator
from resdata.grid.faults import Layer
from resdata.util.util import IntVector


def test_dimensions_are_exact_for_snake_and_camel_methods():
    layer = Layer(3, 7)

    assert layer.get_nx() == 3
    assert layer.get_ny() == 7
    assert layer.getNX() == 3
    assert layer.getNY() == 7
    assert layer.nx == 3
    assert layer.ny == 7


def test_getitem_setitem_round_trip_at_opposite_corners():
    layer = Layer(4, 3)

    layer[0, 0] = 11
    layer[3, 2] = 17

    assert layer[0, 0] == 11
    assert layer[3, 2] == 17
    assert layer.cellSum() == 28


def test_setitem_accepts_various_integer_values_and_updates_count():
    layer = Layer(3, 2)

    layer[0, 0] = -5
    layer[1, 0] = 0
    layer[2, 1] = 123456

    assert layer[0, 0] == -5
    assert layer[1, 0] == 0
    assert layer[2, 1] == 123456
    assert layer.count_equal(-5) == 1
    assert layer.countEqual(0) == 4
    assert layer.count_equal(123456) == 1
    layer[0, 0] = 0
    assert layer.count_equal(-5) == 0
    assert layer.count_equal(0) == 5


def test_indexing_rejects_negative_coordinates():
    layer = Layer(3, 3)

    with pytest.raises(ValueError):
        layer[-1, 0]
    with pytest.raises(ValueError):
        layer[0, -1] = 4


def test_indexing_rejects_out_of_range_and_malformed_coordinates():
    layer = Layer(3, 3)

    with pytest.raises(ValueError):
        layer[3, 0]
    with pytest.raises(ValueError):
        layer[0, 3] = 4
    with pytest.raises(ValueError):
        layer[1]


def test_cell_contact_validates_coordinates_with_index_error():
    layer = Layer(3, 3)

    with pytest.raises(IndexError):
        layer.cellContact((-1, 0), (0, 0))
    with pytest.raises(IndexError):
        layer.cellContact((0, 0), (3, 0))


def test_cell_contact_reports_adjacent_non_adjacent_and_same_cell():
    layer = Layer(4, 4)

    assert layer.cellContact((0, 0), (1, 0)) is True
    assert layer.cellContact((0, 0), (0, 1)) is True
    assert layer.cellContact((0, 0), (2, 0)) is False
    assert layer.cellContact((0, 0), (1, 1)) is False
    assert layer.cellContact((2, 2), (2, 2)) is False


def test_add_interp_barrier_blocks_contact_and_sets_left_barrier():
    layer = Layer(4, 4)
    dimx = layer.getNX() + 1

    layer.addInterpBarrier(1, 1 + 4 * dimx)

    assert layer.left_barrier(1, 0) is True
    assert layer.leftBarrier(1, 3) is True
    assert layer.cellContact((0, 0), (1, 0)) is False
    assert layer.cellContact((1, 3), (0, 3)) is False


def test_add_polyline_barrier_sets_expected_diagonal_edges():
    size = 5
    layer = Layer(size, size)
    grid = GridGenerator.create_rectangular((size, size, 1), (1, 1, 1))
    polyline = CPolyline(init_points=[(0, 0), (size, size)])

    layer.addPolylineBarrier(polyline, grid, 0)

    for index in range(size):
        assert layer.bottom_barrier(index, index) is True
        if index < size - 1:
            assert layer.left_barrier(index + 1, index) is True


def test_update_active_copies_grid_actnum_for_layer_k():
    size = 3
    layer = Layer(size, size)
    actnum = IntVector(initial_size=size * size, default_value=1)
    actnum[4] = 0
    grid = GridGenerator.create_rectangular((size, size, 1), (1, 1, 1), actnum=actnum)

    layer.updateActive(grid, 0)

    assert layer.active_cell(1, 1) is False
    assert layer.activeCell(0, 0) is True
