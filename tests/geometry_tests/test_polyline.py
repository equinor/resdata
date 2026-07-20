import pytest
from resdata.geometry import GeometryTools, Polyline
from resdata.geometry.xyz_io import XYZIo

from tests import ResdataTest


class PolylineTest(ResdataTest):
    def setUp(self):
        self.polyline = self.createTestPath("local/geometry/pol11.xyz")
        self.closed_polyline = self.createTestPath("local/geometry/pol8.xyz")

    def test_construction(self):
        polyline = Polyline(name="test line")

        with self.assertRaises(IndexError):
            polyline.isClosed()

        self.assertEqual(polyline.getName(), "test line")

        self.assertEqual(len(polyline), 0)

        polyline.addPoint(0, 0, 0)
        self.assertEqual(len(polyline), 1)

        polyline.addPoint(1, 1, 0)
        self.assertEqual(len(polyline), 2)

        polyline.addPoint(1, 1.5)
        self.assertEqual(len(polyline), 3)

        self.assertEqual(polyline[0], (0, 0, 0))
        self.assertEqual(polyline[1], (1, 1, 0))
        self.assertEqual(polyline[2], (1, 1.5))

        polyline.addPoint(0, 1, 0)
        self.assertFalse(polyline.isClosed())

        polyline.addPoint(0, 0, 0)
        self.assertTrue(polyline.isClosed())

    def test_construction_default(self):
        with self.assertRaises(TypeError):
            pl = Polyline(init_points=1)

        with self.assertRaises(TypeError):
            pl = Polyline(init_points=[1.23])

        pl = Polyline(init_points=[(1, 0), (1, 1), (1, 2)])
        self.assertEqual(len(pl), 3)

    def test_iteration(self):
        values = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (1, 1, 1)]

        polyline = Polyline(name="iteration line")

        for p in values:
            polyline.addPoint(*p)

        for index, point in enumerate(polyline):
            self.assertEqual(point, values[index])

    def test_read_xyz_from_file(self):
        with self.assertRaises(IOError):
            XYZIo.readXYZFile("does/not/exist.xyz")

        polyline = XYZIo.readXYZFile(self.polyline)

        self.assertEqual(polyline.getName(), "pol11.xyz")
        self.assertEqual(len(polyline), 13)
        self.assertFalse(polyline.isClosed())
        self.assertEqual(
            polyline[0], (390271.843750, 6606121.334396, 1441.942627)
        )  # first point
        self.assertEqual(
            polyline[12], (389789.263184, 6605784.945099, 1446.627808)
        )  # last point

        polyline = XYZIo.readXYZFile(self.closed_polyline)

        self.assertEqual(polyline.getName(), "pol8.xyz")
        self.assertEqual(len(polyline), 21)
        self.assertTrue(polyline.isClosed())
        self.assertEqual(
            polyline[0], (396202.413086, 6606091.935028, 1542.620972)
        )  # first point
        self.assertEqual(
            polyline[20], (396202.413086, 6606091.935028, 1542.620972)
        )  # last point

    def test_closed(self):
        pl = Polyline(init_points=[(1, 0), (1, 1), (0, 2)])
        self.assertFalse(pl.isClosed())
        pl.addPoint(1, 0)
        self.assertEqual(4, len(pl))
        self.assertTrue(pl.isClosed())

        pl = Polyline(init_points=[(1, 0), (1, 1), (0, 2)])
        self.assertFalse(pl.isClosed())
        pl.assertClosed()
        self.assertEqual(4, len(pl))
        self.assertTrue(pl.isClosed())

    def test_save(self):
        work_area = self.tmp_path_factory.mktemp("polyline_fwrite", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(work_area)
            p1 = Polyline(init_points=[(1, 0), (1, 1), (1, 2)])
            p2 = Polyline(init_points=[(1, 0), (1, 1), (1, 2)])
            self.assertTrue(p1 == p2)

            XYZIo.saveXYFile(p1, "poly.xy")

            p2 = XYZIo.readXYFile("poly.xy")
            self.assertTrue(p1 == p2)

    def test_unzip(self):
        p2 = Polyline(init_points=[(1, 0), (1, 1), (1, 2)])
        p3 = Polyline(init_points=[(1, 0, 1), (1, 1, 2), (1, 2, 3)])
        x, y = p2.unzip()
        self.assertEqual(x, [1, 1, 1])
        self.assertEqual(y, [0, 1, 2])

        x, y, z = p3.unzip()
        self.assertEqual(x, [1, 1, 1])
        self.assertEqual(y, [0, 1, 2])
        self.assertEqual(z, [1, 2, 3])

        with self.assertRaises(ValueError):
            x, y, z = p2.unzip()

        with self.assertRaises(ValueError):
            x, y = p3.unzip()

    def test_intersection(self):
        p1 = Polyline(init_points=[(0, 0), (1, 0)])
        p2 = Polyline(init_points=[(0.5, 0.5), (0.5, -0.5)])
        p3 = Polyline(init_points=[(0, 1), (1, 1)])

        self.assertTrue(GeometryTools.polylinesIntersect(p1, p2))
        self.assertFalse(GeometryTools.polylinesIntersect(p2, p3))
        self.assertFalse(GeometryTools.polylinesIntersect(p1, p3))

        self.assertTrue(p1.intersects(p2))
        self.assertTrue(p2.intersects(p1))

        self.assertTrue(not p1.intersects(p3))
        self.assertTrue(not p3.intersects(p1))

    def test_add(self):
        l1 = Polyline(init_points=[(-1, 0.5), (0.5, 0.5)])
        l2 = Polyline(init_points=[(-1, 0.5), (0.5, 0.5)])

        l3 = l1 + l2
        self.assertEqual(len(l3), 4)
        self.assertEqual(l1[0], l3[0])
        self.assertEqual(l1[1], l3[1])
        self.assertEqual(l1[0], l3[2])
        self.assertEqual(l1[1], l3[3])

        l4 = l1
        l4 += l2
        self.assertEqual(l3, l4)

    def test_extend_to_edge(self):
        bound = Polyline(init_points=[(0, 0), (1, 0), (1, 1), (0, 1)])
        l1 = Polyline(init_points=[(-1, 0.5), (0.5, 0.5)])
        l2 = Polyline(init_points=[(0.25, 0.25), (0.75, 0.75)])

        # Bound is not closed
        with self.assertRaises(AssertionError):
            GeometryTools.extendToEdge(bound, l1)

        bound.assertClosed()
        # l1 is not fully contained in bound
        with self.assertRaises(ValueError):
            GeometryTools.extendToEdge(bound, l1)

        l3 = GeometryTools.extendToEdge(bound, l2)
        self.assertEqual(l3[0], (0.00, 0.00))
        self.assertEqual(l3[1], (0.25, 0.25))
        self.assertEqual(l3[2], (0.75, 0.75))
        self.assertEqual(l3[3], (1.00, 1.00))
        self.assertEqual(len(l3), 4)


def test_that_str_lists_all_points():
    polyline = Polyline(init_points=[(0, 0), (1, 2)])

    text = str(polyline)

    assert text == "Polyline:[ (0,0) (1,2) ]"


def test_that_polylines_of_different_length_are_not_equal():
    short = Polyline(init_points=[(0, 0)])
    long = Polyline(init_points=[(0, 0), (1, 1)])

    assert short != long


def test_that_polylines_with_a_differing_point_are_not_equal():
    first = Polyline(init_points=[(0, 0), (1, 1)])
    second = Polyline(init_points=[(0, 0), (2, 2)])

    assert first != second


def test_that_equal_polylines_compare_equal():
    first = Polyline(init_points=[(0, 0), (1, 1)])
    second = Polyline(init_points=[(0, 0), (1, 1)])

    assert first == second


def test_that_assert_closed_appends_the_first_point_keeping_z():
    polyline = Polyline(init_points=[(0, 0, 5), (1, 1, 6), (2, 0, 7)])

    polyline.assertClosed()

    assert polyline.isClosed()
    assert polyline[-1] == (0, 0, 5)


def test_that_unzip2_returns_only_x_and_y_for_three_dimensional_points():
    polyline = Polyline(init_points=[(1, 2, 9), (3, 4, 8)])

    x, y = polyline.unzip2()

    assert x == [1, 3]
    assert y == [2, 4]


def test_that_getitem_out_of_range_raises_index_error():
    polyline = Polyline(init_points=[(0, 0), (1, 1)])

    with pytest.raises(IndexError):
        polyline[5]


def test_that_connect_returns_the_shortest_link_to_the_target():
    polyline = Polyline(init_points=[(0, 0), (0, 1)])
    target = Polyline(init_points=[(10, 0), (10, 1)])

    link = polyline.connect(target)

    # The link starts at one of the polyline endpoints and ends at its
    # projection onto the target line, a horizontal distance of 10 away.
    assert link[0] in [(0, 0), (0, 1)]
    assert link[1][0] == pytest.approx(10.0)
    assert GeometryTools.distance(link[0], link[1]) == pytest.approx(10.0)


def test_that_connect_uses_the_first_endpoint_when_it_is_closer():
    polyline = Polyline(init_points=[(0, 0), (0, 5)])
    target = Polyline(init_points=[(-5, 1), (5, 1)])

    link = polyline.connect(target)

    # The first endpoint (0, 0) is closest to the target line y=1.
    assert link[0] == (0, 0)
    assert link[1] == pytest.approx((0.0, 1.0))
