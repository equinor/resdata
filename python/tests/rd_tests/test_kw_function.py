#!/usr/bin/env python
import os
import random
from resdata import ResDataType
from resdata.resfile import ResdataKW, Resdata3DKW
from resdata.grid import Grid
from resdata.util.util import IntVector
from tests import ResdataTest


class KWFunctionTest(ResdataTest):
    def test_region_filter(self):
        nx = 10
        ny = 10
        nz = 1
        actnum = IntVector(initial_size=nx * ny * nz, default_value=1)
        actnum[nx * ny - 1] = 0

        grid = Grid.createRectangular((nx, ny, nz), (1, 1, 1), actnum=actnum)
        self.assertEqual(grid.getNumActive(), nx * ny * nz - 1)

        kw = Resdata3DKW.create("REGIONS", grid, ResDataType.RD_INT, global_active=True)
        kw.assign(0)
        kw[0 : int(nx * ny / 2)] = 1
        kw[5, 2, 0] = 0
        kw[0, 9, 0] = 2

        kw.fixUninitialized(grid)

        # Not assigned because they are in contact with a '2'; these
        # two are problem cells.
        self.assertEqual(kw[0, ny - 2, 0], 0)
        self.assertEqual(kw[1, ny - 1, 0], 0)

        # Not assigned because it is inactive
        self.assertEqual(kw[nx - 1, ny - 1, 0], 0)

        self.assertEqual(kw[5, 2, 0], 1)
        for j in range(5, 10):
            self.assertEqual(kw[5, j, 0], 1)

        for i in range(10):
            self.assertEqual(kw[i, 7, 0], 1)

    def test_porv_kw(self):
        porv_int = ResdataKW("PORV", 100, ResDataType.RD_INT)
        with self.assertRaises(TypeError):
            actnum = porv_int.create_actnum()

        prv = ResdataKW("PRV", 100, ResDataType.RD_FLOAT)
        with self.assertRaises(ValueError):
            actnum = prv.create_actnum()

        porv = ResdataKW("PORV", 4, ResDataType.RD_FLOAT)
        porv[0] = 0
        porv[1] = 0.50
        porv[2] = 0.50
        porv[3] = 0
        actnum = porv.create_actnum()
        self.assertEqual(tuple(actnum), (0, 1, 1, 0))
