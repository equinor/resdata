#!/usr/bin/env python
from resdata.resfile import ResdataFile
from resdata.grid import Grid, ResdataRegion
from resdata.grid.faults import Layer
from tests import ResdataTest, equinor_test


@equinor_test()
class RegionTest(ResdataTest):
    def setUp(self):
        case = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE")
        self.grid = Grid(case)
        self.rst_file = ResdataFile("%s.UNRST" % case)
        self.init_file = ResdataFile("%s.INIT" % case)

    def test_kw_imul(self):
        P = self.rst_file["PRESSURE"][5]
        fipnum = self.init_file["FIPNUM"][0]
        fipnum_copy = fipnum.deep_copy()

        reg = ResdataRegion(self.grid, False)
        reg.select_more(P, 260)
        fipnum.mul(-1, mask=reg)
        self.assertFalse(fipnum.equal(fipnum_copy))

        fipnum.mul(-1, mask=reg)
        self.assertTrue(fipnum.equal(fipnum_copy))

    def test_equal(self):
        reg1 = ResdataRegion(self.grid, False)
        reg2 = ResdataRegion(self.grid, False)

        self.assertTrue(reg1 == reg2)

        reg1.select_islice(4, 6)
        self.assertFalse(reg1 == reg2)
        reg2.select_islice(4, 7)
        self.assertFalse(reg1 == reg2)
        reg1.select_islice(7, 7)
        self.assertTrue(reg1 == reg2)

    def test_kw_idiv(self):
        P = self.rst_file["PRESSURE"][5]
        fipnum = self.init_file["FIPNUM"][0]
        fipnum_copy = fipnum.deep_copy()

        reg = ResdataRegion(self.grid, False)
        reg.select_more(P, 260)
        fipnum.div(-1, mask=reg)
        self.assertFalse(fipnum.equal(fipnum_copy))

        fipnum.div(-1, mask=reg)
        self.assertTrue(fipnum.equal(fipnum_copy))

    def test_kw_iadd(self):
        P = self.rst_file["PRESSURE"][5]
        fipnum = self.init_file["FIPNUM"][0]
        fipnum_copy = fipnum.deep_copy()

        reg = ResdataRegion(self.grid, False)
        reg.select_more(P, 260)
        fipnum.add(1, mask=reg)
        self.assertFalse(fipnum.equal(fipnum_copy))

        reg.invert()
        fipnum.add(1, mask=reg)

        fipnum.sub(1)
        self.assertTrue(fipnum.equal(fipnum_copy))

    def test_kw_isub(self):
        P = self.rst_file["PRESSURE"][5]
        fipnum = self.init_file["FIPNUM"][0]
        fipnum_copy = fipnum.deep_copy()

        reg = ResdataRegion(self.grid, False)
        reg.select_more(P, 260)
        fipnum.sub(1, mask=reg)
        self.assertFalse(fipnum.equal(fipnum_copy))
        fipnum.add(1, mask=reg)
        self.assertTrue(fipnum.equal(fipnum_copy))

    def test_slice(self):
        reg = ResdataRegion(self.grid, False)
        reg.select_islice(0, 5)
        OK = True

        global_list = reg.get_global_list()
        self.assertEqual(global_list.parent(), reg)

        for gi in global_list:
            i, j, k = self.grid.get_ijk(global_index=gi)
            if i > 5:
                OK = False
        self.assertTrue(OK)
        self.assertTrue(
            self.grid.get_ny() * self.grid.get_nz() * 6 == len(reg.get_global_list())
        )

        reg.select_jslice(7, 8, intersect=True)
        OK = True
        for gi in reg.get_global_list():
            i, j, k = self.grid.get_ijk(global_index=gi)
            if i > 5:
                OK = False

            if j < 7 or j > 8:
                OK = False

        self.assertTrue(OK)
        self.assertTrue(2 * self.grid.get_nz() * 6 == len(reg.get_global_list()))

        reg2 = ResdataRegion(self.grid, False)
        reg2.select_kslice(3, 5)
        reg &= reg2
        OK = True
        for gi in reg.get_global_list():
            i, j, k = self.grid.get_ijk(global_index=gi)
            if i > 5:
                OK = False

            if j < 7 or j > 8:
                OK = False

            if k < 3 or k > 5:
                OK = False

        self.assertTrue(OK)
        self.assertTrue(2 * 3 * 6 == len(reg.get_global_list()))

    def test_index_list(self):
        reg = ResdataRegion(self.grid, False)
        reg.select_islice(0, 5)
        active_list = reg.get_active_list()
        global_list = reg.get_global_list()

    def test_polygon(self):
        reg = ResdataRegion(self.grid, False)
        x, y, z = self.grid.get_xyz(ijk=(10, 10, 0))
        dx = 0.1
        dy = 0.1
        reg.select_inside_polygon(
            [(x - dx, y - dy), (x - dx, y + dy), (x + dx, y + dy), (x + dx, y - dy)]
        )
        self.assertTrue(self.grid.get_nz() == len(reg.get_global_list()))

    def test_heidrun(self):
        root = self.createTestPath("Equinor/ECLIPSE/Heidrun")
        grid = Grid("%s/FF12_2013B2_AMAP_AOP-J15_NO62_MOVEX.EGRID" % root)

        polygon = []
        with open("%s/polygon.ply" % root) as fileH:
            for line in fileH:
                tmp = line.split()
                polygon.append((float(tmp[0]), float(tmp[1])))
        self.assertEqual(len(polygon), 11)

        reg = ResdataRegion(grid, False)
        reg.select_inside_polygon(polygon)
        self.assertEqual(0, len(reg.get_global_list()) % grid.get_nz())

    def test_layer(self):
        region = ResdataRegion(self.grid, False)
        layer = Layer(self.grid.get_nx(), self.grid.get_ny() + 1)
        with self.assertRaises(ValueError):
            region.select_from_layer(layer, 0, 1)

        layer = Layer(self.grid.get_nx(), self.grid.get_ny())
        layer[0, 0] = 1
        layer[1, 1] = 1
        layer[2, 2] = 1

        with self.assertRaises(ValueError):
            region.select_from_layer(layer, -1, 1)

        with self.assertRaises(ValueError):
            region.select_from_layer(layer, self.grid.get_nz(), 1)

        region.select_from_layer(layer, 0, 2)
        glist = region.get_global_list()
        self.assertEqual(0, len(glist))

        region.select_from_layer(layer, 0, 1)
        glist = region.get_global_list()
        self.assertEqual(3, len(glist))
