import math

import pytest
from resdata.geometry import CPolyline, GeometryTools, Polyline
from resdata.geometry.xyz_io import XYZIo

from tests import ResdataTest


class GeometryToolsTest(ResdataTest):
    def test_distance(self):
        p1 = (1, 1)
        p2 = (1, 2, 3)
        with self.assertRaises(ValueError):
            GeometryTools.distance(p1, p2)

        with self.assertRaises(TypeError):
            GeometryTools.distance(1, p2)

        p2 = (2, 2)
        self.assertEqual(GeometryTools.distance(p1, p2), math.sqrt(2))

        p1 = (1, 1, 1)
        p2 = (2, 2, 2)
        self.assertEqual(GeometryTools.distance(p1, p2), math.sqrt(3))

    def test_join__polylines(self):
        l1 = Polyline(init_points=[(0, 1), (1, 1)])
        l2 = CPolyline(init_points=[(2, -1), (2, 0)])
        l3 = CPolyline(init_points=[(2, 2), (2, 3)])
        l4 = Polyline()
        l5 = CPolyline(init_points=[(0.5, 0), (0.5, 2)])

        with self.assertRaises(ValueError):
            GeometryTools.joinPolylines(l1, l4)

        with self.assertRaises(ValueError):
            GeometryTools.joinPolylines(l4, l1)

        self.assertIsNone(GeometryTools.joinPolylines(l1, l5))

        self.assertEqual(GeometryTools.joinPolylines(l1, l2), [(1, 1), (2, 0)])

    def test_join_extend_polylines_onto(self):
        l1 = Polyline(init_points=[(0, 1), (1, 1)])
        l2 = CPolyline(init_points=[(2, 0), (2, 2)])
        l3 = CPolyline(init_points=[(0.5, 0), (0.5, 2)])
        l4 = Polyline(init_points=[(0, 5), (1, 5)])
        l5 = Polyline(init_points=[(0, 5)])

        self.assertIsNone(GeometryTools.connectPolylines(l1, l3))

        with self.assertRaises(ValueError):
            GeometryTools.connectPolylines(l1, l5)

        with self.assertRaises(ValueError):
            GeometryTools.connectPolylines(l1, l4)

        self.assertEqual(GeometryTools.connectPolylines(l1, l2), [(1, 1), (2, 1)])

    def test_ray_line_intersection(self):
        p = GeometryTools.rayLineIntersection((0, 0), (1, 0), (5, -1), (5, 1))
        self.assertEqual(p, (5, 0))

        self.assertIsNone(
            GeometryTools.rayLineIntersection((0, 0), (-1, 0), (5, -1), (5, 1))
        )
        self.assertIsNone(
            GeometryTools.rayLineIntersection((0, 0), (0, 1), (5, -1), (5, 1))
        )
        self.assertIsNone(
            GeometryTools.rayLineIntersection((0, 0), (0, -1), (5, -1), (5, 1))
        )

        p = GeometryTools.rayLineIntersection((0, 0), (1, 1), (5, -6), (5, 6))
        self.assertEqual(p, (5, 5))

    def test_nearest_point(self):
        l1 = Polyline(init_points=[(0, 0), (10, 0)])

        p = GeometryTools.nearestPointOnPolyline((5, 5), l1)
        self.assertEqual(p, (5, 0))


def test_that_ray_along_a_collinear_segment_ahead_returns_the_midpoint():
    # point behind a segment that lies on the ray -> coincident, forward overlap
    result = GeometryTools.rayLineIntersection((0, 0), (1, 0), (1, 0), (2, 0))

    assert result == pytest.approx((1.5, 0.0))


def test_that_ray_ending_inside_a_collinear_segment_returns_the_midpoint():
    # point inside the segment with a short ray -> coincident via the segment
    # projection branch
    result = GeometryTools.rayLineIntersection((1.7, 0), (-0.1, 0), (1, 0), (2, 0))

    assert result == pytest.approx((1.5, 0.0))


def test_that_collinear_but_non_overlapping_ray_returns_none():
    result = GeometryTools.rayLineIntersection((5, 0), (1, 0), (1, 0), (2, 0))

    assert result is None


def test_that_connect_polylines_requires_at_least_two_points():
    with pytest.raises(ValueError, match="at least two points"):
        GeometryTools.connectPolylines([(0, 0)], [(10, 10), (11, 11)])


def test_that_connect_polylines_extends_from_the_front_end():
    connection = GeometryTools.connectPolylines([(0, 0), (1, 1)], [(-2, -1), (-1, -2)])

    assert connection[0] == pytest.approx((0, 0))
    assert connection[1] == pytest.approx((-1.5, -1.5))


def test_that_connect_polylines_returns_none_for_intersecting_polylines():
    assert GeometryTools.connectPolylines([(0, 0), (2, 2)], [(0, 2), (2, 0)]) is None


def test_that_nearest_point_on_polyline_requires_at_least_two_points():
    with pytest.raises(ValueError, match="len\\(\\) >= 2"):
        GeometryTools.nearestPointOnPolyline((0, 0), [(5, 5)])
