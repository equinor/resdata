#!/usr/bin/env python
import pytest

from resdata import util

from resdata import ResDataType
from resdata.resfile import ResdataKW
from resdata.grid import Grid, GridGenerator
from resdata.grid.faults import (
    FaultCollection,
    Fault,
    FaultLine,
    FaultSegment,
    FaultBlockLayer,
    SegmentMap,
)
from resdata.util.test import TestAreaContext
from resdata.geometry import Polyline, CPolyline
from tests import ResdataTest


class FaultTest(ResdataTest):
    @classmethod
    def setUpClass(cls):
        cls.grid = GridGenerator.create_rectangular((151, 100, 50), (1, 1, 1))

    def setUp(self):
        self.faults1 = self.createTestPath("local/ECLIPSE/FAULTS/fault1.grdecl")
        self.faults2 = self.createTestPath("local/ECLIPSE/FAULTS/fault2.grdecl")

    def test_PolylineIJ(self):
        nx = 10
        ny = 10
        nz = 10
        grid = GridGenerator.create_rectangular((nx, ny, nz), (0.1, 0.1, 0.1))
        f = Fault(grid, "F")
        f.add_record(0, 1, 0, 0, 0, 0, "Y-")
        f.add_record(2, 2, 0, 1, 0, 0, "X-")
        f.add_record(2, 2, 1, 1, 0, 0, "Y")

        pl = f.get_ij_polyline(0)
        self.assertEqual(pl, [(0, 0), (2, 0), (2, 2), (3, 2)])

    def test_empty_collection(self):
        faults = FaultCollection()
        self.assertEqual(0, len(faults))

        self.assertFalse(faults.hasFault("FX"))

        with self.assertRaises(TypeError):
            _ = faults[[]]

        with self.assertRaises(KeyError):
            _ = faults["FX"]

        with self.assertRaises(IndexError):
            _ = faults[0]

        self.assertFalse("NAME" in faults)

    def test_collection_invalid_arg(self):
        with self.assertRaises(ValueError):
            faults = FaultCollection(self.faults1)

        with self.assertRaises(ValueError):
            faults = FaultCollection(self.faults1, self.faults2)

    def test_splitLine(self):
        faults = FaultCollection(self.grid)
        with self.assertRaises(ValueError):
            # Not slash terminated
            t = faults.splitLine(
                "'F1'             149  149     29   29      1   43    'Y'"
            )

        with self.assertRaises(ValueError):
            # Not integer
            t = faults.splitLine(
                "'F1'             149  149     29   29      1   43X    'Y' /"
            )

        with self.assertRaises(ValueError):
            # Missing item
            t = faults.splitLine(
                "'F1'             149     29   29      1   43    'Y' /"
            )

        with self.assertRaises(ValueError):
            # Quote problem
            t = faults.splitLine(
                "'F1             149     149 29   29      1   43    'X' /"
            )

    def test_empty_fault(self):
        f = Fault(self.grid, "NAME")
        self.assertEqual("NAME", f.getName())

        with self.assertRaises(Exception):
            g = f["Key"]

    def test_empty_faultLine(self):
        fl = FaultLine(self.grid, 10)
        self.assertEqual(10, fl.getK())
        self.assertEqual(0, len(fl))

        with self.assertRaises(TypeError):
            f = fl["Test"]

        with self.assertRaises(IndexError):
            f = fl[0]

    def test_faultLine_center(self):
        nx = 10
        ny = 10
        nz = 2
        grid = GridGenerator.create_rectangular((nx, ny, nz), (0.1, 0.1, 0.1))
        fl = FaultLine(grid, 0)
        C1 = (nx + 1) * 5 + 3
        C2 = C1 + 2
        C3 = C2 + 2

        s1 = FaultSegment(C1, C2)
        s2 = FaultSegment(C2, C3)

        fl.tryAppend(s1)
        fl.tryAppend(s2)

        self.assertEqual(len(fl), 2)
        self.assertEqual(fl.center(), (0.50, 0.50))

    def test_fault_segments(self):
        S1 = FaultSegment(0, 10)
        S2 = FaultSegment(10, 20)
        S3 = FaultSegment(0, 10)
        S4 = FaultSegment(10, 0)
        S5 = FaultSegment(30, 40)
        S6 = FaultSegment(50, 40)
        assert S1 != S2
        assert S1 == S3
        assert S3 == S4
        assert S1.joins(S2)
        assert S2.joins(S4)
        assert S1.joins(S4)
        assert not S2.joins(S5)
        assert not S5.joins(S2)
        assert S6.joins(S5)

    def test_segment_map(self):
        S1 = FaultSegment(0, 10)
        S2 = FaultSegment(10, 20)
        S5 = FaultSegment(30, 40)
        SM = SegmentMap()
        SM.add_segment(S1)
        SM.add_segment(S2)
        assert len(SM) == 3
        SM.verify()
        SM.add_segment(S5)
        assert len(SM) == 5
        SM.print_content()

    def test_faultLine(self):
        fl = FaultLine(self.grid, 10)
        S1 = FaultSegment(0, 10)
        S2 = FaultSegment(10, 20)
        fl.tryAppend(S1)
        fl.tryAppend(S2)
        fl.verify()
        S3 = FaultSegment(20, 30)
        fl.tryAppend(S3)
        fl.verify()
        # ---
        fl = FaultLine(self.grid, 10)
        S1 = FaultSegment(0, 10)
        S2 = FaultSegment(20, 10)
        fl.tryAppend(S1)
        self.assertTrue(fl.tryAppend(S2))
        fl.verify()
        # ---
        fl = FaultLine(self.grid, 10)
        S1 = FaultSegment(10, 0)
        S2 = FaultSegment(20, 10)
        fl.tryAppend(S1)
        fl.tryAppend(S2)
        fl.verify()
        # ---
        fl = FaultLine(self.grid, 10)
        S1 = FaultSegment(10, 0)
        S2 = FaultSegment(10, 20)
        fl.tryAppend(S1)
        fl.tryAppend(S2)
        fl.verify()

        fl = FaultLine(self.grid, 10)
        S1 = FaultSegment(10, 0)
        S2 = FaultSegment(10, 20)
        fl.tryAppend(S1)
        fl.tryAppend(S2)
        S3 = FaultSegment(40, 30)
        self.assertTrue(fl.tryAppend(S3) == False)
        self.assertEqual(len(fl), 2)

        pl = fl.getPolyline()
        self.assertIsInstance(pl, CPolyline)
        self.assertEqual(len(pl), len(fl) + 1)

        S3 = FaultSegment(20, 30)
        fl.tryAppend(S3)
        pl = fl.getPolyline()
        self.assertIsInstance(pl, CPolyline)
        self.assertEqual(len(pl), len(fl) + 1)

    def test_load(self):
        faults = FaultCollection(self.grid, self.faults1)
        self.assertEqual(3, len(faults))
        faults.load(self.grid, self.faults2)
        self.assertEqual(7, len(faults))
        fault1 = faults["F1"]
        layer8 = fault1[8]
        self.assertEqual(len(layer8), 1)

        with self.assertRaises(IOError):
            faults.load(self.grid, "No/this/does/not/exist")

    def test_connect_faults(self):
        grid = GridGenerator.create_rectangular((100, 100, 10), (1, 1, 1))

        #    Fault1                    Fault4
        #      |                         |
        #      |                         |
        #      |                         |
        #      |   -------  Fault2       |
        #      |                         |
        #      |                         |
        #
        #          -------- Fault3
        #

        fault1 = Fault(grid, "Fault1")
        fault2 = Fault(grid, "Fault2")
        fault3 = Fault(grid, "Fault3")
        fault4 = Fault(grid, "Fault4")

        fault1.add_record(1, 1, 10, grid.get_ny() - 1, 0, 0, "X")
        fault2.add_record(5, 10, 15, 15, 0, 0, "Y")
        fault3.add_record(5, 10, 5, 5, 0, 0, "Y")
        fault4.add_record(20, 20, 10, grid.get_ny() - 1, 0, 0, "X")

        for other_fault in [fault2, fault3, fault4]:
            with self.assertRaises(ValueError):
                fault1.extendToFault(other_fault, 0)

        with self.assertRaises(ValueError):
            fault2.extendToFault(fault3, 0)

        for other_fault in [fault1, fault2, fault4]:
            with self.assertRaises(ValueError):
                fault3.extendToFault(other_fault, 0)

        for other_fault in [fault1, fault2, fault3]:
            with self.assertRaises(ValueError):
                fault4.extendToFault(other_fault, 0)

        ext21 = fault2.extendToFault(fault1, 0)
        self.assertEqual(len(ext21), 2)
        p0 = ext21[0]
        p1 = ext21[1]
        self.assertEqual(p0, (5, 16))
        self.assertEqual(p1, (2, 16))

        ext24 = fault2.extendToFault(fault4, 0)
        self.assertEqual(len(ext24), 2)
        p0 = ext24[0]
        p1 = ext24[1]
        self.assertEqual(p0, (11, 16))
        self.assertEqual(p1, (21, 16))

    def test_intersect_intRays(self):
        p1 = (0, 0)
        dir1 = (1, 0)
        p2 = (0, 0)
        dir2 = (0, 1)

        line = Fault.intersectFaultRays((p1, dir1), (p2, dir2))
        self.assertEqual(line, [])

        # Opposite direction
        p3 = (-1, 0)
        dir3 = (-1, 0)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p3, dir3))

        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p3, dir3), (p1, dir1))

        # Parallell with offset
        p4 = (0, 1)
        dir4 = (1, 0)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p4, dir4))

        p5 = (0, 1)
        dir5 = (-1, 0)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p5, dir5))

        p6 = (1, 1)
        dir6 = (1, 0)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p6, dir6))

        p2 = (-1, 0)
        dir2 = (-1, 0)
        join = Fault.intersectFaultRays((p2, dir1), (p1, dir2))
        self.assertEqual(join, [p2, p1])

        join = Fault.intersectFaultRays((p1, dir3), (p3, dir1))
        self.assertEqual(join, [p1, p3])

        p2 = (1, 0)
        dir2 = (1, 0)
        join = Fault.intersectFaultRays((p1, dir1), (p2, dir2))
        self.assertEqual(join, [p1, p2])

        # Orthogonal
        p2 = (1, 1)
        dir2 = (0, 1)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p2, dir2))

        p2 = (0, 1)
        dir2 = (0, 1)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p2, dir2))

        p2 = (-1, 0)
        dir2 = (0, 1)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p2, dir2))

        p2 = (-1, 1)
        dir2 = (0, 1)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p2, dir2))

        p2 = (-1, 1)
        dir2 = (0, -1)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p2, dir2))

        p2 = (3, -1)
        dir2 = (0, -1)
        with self.assertRaises(ValueError):
            Fault.intersectFaultRays((p1, dir1), (p2, dir2))

        p2 = (1, -1)
        dir2 = (0, 1)
        join = Fault.intersectFaultRays((p1, dir1), (p2, dir2))
        self.assertEqual(join, [p1, (1, 0), p2])

        p2 = (1, 1)
        dir2 = (0, -1)
        join = Fault.intersectFaultRays((p1, dir1), (p2, dir2))
        self.assertEqual(join, [p1, (1, 0), p2])

        p2 = (0, 3)
        dir2 = (0, -1)
        join = Fault.intersectFaultRays((p1, dir1), (p2, dir2))
        self.assertEqual(join, [p1, p2])

        p2 = (3, 0)
        dir2 = (0, -1)
        join = Fault.intersectFaultRays((p1, dir1), (p2, dir2))
        self.assertEqual(join, [p1, p2])

    def test_join_faults(self):
        grid = GridGenerator.create_rectangular((100, 100, 10), (1, 1, 1))

        #    Fault1                    Fault4
        #      |                         |
        #      |                         |
        #      |                         |
        #      |   -------  Fault2       |
        #      |                         |
        #      |                         |
        #
        #          -------- Fault3
        #

        fault1 = Fault(grid, "Fault1")
        fault2 = Fault(grid, "Fault2")
        fault3 = Fault(grid, "Fault3")
        fault4 = Fault(grid, "Fault4")

        fault1.add_record(1, 1, 10, grid.get_ny() - 1, 0, 0, "X")
        fault2.add_record(5, 10, 15, 15, 0, 0, "Y")
        fault3.add_record(5, 10, 5, 5, 0, 0, "Y")
        fault4.add_record(20, 20, 10, grid.get_ny() - 1, 0, 0, "X")

        rays = fault1.getEndRays(0)
        self.assertEqual(rays[0], [(2, 10), (0, -1)])
        self.assertEqual(rays[1], [(2, 100), (0, 1)])

        extra = Fault.joinFaults(fault1, fault3, 0)
        self.assertEqual(extra, [(2, 10), (2, 6), (5, 6)])

    def test_contact(self):
        grid = GridGenerator.create_rectangular((100, 100, 10), (1, 1, 1))

        #    Fault1                    Fault4
        #      |                         |
        #      |                         |
        #      |                         |
        #      |   ----------------------+--  Fault2
        #      |                         |
        #      |                         |
        #
        #          -------- Fault3
        #

        fault1 = Fault(grid, "Fault1")
        fault2 = Fault(grid, "Fault2")
        fault3 = Fault(grid, "Fault3")
        fault4 = Fault(grid, "Fault4")

        fault1.add_record(1, 1, 10, grid.get_ny() - 1, 0, 0, "X")
        fault2.add_record(5, 30, 15, 15, 0, 0, "Y")
        fault3.add_record(2, 10, 9, 9, 0, 0, "Y")
        fault4.add_record(20, 20, 10, grid.get_ny() - 1, 0, 0, "X")

        # self.assertFalse( fault1.intersectsFault(fault2 , 0) )
        # self.assertFalse( fault2.intersectsFault(fault1 , 0) )

        # self.assertTrue( fault2.intersectsFault(fault4 , 0) )
        # self.assertTrue( fault4.intersectsFault(fault2 , 0) )

        self.assertTrue(fault1.intersectsFault(fault1, 0))
        # self.assertTrue( fault3.intersectsFault(fault3 , 0) )

    def test_iter(self):
        faults = FaultCollection(self.grid, self.faults1, self.faults2)
        self.assertEqual(7, len(faults))
        c = 0
        for f in faults:
            c += 1
        self.assertEqual(c, len(faults))

        for f in ["F1", "F2", "F3", "F4"]:
            self.assertTrue(f in faults)

        self.assertFalse("FX" in faults)

    def test_fault(self):
        f = Fault(self.grid, "NAME")

        with pytest.raises(ValueError, match="Invalid face:F"):
            # Invalid face
            f.add_record(10, 10, 11, 11, 1, 43, "F")

        with pytest.raises(ValueError, match="Invalid I1:-1"):
            # Invalid coordinates
            f.add_record(-1, 10, 11, 11, 1, 43, "X")

        with pytest.raises(ValueError, match="Invalid I1:10000"):
            # Invalid coordinates
            f.add_record(10000, 100000, 11, 11, 1, 43, "X")

        with pytest.raises(ValueError, match="Invalid I2:10002"):
            # Invalid coordinates
            f.add_record(10, 10002, 11, 11, 1, 43, "X")

        with pytest.raises(ValueError, match="Invalid J1:-11"):
            # Invalid coordinates
            f.add_record(10, 10, -11, 11, 1, 43, "X")

        with pytest.raises(ValueError, match="Invalid J2:11110"):
            # Invalid coordinates
            f.add_record(10, 10, 11, 11110, 1, 43, "X")

        with pytest.raises(ValueError, match="Invalid K1:-11"):
            # Invalid coordinates
            f.add_record(10, 10, 11, 11, -11, 43, "X")

        with pytest.raises(ValueError, match="Invalid K2:43000"):
            # Invalid coordinates
            f.add_record(10, 10, 11, 11, 1, 43000, "X")

        with pytest.raises(ValueError, match="Invalid I1 I2 indices"):
            # Invalid coordinates
            f.add_record(10, 9, 11, 11, 1, 43, "X")

        with pytest.raises(ValueError, match="Invalid J1 J2 indices"):
            # Invalid coordinates
            f.add_record(8, 9, 12, 11, 1, 43, "X")

        with pytest.raises(ValueError, match="Invalid K1 K2 indices"):
            # Invalid coordinates
            f.add_record(8, 9, 11, 11, 100, 43, "X")

        with pytest.raises(ValueError, match="For face:X we must have I1 == I2"):
            # Invalid coordinates/face combination
            f.add_record(10, 11, 11, 11, 1, 43, "X")

        with pytest.raises(ValueError, match="For face:Y we must have J1 == J2"):
            # Invalid coordinates/face combination
            f.add_record(10, 11, 11, 12, 1, 43, "Y")

        with pytest.raises(ValueError, match="For face:K we must have K1 == K2"):
            # Invalid coordinates/face combination
            f.add_record(10, 11, 11, 12, 1, 43, "K")

        f.add_record(10, 10, 0, 10, 1, 10, "X")

    def test_segment(self):
        s0 = FaultSegment(0, 10)
        self.assertEqual(s0.getC1(), 0)
        self.assertEqual(s0.getC2(), 10)

        s0.swap()
        self.assertEqual(s0.getC1(), 10)
        self.assertEqual(s0.getC2(), 0)

    def test_fault_line(self):
        faults = FaultCollection(self.grid, self.faults1, self.faults2)
        for fault in faults:
            for layer in fault:
                for fl in layer:
                    fl.verify()

    def test_fault_line_order(self):
        nx = 120
        ny = 60
        nz = 43
        grid = GridGenerator.create_rectangular((nx, ny, nz), (1, 1, 1))
        with TestAreaContext("python/faults/line_order"):
            with open("faults.grdecl", "w") as f:
                f.write(
                    """FAULTS
\'F\'              105  107     50   50      1   43    \'Y\'    /
\'F\'              108  108     50   50      1   43    \'X\'    /
\'F\'              108  108     50   50     22   43    \'Y\'    /
\'F\'              109  109     49   49      1   43    \'Y\'    /
\'F\'              110  110     49   49      1   43    \'X\'    /
\'F\'              111  111     48   48      1   43    \'Y\'    /
/
"""
                )
            faults = FaultCollection(grid, "faults.grdecl")

        fault = faults["F"]
        layer = fault[29]
        self.assertEqual(len(layer), 2)

        line1 = layer[0]
        line2 = layer[1]
        self.assertEqual(len(line1), 4)
        self.assertEqual(len(line2), 2)

        seg0 = line1[0]
        seg1 = line1[1]
        seg2 = line1[2]
        seg3 = line1[3]
        self.assertEqual(seg0.getCorners(), (50 * (nx + 1) + 104, 50 * (nx + 1) + 107))
        self.assertEqual(seg1.getCorners(), (50 * (nx + 1) + 107, 50 * (nx + 1) + 108))
        self.assertEqual(seg2.getCorners(), (50 * (nx + 1) + 108, 49 * (nx + 1) + 108))
        self.assertEqual(seg3.getCorners(), (49 * (nx + 1) + 108, 49 * (nx + 1) + 109))

    def test_neighbour_cells(self):
        nx = 10
        ny = 8
        nz = 7
        grid = GridGenerator.create_rectangular((nx, ny, nz), (1, 1, 1))
        faults_file = self.createTestPath("local/ECLIPSE/FAULTS/faults_nb.grdecl")
        faults = FaultCollection(grid, faults_file)

        fault = faults["FY"]
        self.assertEqual(len(fault), 1)
        fault_layer = fault[0]

        fl1 = fault_layer[0]
        nb_cells1 = fl1.getNeighborCells()
        true_nb_cells1 = [(0, nx), (1, nx + 1), (2, nx + 2), (3, nx + 3), (4, nx + 4)]
        self.assertListEqual(nb_cells1, true_nb_cells1)

        fl2 = fault_layer[1]
        nb_cells2 = fl2.getNeighborCells()
        true_nb_cells2 = [(6, nx + 6), (7, nx + 7), (8, nx + 8), (9, nx + 9)]
        self.assertListEqual(nb_cells2, true_nb_cells2)

        nb_cells = fault_layer.getNeighborCells()
        self.assertListEqual(nb_cells, true_nb_cells1 + true_nb_cells2)

        fault = faults["FY0"]
        fault_layer = fault[0]
        fl1 = fault_layer[0]
        nb_cells1 = fl1.getNeighborCells()
        true_nb_cells1 = [(-1, 0), (-1, 1), (-1, 2)]
        self.assertListEqual(nb_cells1, true_nb_cells1)

        fault = faults["FYNY"]
        fault_layer = fault[0]
        fl1 = fault_layer[0]
        nb_cells1 = fl1.getNeighborCells()
        true_nb_cells1 = [
            (nx * (ny - 1), -1),
            (nx * (ny - 1) + 1, -1),
            (nx * (ny - 1) + 2, -1),
        ]
        self.assertListEqual(nb_cells1, true_nb_cells1)

        fault = faults["FX"]
        fault_layer = fault[0]
        fl1 = fault_layer[0]
        nb_cells1 = fl1.getNeighborCells()
        true_nb_cells1 = [(0, 1), (nx, nx + 1), (2 * nx, 2 * nx + 1)]
        self.assertListEqual(nb_cells1, true_nb_cells1)

        fault = faults["FX0"]
        fault_layer = fault[0]
        fl1 = fault_layer[0]
        nb_cells1 = fl1.getNeighborCells()
        true_nb_cells1 = [(-1, 0), (-1, nx), (-1, 2 * nx)]
        self.assertListEqual(nb_cells1, true_nb_cells1)

        fault = faults["FXNX"]
        fault_layer = fault[0]
        fl1 = fault_layer[0]
        nb_cells1 = fl1.getNeighborCells()
        true_nb_cells1 = [(nx - 1, -1), (2 * nx - 1, -1), (3 * nx - 1, -1)]
        self.assertListEqual(nb_cells1, true_nb_cells1)

    def test_polyline_intersection(self):
        grid = GridGenerator.create_rectangular((100, 100, 10), (0.25, 0.25, 1))

        #    Fault1                    Fault4
        #      |                         |
        #      |                         |
        #      |                         |
        #      |   -------  Fault2       |
        #      |                         |
        #      |                         |
        #                              (5 , 2.50)
        #          -------- Fault3
        #

        fault1 = Fault(grid, "Fault1")
        fault2 = Fault(grid, "Fault2")
        fault3 = Fault(grid, "Fault3")
        fault4 = Fault(grid, "Fault4")

        fault1.add_record(1, 1, 10, grid.get_ny() - 1, 0, 0, "X")
        fault2.add_record(5, 10, 15, 15, 0, 0, "Y")
        fault3.add_record(5, 10, 5, 5, 0, 0, "Y")
        fault4.add_record(20, 20, 10, grid.get_ny() - 1, 0, 0, "X")

        polyline = Polyline(init_points=[(4, 4), (8, 4)])
        self.assertTrue(fault4.intersectsPolyline(polyline, 0))

        cpolyline = CPolyline(init_points=[(4, 4), (8, 4)])
        self.assertTrue(fault4.intersectsPolyline(cpolyline, 0))

        polyline = Polyline(init_points=[(8, 4), (16, 4)])
        self.assertFalse(fault4.intersectsPolyline(polyline, 0))

        cpolyline = CPolyline(init_points=[(8, 4), (16, 4)])
        self.assertFalse(fault4.intersectsPolyline(cpolyline, 0))

    def test_num_linesegment(self):
        nx = 10
        ny = 10
        nz = 1
        grid = GridGenerator.create_rectangular((nx, ny, nz), (1, 1, 1))
        with TestAreaContext("python/faults/line_order"):
            with open("faults.grdecl", "w") as f:
                f.write(
                    """FAULTS
\'F1\'              1    4       2    2       1    1    \'Y\'    /
\'F1\'              6    8       2    2       1    1    \'Y\'    /
\'F2\'              1    8       2    2       1    1    \'Y\'    /
/
"""
                )
            faults = FaultCollection(grid, "faults.grdecl")

            f1 = faults["F1"]
            f2 = faults["F2"]
            self.assertEqual(2, f1.numLines(0))
            self.assertEqual(1, f2.numLines(0))

    def test_extend_to_polyline(self):
        grid = GridGenerator.create_rectangular((3, 3, 1), (1, 1, 1))

        #  o   o   o   o
        #
        #  o---o---o---o
        #
        #  o===+   o   o
        #  |
        #  o   o   o   o

        fault1 = Fault(grid, "Fault")
        fault2 = Fault(grid, "Fault2")

        fault1.add_record(0, 0, 0, 0, 0, 0, "X-")
        fault1.add_record(0, 0, 0, 0, 0, 0, "Y")

        fault2.add_record(0, 2, 1, 1, 0, 0, "Y")

        polyline = CPolyline(init_points=[(0, 2), (3, 2)])

        points = fault1.extend_to_polyline(polyline, 0)
        assert points == [(1, 1), (2, 2)]

        end_join = fault1.end_join(polyline, 0)
        end_join2 = fault1.end_join(fault2, 0)
        assert end_join == [(1, 1), (0, 2)]
        assert end_join2 == [(1, 1), (0, 2)]

        polyline2 = CPolyline(init_points=[(0.8, 2), (0.8, 0.8)])
        end_join = fault1.end_join(polyline2, 0)
        assert not end_join

    def test_extend_polyline_on(self):
        grid = GridGenerator.create_rectangular((3, 3, 1), (1, 1, 1))

        #  o   o   o   o
        #
        #  o---o---o---o
        #
        #  o===o===o===o
        #
        #  o   o   o   o

        fault1 = Fault(grid, "Fault")
        fault1.add_record(0, 2, 0, 0, 0, 0, "Y")

        polyline0 = CPolyline(init_points=[(0, 2)])
        polyline1 = CPolyline(init_points=[(0, 2), (3, 2)])
        polyline2 = CPolyline(init_points=[(1, 3), (1, 2)])
        polyline3 = CPolyline(init_points=[(1, 3), (1, 0)])

        with self.assertRaises(ValueError):
            fault1.extendPolylineOnto(polyline0, 0)

        points = fault1.extendPolylineOnto(polyline1, 0)
        self.assertIsNone(points)

        points = fault1.extendPolylineOnto(polyline2, 0)
        self.assertEqual(points, [(1, 2), (1, 1)])

        points = fault1.extendPolylineOnto(polyline3, 0)
        self.assertIsNone(points)

    def test_stepped(self):
        grid = GridGenerator.create_rectangular((6, 1, 4), (1, 1, 1))
        f = Fault(grid, "F")
        f.addRecord(4, 4, 0, 0, 0, 1, "X")
        f.addRecord(2, 2, 0, 0, 1, 1, "Z")
        f.addRecord(1, 1, 0, 0, 2, 3, "X")

        block_kw = ResdataKW("FAULTBLK", grid.get_global_size(), ResDataType.RD_INT)
        block_kw.assign(1)
        block_kw[5] = 2
        block_kw[11] = 2
        block_kw[14:18] = 2
        block_kw[14:18] = 2
        block_kw[20:23] = 2

        layer0 = FaultBlockLayer(grid, 0)
        layer0.scanKeyword(block_kw)
        layer0.addFaultBarrier(f)
        self.assertTrue(layer0.cellContact((0, 0), (1, 0)))
        self.assertFalse(layer0.cellContact((4, 0), (5, 0)))

        layer1 = FaultBlockLayer(grid, 1)
        layer1.scanKeyword(block_kw)
        layer1.addFaultBarrier(f)
        self.assertTrue(layer1.cellContact((0, 0), (1, 0)))
        self.assertFalse(layer1.cellContact((4, 0), (5, 0)))

        layer2 = FaultBlockLayer(grid, 2)
        layer2.scanKeyword(block_kw)
        layer2.addFaultBarrier(f)
        self.assertTrue(layer2.cellContact((0, 0), (1, 0)))
        self.assertFalse(layer2.cellContact((1, 0), (2, 0)))

        layer3 = FaultBlockLayer(grid, 3)
        layer3.scanKeyword(block_kw)
        layer3.addFaultBarrier(f)
        self.assertTrue(layer3.cellContact((0, 0), (1, 0)))
        self.assertFalse(layer3.cellContact((1, 0), (2, 0)))

    def test_connectWithPolyline(self):
        grid = GridGenerator.create_rectangular((4, 4, 1), (1, 1, 1))

        #  o   o   o   o   o
        #
        #  o   o   o   o   o
        #
        #  o---o---o---o---o
        #
        #  o   o   o   o   o
        #          |
        #  o   o   o   o   o

        fault1 = Fault(grid, "Fault1")
        fault1.addRecord(0, 3, 1, 1, 0, 0, "Y")

        fault2 = Fault(grid, "Fault2")
        fault2.addRecord(1, 1, 0, 0, 0, 0, "X")

        fault3 = Fault(grid, "Fault3")
        fault3.addRecord(1, 1, 0, 2, 0, 0, "X")

        self.assertIsNone(fault3.connect(fault1, 0))
        self.assertIsNone(fault3.connect(fault1.getPolyline(0), 0))
        self.assertIsNone(fault3.connect(Polyline(init_points=[(4, 4), (1, 2)]), 0))

        intersect = fault2.connect(fault1, 0)
        intersect2 = fault2.connect(Polyline(init_points=[(0, 0), (1, 2)]), 0)
        self.assertEqual(len(intersect), 2)
        self.assertEqual(len(intersect2), 2)
        p1 = intersect[0]
        p2 = intersect[1]

        self.assertEqual(p1, (2, 1))
        self.assertEqual(p2, (2, 2))

    def test_extend_to_bbox(self):
        bbox = [(0, 0), (10, 0), (10, 10), (0, 10)]
        grid = GridGenerator.create_rectangular((4, 4, 1), (1, 1, 1))
        fault1 = Fault(grid, "Fault1")
        fault2 = Fault(grid, "Fault2")
        fault1.add_record(0, 3, 1, 1, 0, 0, "Y")
        fault2.add_record(1, 1, 0, 1, 0, 0, "X")

        with pytest.raises(
            Exception, match="Logical error - must intersect with bounding box"
        ):
            fault1.extend_to_b_box(bbox, 0, start=True)

        line1 = fault1.extend_to_b_box(bbox, 0, start=False)
        assert line1.getName() == "Extend:Fault1"
        assert line1 == CPolyline(init_points=[(4, 2), (10, 2)])

        line2 = fault2.extend_to_b_box(bbox, 0, start=True)
        assert line2.getName() == "Extend:Fault2"
        assert line2 == CPolyline(init_points=[(2, 0), (2, 0)])
