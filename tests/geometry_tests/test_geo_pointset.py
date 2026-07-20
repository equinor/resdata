import pytest
from resdata.geometry import GeoPointset, Surface

from tests import ResdataTest


class GeoPointsetTest(ResdataTest):
    def test_init(self):
        gp = GeoPointset()
        self.assertEqual(0, len(gp))

    def test_repr(self):
        gp = GeoPointset()
        self.assertTrue(repr(gp).startswith("GeoPointset"))

    def test_from_surface(self):
        srf_path = self.createTestPath("local/geometry/surface/valid_ascii.irap")
        srf = Surface(srf_path)
        gp = GeoPointset.fromSurface(srf)
        self.assertEqual(3871, len(srf))
        self.assertEqual(len(srf), len(gp))

    def test_getitem(self):
        srf_path = self.createTestPath("local/geometry/surface/valid_ascii.irap")
        srf = Surface(srf_path)
        gp = GeoPointset.fromSurface(srf)
        for i in (561, 1105, 1729, 2465, 2821):
            self.assertEqual(gp[i], srf[i])

    def test_equal(self):
        srf_path = self.createTestPath("local/geometry/surface/valid_ascii.irap")
        srf = Surface(srf_path)
        gp = GeoPointset.fromSurface(srf)
        gp2 = GeoPointset.fromSurface(srf)
        self.assertEqual(gp, gp2)


def _make_pointset(nx=3, ny=2):
    surface = Surface(
        nx=nx, ny=ny, xinc=1.0, yinc=1.0, xstart=0.0, ystart=0.0, angle=0.0
    )
    surface.assign(1.0)
    return surface.getPointset()


def test_that_len_matches_the_number_of_surface_nodes():
    pointset = _make_pointset(3, 2)

    assert len(pointset) == 6


def test_that_from_surface_returns_the_surface_pointset():
    surface = Surface(nx=2, ny=2, xinc=1.0, yinc=1.0, xstart=0.0, ystart=0.0, angle=0.0)

    assert GeoPointset.fromSurface(surface) == surface.getPointset()


def test_that_comparing_with_a_non_pointset_is_not_equal():
    pointset = _make_pointset()

    assert not (pointset == 5)


def test_that_negative_index_counts_from_the_end():
    pointset = _make_pointset(3, 2)

    assert pointset[-1] == pointset[len(pointset) - 1]


def test_that_out_of_range_index_raises_index_error():
    pointset = _make_pointset()

    with pytest.raises(IndexError, match="Invalid index"):
        pointset[1000]


def test_that_a_non_int_index_raises_value_error():
    pointset = _make_pointset()

    with pytest.raises(ValueError, match="Index must be int"):
        pointset["x"]
