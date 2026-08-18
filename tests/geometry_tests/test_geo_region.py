import hypothesis.strategies as st
import pytest
from hypothesis import given
from resdata.geometry import CPolyline, GeoPointset, GeoRegion, Surface

from tests import ResdataTest


class GeoRegionTest(ResdataTest):
    def test_init(self):
        pointset = GeoPointset()
        georegion = GeoRegion(pointset)
        self.assertEqual(0, len(georegion))

    def test_repr(self):
        pointset = GeoPointset()
        georegion = GeoRegion(pointset)
        self.assertTrue(repr(georegion).startswith("GeoRegion"))

    @staticmethod
    def small_surface():
        ny, nx = 12, 12
        xinc, yinc = 1, 1
        xstart, ystart = -1, -1
        angle = 0.0
        s_args = (None, nx, ny, xinc, yinc, xstart, ystart, angle)
        return Surface(*s_args)

    def test_select_polygon(self):
        surface = self.small_surface()
        pointset = GeoPointset.fromSurface(surface)
        georegion = GeoRegion(pointset)
        self.assertEqual(0, len(georegion))
        points = [(-0.1, 2.0), (1.9, 8.1), (6.1, 8.1), (9.1, 5), (7.1, 0.9)]
        polygon = CPolyline(name="test_polygon", init_points=points)
        picked = 52  # https://www.futilitycloset.com/2013/04/24/picks-theorem/
        georegion.select_inside(polygon)
        self.assertEqual(picked, len(georegion))
        georegion.deselect_inside(polygon)
        self.assertEqual(0, len(georegion))
        georegion.select_outside(polygon)
        self.assertEqual(len(surface) - picked, len(georegion))
        georegion.deselect_outside(polygon)
        self.assertEqual(0, len(georegion))

        georegion.select_inside(polygon)
        georegion.select_outside(polygon)
        self.assertEqual(len(surface), len(georegion))
        georegion.deselect_inside(polygon)
        georegion.deselect_outside(polygon)
        self.assertEqual(0, len(georegion))

        georegion.select_inside(polygon)
        self.assertEqual(picked, len(georegion))
        internal_square = [(2.5, 2.5), (2.5, 6.5), (6.5, 6.5), (6.5, 2.5)]
        georegion.deselect_inside(CPolyline(init_points=internal_square))
        self.assertEqual(picked - 4 * 4, len(georegion))  # internal square is 4x4

    def test_select_halfspace(self):
        surface = self.small_surface()
        pointset = GeoPointset.fromSurface(surface)
        georegion = GeoRegion(pointset)
        self.assertEqual(0, len(georegion))
        line = [(-0.1, 2.0), (1.9, 8.1)]
        picked = 118
        georegion.select_above(line)
        self.assertEqual(picked, len(georegion))
        georegion.deselect_above(line)
        self.assertEqual(0, len(georegion))
        georegion.select_below(line)
        self.assertEqual(len(surface) - picked, len(georegion))
        georegion.deselect_below(line)
        self.assertEqual(0, len(georegion))

        georegion.select_above(line)
        georegion.select_below(line)
        self.assertEqual(len(surface), len(georegion))
        georegion.deselect_above(line)
        georegion.deselect_below(line)
        self.assertEqual(0, len(georegion))

    def test_raises(self):
        surface = self.small_surface()
        pointset = GeoPointset.fromSurface(surface)
        georegion = GeoRegion(pointset)
        with self.assertRaises(ValueError):
            georegion.select_above(((2,), (1, 3)))
        with self.assertRaises(ValueError):
            georegion.select_above((("not-a-number", 2), (1, 3)))


def _make_region(preselect=False):
    surface = Surface(nx=3, ny=2, xinc=1.0, yinc=1.0, xstart=0.0, ystart=0.0, angle=0.0)
    surface.assign(1.0)
    pointset = surface.getPointset()
    return GeoRegion(pointset, preselect=preselect)


def test_that_selecting_with_a_non_polyline_raises_value_error():
    region = _make_region()

    with pytest.raises(ValueError, match="Need to select with a CPolyline"):
        region.select_inside("not a polyline")


def test_that_selecting_inside_a_covering_polygon_selects_every_point():
    region = _make_region()
    polygon = CPolyline(init_points=[(-1, -1), (10, -1), (10, 10), (-1, 10), (-1, -1)])

    region.select_inside(polygon)

    assert len(region) == 6


def test_that_selecting_above_and_deselecting_below_a_line_are_consistent():
    region = _make_region()
    line = ((-1.0, 0.5), (10.0, 0.5))

    region.select_above(line)
    selected_above = len(region)

    region.deselect_below(line)

    # Deselecting below a line the points are already above should be a no-op.
    assert len(region) == selected_above
    assert selected_above > 0


def test_that_preselected_region_starts_with_all_points_active():
    region = _make_region(preselect=True)

    assert len(region) == 6
    assert "preselected" in repr(region)


dimensions = st.integers(min_value=1, max_value=10)
finites = st.floats(min_value=-1e10, max_value=1e10, allow_nan=False)
lines = st.builds(
    CPolyline, init_points=st.lists(st.tuples(finites, finites), min_size=2, max_size=2)
)
polygons = st.builds(CPolyline, init_points=st.lists(st.tuples(finites, finites)))
surfaces = st.builds(
    Surface,
    nx=dimensions,
    ny=dimensions,
    xinc=finites,
    yinc=finites,
    xstart=finites,
    ystart=finites,
    angle=finites,
)

polygon_selectors = st.sampled_from(
    [
        lambda r: r.select_inside,
        lambda r: r.deselect_inside,
        lambda r: r.select_outside,
        lambda r: r.deselect_outside,
    ]
)

line_selectors = st.sampled_from(
    [
        lambda r: r.select_above,
        lambda r: r.deselect_above,
        lambda r: r.select_below,
        lambda r: r.deselect_below,
    ]
)


@st.composite
def geo_regions(draw):
    surface = draw(surfaces)
    surface.assign(draw(finites))
    pointset = surface.getPointset()
    region = GeoRegion(pointset, preselect=draw(st.booleans()))
    for line in draw(st.lists(lines)):
        selector = draw(line_selectors)
        selector(region)(line)
    for polygon in draw(st.lists(polygons)):
        selector = draw(polygon_selectors)
        selector(region)(polygon)
    return region


@given(geo_regions())
def test_that_len_and_active_list_agree(region):
    assert len(region) == len(region.getActiveList())
