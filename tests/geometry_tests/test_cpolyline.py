import math

import pytest
from resdata.geometry import CPolyline, Polyline
from resdata.geometry.xyz_io import XYZIo

from tests import ResdataTest


class CPolylineTest(ResdataTest):
    def setUp(self):
        self.polyline1 = self.createTestPath("local/geometry/pol11.xyz")
        self.polyline2 = self.createTestPath("local/geometry/pol8.xyz")
        self.polyline3 = self.createTestPath("local/geometry/pol8_noend.xyz")

    def test_construction(self):
        polyline = CPolyline()
        self.assertEqual(len(polyline), 0)

        with self.assertRaises(IOError):
            CPolyline.createFromXYZFile("Does/not/exist")

        p1 = CPolyline.createFromXYZFile(self.polyline1)
        self.assertEqual(len(p1), 13)
        x, y = p1[-1]
        self.assertEqual(x, 389789.263184)
        self.assertEqual(y, 6605784.945099)

        p2 = CPolyline.createFromXYZFile(self.polyline2)
        self.assertEqual(len(p2), 20)
        x, y = p2[-1]
        self.assertEqual(x, 396056.314697)
        self.assertEqual(y, 6605835.119461)

        p3 = CPolyline.createFromXYZFile(self.polyline3)
        self.assertEqual(len(p3), 20)
        x, y = p3[-1]
        self.assertEqual(x, 396056.314697)
        self.assertEqual(y, 6605835.119461)

    def test_front(self):
        polyline = CPolyline()
        polyline.addPoint(1, 1)
        polyline.addPoint(0, 0, front=True)
        self.assertEqual(len(polyline), 2)

        x, y = polyline[0]
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

        x, y = polyline[1]
        self.assertEqual(x, 1)
        self.assertEqual(y, 1)

    def test_equal(self):
        pl1 = CPolyline(name="Poly1", init_points=[(0, 0), (1, 1), (2, 2)])
        pl2 = CPolyline(name="Poly2", init_points=[(0, 0), (1, 1), (2, 2)])
        pl3 = CPolyline(init_points=[(0, 0), (1, 1), (2, 3)])

        self.assertEqual(pl1, pl1)
        self.assertEqual(pl1, pl2)
        self.assertFalse(pl1 == pl3)

    def test_length(self):
        polyline = CPolyline(init_points=[(0, 1)])
        self.assertEqual(polyline.segmentLength(), 0)

        polyline = CPolyline(init_points=[(0, 0), (1, 0), (1, 1), (2, 2)])
        self.assertEqual(polyline.segmentLength(), 2 + math.sqrt(2))

    def test_extend_to_bbox(self):
        bbox = [(0, 0), (10, 0), (10, 10), (0, 10)]

        polyline = CPolyline(init_points=[(11, 11), (13, 13)])
        with self.assertRaises(ValueError):
            polyline.extendToBBox(bbox, start=False)

        polyline = CPolyline(init_points=[(1, 1), (3, 3)])

        line1 = polyline.extendToBBox(bbox, start=True)
        self.assertEqual(line1, CPolyline(init_points=[(1, 1), (0, 0)]))

        line1 = polyline.extendToBBox(bbox, start=False)
        self.assertEqual(line1, CPolyline(init_points=[(3, 3), (10, 10)]))

        line2 = CPolyline(name="MyPoly", init_points=[(1, 1), (3, 3)])
        extended = line2.extendToBBox(bbox, start=False)
        self.assertEqual(extended.getName(), "Extend:MyPoly")

    def test_str(self):
        named = CPolyline(name="MyPoly", init_points=[(0, 0), (1, 1)])
        self.assertIn("MyPoly", str(named))
        unnamed = CPolyline(init_points=[(0, 0), (1, 1)])
        self.assertTrue(str(unnamed).startswith("["))

    def test_item(self):
        polyline = CPolyline()
        polyline.addPoint(10, 20)
        self.assertEqual(len(polyline), 1)

        with self.assertRaises(TypeError):
            x, y = polyline["KEY"]

        with self.assertRaises(IndexError):
            x, y = polyline[10]

        x, y = polyline[0]
        self.assertEqual(x, 10)
        self.assertEqual(y, 20)

        polyline.addPoint(20, 20)
        x, y = polyline[-1]
        self.assertEqual(x, 20)
        self.assertEqual(y, 20)

    def test_cross_segment(self):
        polyline = CPolyline(init_points=[(0, 0), (1, 0), (1, 1)])
        #
        #            x
        #            |
        #            |
        #            |
        #    x-------x
        #

        self.assertTrue(polyline.segmentIntersects((0.5, 0.5), (0.5, -0.5)))
        self.assertTrue(polyline.segmentIntersects((0.5, 0.5), (1.5, 0.5)))

        self.assertFalse(polyline.segmentIntersects((0.5, 0.5), (0.5, 1.5)))
        self.assertFalse(polyline.segmentIntersects((0.5, 0.5), (-0.5, 0.5)))
        self.assertFalse(polyline.segmentIntersects((0.5, 1.5), (1.5, 1.5)))

        self.assertTrue(polyline.segmentIntersects((1.0, 1.0), (1.5, 1.5)))
        self.assertTrue(polyline.segmentIntersects((1.5, 1.5), (1.0, 1.0)))
        self.assertTrue(polyline.segmentIntersects((1, 0), (1.0, 1.0)))

    def test_intersects(self):
        polyline1 = CPolyline(init_points=[(0, 0), (1, 0), (1, 1)])
        polyline2 = CPolyline(init_points=[(0.50, 0.50), (1.50, 0.50)])
        polyline3 = Polyline(init_points=[(0.50, 0.50), (1.50, 0.50)])
        polyline4 = CPolyline(init_points=[(0.50, 1.50), (1.50, 1.50)])

        self.assertTrue(polyline1.intersects(polyline2))
        self.assertTrue(polyline1.intersects(polyline3))
        self.assertFalse(polyline1.intersects(polyline4))

    def test_intersects2(self):
        polyline = CPolyline(init_points=[(2, 10), (2, 100)])
        self.assertTrue(polyline.intersects(polyline))

    def test_name(self):
        p1 = CPolyline()
        self.assertTrue(p1.getName() is None)

        p2 = CPolyline(name="Poly2")
        self.assertEqual(p2.getName(), "Poly2")

        p1 = CPolyline.createFromXYZFile(self.polyline1, name="poly")
        self.assertEqual(p1.getName(), "poly")

    def test_unzip(self):
        pl = CPolyline(init_points=[(0, 3), (1, 4), (2, 5)])
        x, y = pl.unzip()
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, [3, 4, 5])


def test_that_repr_matches_str():
    polyline = CPolyline(name="border", init_points=[(0, 0), (1, 1)])

    assert repr(polyline) == str(polyline)


def test_that_adding_two_polylines_concatenates_their_points():
    first = CPolyline(init_points=[(0, 0), (1, 1)])
    second = CPolyline(init_points=[(2, 2), (3, 3)])

    joined = first + second

    assert len(joined) == 4
    assert joined[0] == pytest.approx((0, 0))
    assert joined[3] == pytest.approx((3, 3))


def test_that_right_adding_a_point_sequence_prepends_it():
    polyline = CPolyline(init_points=[(2, 2), (3, 3)])

    joined = [(0, 0), (1, 1)] + polyline

    assert len(joined) == 4
    assert joined[0] == pytest.approx((0, 0))
    assert joined[3] == pytest.approx((3, 3))


def test_that_segment_length_of_empty_polyline_raises_value_error():
    polyline = CPolyline(name="empty")

    with pytest.raises(ValueError, match="zero point polyline"):
        polyline.segmentLength()


def test_that_unzip2_returns_separate_x_and_y_lists():
    polyline = CPolyline(init_points=[(1, 2), (3, 4)])

    x, y = polyline.unzip2()

    assert x == pytest.approx([1, 3])
    assert y == pytest.approx([2, 4])


def test_that_getitem_with_non_integer_index_raises_type_error():
    polyline = CPolyline(init_points=[(0, 0), (1, 1)])

    with pytest.raises(TypeError, match="Index argument must be integer"):
        polyline["first"]


def test_that_connect_links_the_nearest_endpoint_to_the_target():
    polyline = CPolyline(init_points=[(0, 0), (0, 5)])
    target = CPolyline(init_points=[(-5, 1), (5, 1)])

    link = polyline.connect(target)

    # The first endpoint (0, 0) is closest to the target line y=1.
    assert link[0] == pytest.approx((0, 0))
    assert link[1] == pytest.approx((0.0, 1.0))


def test_that_connect_uses_the_last_endpoint_when_it_is_closer():
    polyline = CPolyline(init_points=[(0, 5), (0, 0)])
    target = CPolyline(init_points=[(-5, 1), (5, 1)])

    link = polyline.connect(target)

    # The last endpoint (0, 0) is closest to the target line y=1.
    assert link[0] == pytest.approx((0, 0))
    assert link[1] == pytest.approx((0.0, 1.0))
