from __future__ import annotations

import pytest

from ._grid_fixtures import build_single_cell_grid, make_rectangular_grid

# Unit cube spanning [1,2]x[1,2]x[0,1]. Offset from the origin so no corner
# sits at (0, 0, *), which would mark the cell as degenerate.
_UNIT_CELL_CORNERS = (
    (1, 1, 0),
    (2, 1, 0),
    (1, 2, 0),
    (2, 2, 0),
    (1, 1, 1),
    (2, 1, 1),
    (1, 2, 1),
    (2, 2, 1),
)


@pytest.fixture
def unit_cube_grid():
    return build_single_cell_grid(_UNIT_CELL_CORNERS)


@pytest.mark.parametrize("start_ijk", [None, (0, 0, 0)])
def test_that_point_at_unit_cube_center_is_in_the_cell(unit_cube_grid, start_ijk):
    assert unit_cube_grid.find_cell(1.5, 1.5, 0.5, start_ijk=start_ijk) == (0, 0, 0)


@pytest.mark.parametrize("start_ijk", [None, (0, 0, 0)])
def test_that_point_far_from_unit_cube_is_outside(unit_cube_grid, start_ijk):
    assert unit_cube_grid.find_cell(100.0, 100.0, 100.0, start_ijk=start_ijk) is None
    assert unit_cube_grid.find_cell(-100.0, -100.0, -100.0, start_ijk=start_ijk) is None


@pytest.mark.parametrize("start_ijk", [None, (0, 0, 0)])
def test_that_point_just_above_unit_cube_top_is_outside(unit_cube_grid, start_ijk):
    assert unit_cube_grid.find_cell(1.5, 1.5, 2.0, start_ijk=start_ijk) is None


@pytest.mark.parametrize("start_ijk", [None, (0, 0, 0)])
def test_that_point_on_outer_max_i_face_is_inside_unit_cube(unit_cube_grid, start_ijk):
    assert unit_cube_grid.find_cell(2.0, 1.5, 0.5, start_ijk=start_ijk) == (0, 0, 0)


@pytest.mark.parametrize("start_ijk", [None, (0, 0, 0)])
def test_that_point_on_outer_top_face_is_inside_unit_cube(unit_cube_grid, start_ijk):
    assert unit_cube_grid.find_cell(1.5, 1.5, 1.0, start_ijk=start_ijk) == (0, 0, 0)


@pytest.mark.parametrize("start_ijk", [None, (0, 0, 0)])
def test_that_point_on_outer_min_i_face_is_inside_unit_cube(unit_cube_grid, start_ijk):
    assert unit_cube_grid.find_cell(1.0, 1.5, 0.5, start_ijk=start_ijk) == (0, 0, 0)


def test_that_twisted_single_cell_contains_no_points():
    # Top and bottom faces swapped: bottom face lies above top face. Such a
    # cell is treated as degenerate and contains no points.
    corners = (
        (1, 1, 1),
        (2, 1, 1),
        (1, 2, 1),
        (2, 2, 1),
        (1, 1, 0),
        (2, 1, 0),
        (1, 2, 0),
        (2, 2, 0),
    )
    grid = build_single_cell_grid(corners)
    assert grid.find_cell(1.5, 1.5, 0.5) is None
    assert grid.find_cell(1.5, 1.5, 0.5, start_ijk=(0, 0, 0)) is None


def test_that_tainted_single_cell_with_origin_corner_contains_no_points():
    # Corner 0 at (0, 0, 0) marks the cell as degenerate.
    corners = (
        (0, 0, 0),
        (1, 0, 0),
        (0, 1, 0),
        (1, 1, 0),
        (0, 0, 1),
        (1, 0, 1),
        (0, 1, 1),
        (1, 1, 1),
    )
    grid = build_single_cell_grid(corners, actnum_value=0)
    assert grid.find_cell(0.5, 0.5, 0.5) is None
    assert grid.find_cell(0.5, 0.5, 0.5, start_ijk=(0, 0, 0)) is None


def test_that_concave_cell_contains_interior_point_but_not_concave_notch():
    # One top corner is pulled inward in the xy-plane, producing a
    # non-convex hexahedron.
    corners = (
        (1, 1, 0),
        (2, 1, 0),
        (1, 2, 0),
        (1.3, 1.3, 0),
        (1, 1, 1),
        (2, 1, 1),
        (1, 2, 1),
        (2, 2, 1),
    )
    grid = build_single_cell_grid(corners)
    assert grid.find_cell(1.2, 1.2, 0.5) == (0, 0, 0)
    assert grid.find_cell(1.2, 1.2, 0.5, start_ijk=(0, 0, 0)) == (0, 0, 0)
    assert grid.find_cell(1.8, 1.8, 0.1) is None


def test_that_collapsed_edge_cell_contains_point_on_collapsed_edge():
    # Corners 6 and 7 collapsed to the same point: top-j edge of the cell
    # collapses and the cell becomes a wedge.
    corners = (
        (1, 1, 0),
        (2, 1, 0),
        (1, 2, 0),
        (2, 2, 0),
        (1, 1, 1),
        (2, 1, 1),
        (1.5, 2, 1),
        (1.5, 2, 1),
    )
    grid = build_single_cell_grid(corners)
    # Midpoint of the segment from corner 2 (1, 2, 0) to corners 6/7 at
    # (1.5, 2, 1).
    assert grid.find_cell(1.25, 2.0, 0.5) == (0, 0, 0)
    assert grid.find_cell(1.25, 2.0, 0.5, start_ijk=(0, 0, 0)) == (0, 0, 0)


def test_that_collapsed_edge_cell_contains_point_well_inside_the_wedge():
    corners = (
        (1, 1, 0),
        (2, 1, 0),
        (1, 2, 0),
        (2, 2, 0),
        (1, 1, 1),
        (2, 1, 1),
        (1.5, 2, 1),
        (1.5, 2, 1),
    )
    grid = build_single_cell_grid(corners)
    assert grid.find_cell(1.3, 1.3, 0.5) == (0, 0, 0)
    assert grid.find_cell(1.3, 1.3, 0.5, start_ijk=(0, 0, 0)) == (0, 0, 0)


@pytest.mark.parametrize("actnum_value", [0, 1])
def test_that_flat_single_cell_returns_zero_volume(actnum_value):
    # All corners share the same z; the cell has no volume. The lookup
    # treats it as invalid: active cells map to themselves, inactive ones
    # are not contained.
    corners = (
        (1, 1, 0),
        (2, 1, 0),
        (1, 2, 0),
        (2, 2, 0),
        (1, 1, 0),
        (2, 1, 0),
        (1, 2, 0),
        (2, 2, 0),
    )
    grid = build_single_cell_grid(corners, actnum_value=actnum_value)
    found = grid.find_cell(1.5, 1.5, 0.0)
    if actnum_value == 1:
        assert found == (0, 0, 0)
    else:
        assert found is None


def test_that_box_search_finds_cell_at_start_hint():
    grid = make_rectangular_grid(5, 5, 5, 1, 1, 1)
    # Cell at (1, 0, 0) center is (1.5, 0.5, 0.5).
    assert grid.find_cell(1.5, 0.5, 0.5, start_ijk=(1, 0, 0)) == (1, 0, 0)


def test_that_box_search_finds_cell_far_from_start_hint():
    grid = make_rectangular_grid(5, 5, 5, 1, 1, 1)
    assert grid.find_cell(4.5, 0.5, 4.5, start_ijk=(0, 4, 0)) == (4, 0, 4)


def test_that_box_search_returns_none_for_point_outside_grid():
    grid = make_rectangular_grid(5, 5, 5, 1, 1, 1)
    assert grid.find_cell(100.0, 100.0, 100.0, start_ijk=(1, 0, 0)) is None


def test_that_box_search_finds_cell_without_start_hint():
    grid = make_rectangular_grid(5, 5, 5, 1, 1, 1)
    assert grid.find_cell(4.5, 3.5, 4.5) == (4, 3, 4)
