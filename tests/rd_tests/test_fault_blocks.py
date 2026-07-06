#!/usr/bin/env python
import gc

import cwrap
import pytest
from resdata import ResDataType
from resdata.geometry import CPolylineCollection, Polyline
from resdata.grid import Grid, GridGenerator, ResdataRegion
from resdata.grid.faults import Fault, FaultBlock, FaultBlockLayer, FaultCollection
from resdata.resfile import ResdataKW

from tests import ResdataTest
from tests.util import TestAreaContext


class FaultBlockTest(ResdataTest):
    def setUp(self):
        self.grid = GridGenerator.create_rectangular((10, 10, 10), (1, 1, 1))
        self.kw = ResdataKW("FAULTBLK", self.grid.get_global_size(), ResDataType.RD_INT)
        self.kw.assign(1)

        reg = ResdataRegion(self.grid, False)

        for k in range(self.grid.get_nz()):
            reg.clear()
            reg.select_kslice(k, k)
            self.kw.assign(k, mask=reg)
            self.kw[k * self.grid.get_nx() * self.grid.get_ny() + 7] = 177

    def test_fault_block(self):
        grid = GridGenerator.create_rectangular((5, 5, 1), (1, 1, 1))
        kw = ResdataKW("FAULTBLK", grid.get_global_size(), ResDataType.RD_INT)
        kw.assign(0)
        for j in range(1, 4):
            for i in range(1, 4):
                g = i + j * grid.get_nx()
                kw[g] = 1

        layer = FaultBlockLayer(grid, 0)
        layer.scan_keyword(kw)
        block = layer[1]

        self.assertEqual((2.50, 2.50), block.get_centroid())
        self.assertEqual(len(block), 9)
        self.assertEqual(layer, block.get_parent_layer())

    def test_get_ijk(self):
        with TestAreaContext("python/fault_block_layer/neighbour") as work_area:
            with open("kw.grdecl", "w") as fileH:
                fileH.write("FAULTBLK \n")
                fileH.write("1 1 1 0 0\n")
                fileH.write("1 2 2 0 3\n")
                fileH.write("4 2 2 3 3\n")
                fileH.write("4 4 4 0 0\n")
                fileH.write("4 4 4 0 5\n")
                fileH.write("/\n")
            with cwrap.open("kw.grdecl") as f:
                kw = ResdataKW.read_grdecl(f, "FAULTBLK", rd_type=ResDataType.RD_INT)

        grid = GridGenerator.create_rectangular((5, 5, 1), (1, 1, 1))
        layer = FaultBlockLayer(grid, 0)
        layer.load_keyword(kw)

        block = layer[0, 0]
        self.assertEqual(block.get_block_id(), 1)

        block = layer[2, 2]
        self.assertEqual(block.get_block_id(), 2)

        with self.assertRaises(ValueError):
            layer[3, 3]

        with self.assertRaises(IndexError):
            layer[5, 5]

    def test_neighbours(self):
        with TestAreaContext("python/fault_block_layer/neighbour") as work_area:
            with open("kw.grdecl", "w") as fileH:
                fileH.write("FAULTBLK \n")
                fileH.write("1 1 1 0 0\n")
                fileH.write("1 2 2 0 3\n")
                fileH.write("4 2 2 3 3\n")
                fileH.write("4 4 4 0 0\n")
                fileH.write("4 4 4 0 5\n")
                fileH.write("/\n")

            with cwrap.open("kw.grdecl") as f:
                kw = ResdataKW.read_grdecl(f, "FAULTBLK", rd_type=ResDataType.RD_INT)

        grid = GridGenerator.create_rectangular((5, 5, 1), (1, 1, 1))
        layer = FaultBlockLayer(grid, 0)

        layer.load_keyword(kw)
        block1 = layer.get_block(1)
        block2 = layer.get_block(2)
        block3 = layer.get_block(3)
        block4 = layer.get_block(4)
        block5 = layer.get_block(5)
        self.assertEqual(block1.get_parent_layer(), layer)

        # Expected: 1 -> {2,4}, 2 -> {1,3,4}, 3 -> {2}, 4 -> {1,2}, 5-> {}

        neighbours = block1.get_neighbours()
        self.assertEqual(len(neighbours), 2)
        self.assertTrue(block2 in neighbours)
        self.assertTrue(block4 in neighbours)

        neighbours = block2.get_neighbours()
        self.assertEqual(len(neighbours), 3)
        self.assertTrue(block1 in neighbours)
        self.assertTrue(block3 in neighbours)
        self.assertTrue(block4 in neighbours)

        neighbours = block3.get_neighbours()
        self.assertEqual(len(neighbours), 1)
        self.assertTrue(block2 in neighbours)

        neighbours = block4.get_neighbours()
        self.assertEqual(len(neighbours), 2)
        self.assertTrue(block1 in neighbours)
        self.assertTrue(block2 in neighbours)

        neighbours = block5.get_neighbours()
        self.assertEqual(len(neighbours), 0)

    def test_neighbours2(self):
        nx = 8
        ny = 8
        nz = 1
        grid = GridGenerator.create_rectangular((nx, ny, nz), (1, 1, 1))
        layer = FaultBlockLayer(grid, 0)
        with TestAreaContext("python/FaultBlocks/neighbours"):
            with open("faultblock.grdecl", "w") as fileH:
                fileH.write("FAULTBLK \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("3 3 3 3 2 2 2 2 \n")
                fileH.write("3 3 3 3 2 2 2 2 \n")
                fileH.write("3 3 3 3 2 2 2 2 \n")
                fileH.write("3 3 3 3 2 2 2 2 \n")
                fileH.write("/\n")

            with cwrap.open("faultblock.grdecl") as f:
                kw = ResdataKW.read_grdecl(f, "FAULTBLK", rd_type=ResDataType.RD_INT)

            with open("faults.grdecl", "w") as f:
                f.write("FAULTS\n")
                f.write("'FY'   1   4   4   4   1   1  'Y'  /\n")
                f.write("'FX'   4   4   1   8   1   1  'X'  /\n")
                f.write("'FXX'   6   6   1   8   1   1  'X'  /\n")
                f.write("/")

            faults = FaultCollection(grid, "faults.grdecl")
        layer.load_keyword(kw)
        b1 = layer.get_block(1)
        b2 = layer.get_block(2)
        b3 = layer.get_block(3)

        nb = b1.get_neighbours()
        self.assertTrue(b2 in nb)
        self.assertTrue(b3 in nb)

        polylines1 = CPolylineCollection()
        p1 = polylines1.createPolyline(name="P1")
        p1.addPoint(4, 0)
        p1.addPoint(4, 4)
        p1.addPoint(4, 8)
        nb = b1.get_neighbours(polylines=polylines1)
        self.assertFalse(b2 in nb)
        self.assertTrue(b3 in nb)

        polylines2 = CPolylineCollection()
        p1 = polylines2.createPolyline(name="P2")
        p1.addPoint(0, 4)
        p1.addPoint(4, 4)
        nb = b1.get_neighbours(polylines=polylines2)
        self.assertTrue(b2 in nb)
        self.assertFalse(b3 in nb)

        layer.add_fault_barrier(faults["FY"])
        nb = b1.get_neighbours()
        self.assertTrue(b2 in nb)
        self.assertFalse(b3 in nb)

        layer.add_fault_barrier(faults["FX"])
        nb = b1.get_neighbours()
        self.assertEqual(len(nb), 0)

        layer.join_faults(faults["FX"], faults["FY"])
        with pytest.raises(ValueError, match="Failed to join faults FXX and FY"):
            layer.join_faults(faults["FXX"], faults["FY"])

    def test_neighbours3(self):
        nx = 8
        ny = 8
        nz = 1
        grid = GridGenerator.create_rectangular((nx, ny, nz), (1, 1, 1))
        layer = FaultBlockLayer(grid, 0)
        with TestAreaContext("python/FaultBlocks/neighbours"):
            with open("faultblock.grdecl", "w") as fileH:
                fileH.write("FAULTBLK \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 1 2 2 2 \n")
                fileH.write("1 1 1 1 1 2 2 2 \n")
                fileH.write("1 1 1 1 1 2 2 2 \n")
                fileH.write("1 1 1 1 1 2 2 2 \n")
                fileH.write("/\n")

            with cwrap.open("faultblock.grdecl") as f:
                kw = ResdataKW.read_grdecl(f, "FAULTBLK", rd_type=ResDataType.RD_INT)
            with open("faults.grdecl", "w") as f:
                f.write("FAULTS\n")
                f.write("'FX'   4   4   1   4   1   1  'X'  /\n")
                f.write("'FX'   5   5   5   8   1   1  'X'  /\n")
                f.write("/")

            faults = FaultCollection(grid, "faults.grdecl")
        layer.load_keyword(kw)
        b1 = layer.get_block(1)
        b2 = layer.get_block(2)

        nb = b1.get_neighbours()
        self.assertTrue(b2 in nb)

        layer.add_fault_barrier(faults["FX"], link_segments=False)
        nb = b1.get_neighbours()
        self.assertTrue(b2 in nb)

    def test_fault_block_edge(self):
        grid = GridGenerator.create_rectangular((5, 5, 1), (1, 1, 1))
        kw = ResdataKW("FAULTBLK", grid.get_global_size(), ResDataType.RD_INT)
        kw.assign(0)
        for j in range(1, 4):
            for i in range(1, 4):
                g = i + j * grid.get_nx()
                kw[g] = 1

        layer = FaultBlockLayer(grid, 0)
        # with self.assertRaises:
        #    layer.getEdgePolygon( )

    def test_fault_block_layer(self):
        with self.assertRaises(ValueError):
            layer = FaultBlockLayer(self.grid, -1)

        with self.assertRaises(ValueError):
            layer = FaultBlockLayer(self.grid, self.grid.get_global_size())

        layer = FaultBlockLayer(self.grid, 1)
        self.assertEqual(1, layer.get_k())

        kw = ResdataKW("FAULTBLK", self.grid.get_global_size(), ResDataType.RD_FLOAT)
        with self.assertRaises(ValueError):
            layer.scan_keyword(kw)

        layer.scan_keyword(self.kw)
        self.assertEqual(2, len(layer))

        with self.assertRaises(TypeError):
            ls = layer["JJ"]

        l = []
        for blk in layer:
            l.append(blk)
        self.assertEqual(len(l), 2)

        l0 = layer[0]
        l1 = layer[1]
        self.assertTrue(isinstance(l1, FaultBlock))
        assert l1[0].i == 7
        assert l1[0].j == 0
        l0.get_centroid()
        l1.get_block_id()
        assert list(l1.get_region_list()) == []

        with self.assertRaises(IndexError):
            l2 = layer[2]

        self.assertEqual(True, 1 in layer)
        self.assertEqual(True, 2 in layer)
        self.assertEqual(False, 77 in layer)
        self.assertEqual(False, 177 in layer)

        l1 = layer.get_block(1)
        self.assertTrue(isinstance(l1, FaultBlock))
        l1.add_cell(9, 9)
        assert len(l1.get_global_index_list()) == len(l1)
        polyline = Polyline(init_points=[(1.0, 0.0), (2.0, 1.0)])
        assert l1.contains_polyline(polyline)

        polyline2 = Polyline(init_points=[(10.5, 1.0), (11, 5)])
        assert not l1.contains_polyline(polyline2)

        with self.assertRaises(KeyError):
            l = layer.get_block(66)

        with self.assertRaises(KeyError):
            layer.delete_block(66)

        layer.delete_block(2)
        self.assertEqual(1, len(layer))
        blk = layer[0]
        self.assertEqual(blk.get_block_id(), 1)

        with self.assertRaises(KeyError):
            layer.add_block(1)

        blk2 = layer.add_block(2)
        self.assertEqual(len(layer), 2)

        blk3 = layer.add_block()
        self.assertEqual(len(layer), 3)

        layer.add_block(100)
        layer.add_block(101)
        layer.add_block(102)
        layer.add_block(103)

        layer.delete_block(2)
        blk1 = layer.get_block(103)
        blk2 = layer[-1]
        self.assertEqual(blk1.get_block_id(), blk2.get_block_id())

        fault_block = layer[0]
        fault_block.assign_to_region(2)
        self.assertEqual([2], list(fault_block.get_region_list()))

        fault_block.assign_to_region(2)
        self.assertEqual([2], list(fault_block.get_region_list()))

        fault_block.assign_to_region(3)
        self.assertEqual([2, 3], list(fault_block.get_region_list()))

        fault_block.assign_to_region(1)
        self.assertEqual([1, 2, 3], list(fault_block.get_region_list()))

        fault_block.assign_to_region(2)
        self.assertEqual([1, 2, 3], list(fault_block.get_region_list()))

    def test_add_polyline_barrier1(self):
        grid = GridGenerator.create_rectangular((4, 1, 1), (1, 1, 1))
        layer = FaultBlockLayer(self.grid, 0)
        polyline = Polyline(init_points=[(1.99, 0.001), (2.01, 0.99)])

        points = [((1, 0), (2, 0))]

        geo_layer = layer.get_geo_layer()
        for p1, p2 in points:
            self.assertTrue(geo_layer.cell_contact(p1, p2))

        layer.add_polyline_barrier(polyline)
        for p1, p2 in points:
            print(p1, p2)
            self.assertFalse(geo_layer.cell_contact(p1, p2))

    def test_add_polyline_barrier2(self):
        grid = GridGenerator.create_rectangular((10, 10, 1), (1, 1, 1))
        layer = FaultBlockLayer(self.grid, 0)
        polyline = Polyline(init_points=[(0.1, 0.9), (8.9, 0.9), (8.9, 8.9)])

        points = [
            ((0, 0), (0, 1)),
            ((2, 0), (2, 1)),
            ((4, 0), (4, 1)),
            ((6, 0), (6, 1)),
            ((8, 0), (8, 1)),
            #
            ((8, 1), (9, 1)),
            ((8, 3), (9, 3)),
            ((8, 5), (9, 5)),
            ((8, 7), (9, 7)),
        ]

        geo_layer = layer.get_geo_layer()
        for p1, p2 in points:
            self.assertTrue(geo_layer.cell_contact(p1, p2))

        layer.add_polyline_barrier(polyline)
        for p1, p2 in points:
            print(p1, p2)
            self.assertFalse(geo_layer.cell_contact(p1, p2))

    def test_fault_block_layer_export(self):
        layer = FaultBlockLayer(self.grid, 1)
        kw1 = ResdataKW("FAULTBLK", self.grid.get_global_size() + 1, ResDataType.RD_INT)
        with self.assertRaises(ValueError):
            layer.export_keyword(kw1)

        kw2 = ResdataKW("FAULTBLK", self.grid.get_global_size(), ResDataType.RD_FLOAT)
        with self.assertRaises(TypeError):
            layer.export_keyword(kw2)

    def test_internal_blocks(self):
        nx = 8
        ny = 8
        nz = 1
        grid = GridGenerator.create_rectangular((nx, ny, nz), (1, 1, 1))
        layer = FaultBlockLayer(grid, 0)
        with TestAreaContext("python/FaultBlocks/internal_blocks"):
            with open("faultblock.grdecl", "w") as fileH:
                fileH.write("FAULTBLK \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 4 4 1 2 5 5 2 \n")
                fileH.write("1 4 4 1 2 5 5 2 \n")
                fileH.write("1 1 1 1 2 2 2 2 \n")
                fileH.write("1 1 1 1 1 2 2 2 \n")
                fileH.write("1 1 3 1 1 2 2 2 \n")
                fileH.write("1 1 1 1 1 2 2 2 \n")
                fileH.write("1 1 1 1 1 2 2 2 \n")
                fileH.write("/\n")

            with cwrap.open("faultblock.grdecl") as f:
                kw = ResdataKW.read_grdecl(f, "FAULTBLK", rd_type=ResDataType.RD_INT)

            with open("faults.grdecl", "w") as f:
                f.write("FAULTS\n")
                f.write("'FX'   4   4   1   4   1   1  'X'  /\n")
                f.write("'FX'   5   5   4   4   1   1  'Y'  /\n")
                f.write("'FX'   5   5   5   8   1   1  'X'  /\n")
                f.write("/")

            faults = FaultCollection(grid, "faults.grdecl")

        layer.load_keyword(kw)
        faulty_kw = ResdataKW("SOIL", 10000, ResDataType.RD_INT)
        with pytest.raises(
            ValueError, match="The fault block keyword had wrong type/size"
        ):
            layer.load_keyword(faulty_kw)
        layer.add_fault_barrier(faults["FX"])
        b1 = layer.get_block(1)
        b2 = layer.get_block(2)
        b3 = layer.get_block(3)
        b4 = layer.get_block(4)
        b5 = layer.get_block(5)

        nb = b1.get_neighbours()
        for b in nb:
            print("Block:%d" % b.get_block_id())

        self.assertTrue(len(nb) == 2)
        self.assertTrue(b3 in nb)
        self.assertTrue(b4 in nb)

        nb = b2.get_neighbours()
        self.assertTrue(len(nb) == 1)
        self.assertTrue(b5 in nb)

    def test_fault_block_str(self):
        grid = GridGenerator.create_rectangular((3, 3, 1), (1, 1, 1))
        kw = ResdataKW("FAULTBLK", grid.get_global_size(), ResDataType.RD_INT)
        kw.assign(7)
        layer = FaultBlockLayer(grid, 0)
        layer.scan_keyword(kw)
        block = layer[0]
        self.assertEqual(str(block), "Block ID: %d" % block.get_block_id())


def test_that_get_geo_layer_does_not_return_dangling_pointer():
    """This is a regression test for a bug where get_geo_layer
    did not call setParent on the returned pointer"""
    grid = GridGenerator.create_rectangular((16, 16, 1), (1, 1, 1))
    fault_block_layer = FaultBlockLayer(grid, 0)

    layer = fault_block_layer.get_geo_layer()
    assert layer.get_nx() == 16

    # Simulate fault_block_layer going out of scope and
    # getting garbage collected
    del fault_block_layer
    gc.collect()

    assert layer.get_nx() == 16
