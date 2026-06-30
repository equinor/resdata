import pytest
from resdata.geometry import CPolyline, GeoPointset, GeoRegion, Surface
from resdata.util.util import IntVector


def make_region(preselect=False):
    surface = Surface(None, 4, 3, 1, 1, 0, 0, 0.0)
    pointset = GeoPointset.fromSurface(surface)
    region = GeoRegion(pointset, preselect=preselect)
    return surface, region


def make_inner_polygon():
    return CPolyline(
        init_points=[
            (0.5, 0.5),
            (2.5, 0.5),
            (2.5, 1.5),
            (0.5, 1.5),
        ]
    )


def active_indices(region):
    return list(region.getActiveList())


def test_new_region_starts_empty_with_empty_int_vector():
    _surface, region = make_region()

    active = region.getActiveList()

    assert isinstance(active, IntVector)
    assert len(region) == 0
    assert len(active) == 0
    assert active_indices(region) == []


def test_preselect_starts_with_every_index_in_order():
    _surface, region = make_region(preselect=True)

    assert len(region) == 12
    assert active_indices(region) == list(range(12))


def test_select_inside_polygon_returns_only_interior_grid_points():
    _surface, region = make_region()

    region.select_inside(make_inner_polygon())

    assert len(region) == 2
    assert active_indices(region) == [5, 6]


def test_select_outside_polygon_returns_ordered_complement():
    _surface, region = make_region()

    region.select_outside(make_inner_polygon())

    assert len(region) == 10
    assert active_indices(region) == [0, 1, 2, 3, 4, 7, 8, 9, 10, 11]


def test_select_inside_polygon_twice_is_idempotent():
    _surface, region = make_region()
    polygon = make_inner_polygon()

    region.select_inside(polygon)
    first = active_indices(region)
    region.select_inside(polygon)

    assert len(region) == 2
    assert active_indices(region) == first == [5, 6]


def test_select_inside_then_outside_polygon_selects_all_points():
    _surface, region = make_region()
    polygon = make_inner_polygon()

    region.select_inside(polygon)
    region.select_outside(polygon)

    assert len(region) == 12
    assert active_indices(region) == list(range(12))


def test_deselect_inside_polygon_from_preselected_region_leaves_complement():
    _surface, region = make_region(preselect=True)

    region.deselect_inside(make_inner_polygon())

    assert len(region) == 10
    assert active_indices(region) == [0, 1, 2, 3, 4, 7, 8, 9, 10, 11]


def test_select_above_horizontal_line_includes_points_on_the_line():
    _surface, region = make_region()

    region.select_above([(0, 1), (3, 1)])

    assert len(region) == 8
    assert active_indices(region) == [0, 1, 2, 3, 4, 5, 6, 7]


def test_select_below_then_deselect_below_horizontal_line_clears_selection():
    _surface, region = make_region()
    line = [(0, 1), (3, 1)]

    region.select_below(line)
    assert active_indices(region) == [8, 9, 10, 11]

    region.deselect_below(line)

    assert len(region) == 0
    assert active_indices(region) == []


def test_invalid_line_shapes_raise_value_error_without_selecting_points():
    _surface, region = make_region()

    with pytest.raises(ValueError):
        region.select_above(((2,), (1, 3)))
    with pytest.raises(ValueError):
        region.select_below(((0, 1), (2, 3), (4, 5)))

    assert len(region) == 0
    assert active_indices(region) == []
