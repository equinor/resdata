#!/usr/bin/env python

from resdata import ResDataType
from resdata.grid import Grid
from resdata.resfile import Resdata3DKW, ResdataKW
from resdata.util.util import IntVector

from tests import ResdataTest


class Resdata3DKWTest(ResdataTest):
    def test_create(self):
        actnum = IntVector(default_value=1, initial_size=1000)
        for i in range(100):
            actnum[i] = 0

        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1), actnum=actnum)
        kw = Resdata3DKW("KW", grid, ResDataType.RD_FLOAT)
        self.assertEqual(len(kw), grid.getNumActive())

        self.assertEqual((10, 10, 10), kw.dims())

    def test_create_global_size(self):
        actnum = IntVector(default_value=1, initial_size=1000)
        for i in range(100):
            actnum[i] = 0

        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1), actnum=actnum)
        kw = Resdata3DKW("KW", grid, ResDataType.RD_FLOAT, global_active=True)
        self.assertEqual(len(kw), grid.getGlobalSize())

        kw.assign(50)
        self.assertEqual(kw[0, 0, 0], 50)

        kw[0, 0, 0] = 45
        self.assertEqual(kw[0, 0, 0], 45)

    def test_fix_uninitialized(self):
        nx = 10
        ny = 11
        nz = 12
        grid = Grid.createRectangular((nx, ny, nz), (1, 1, 1))
        kw = Resdata3DKW("REGIONS", grid, ResDataType.RD_INT, global_active=True)
        kw.assign(3)
        self.assertEqual(3 * nx * ny * nz, sum(kw))

        kw[1, 1, 1] = 0
        kw[3, 3, 3] = 0
        kw[6, 6, 6] = 0

        self.assertEqual(3 * nx * ny * nz - 9, sum(kw))
        kw.fixUninitialized(grid)
        self.assertEqual(3 * nx * ny * nz, sum(kw))

    def test_getitem(self):
        actnum = IntVector(default_value=1, initial_size=1000)
        for i in range(100):
            actnum[i] = 0

        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1), actnum=actnum)
        kw = Resdata3DKW("KW", grid, ResDataType.RD_FLOAT, default_value=77)

        with self.assertRaises(IndexError):
            kw[1000]

        with self.assertRaises(IndexError):
            kw[0, 10, 100]

        with self.assertRaises(ValueError):
            kw[1, 1]

        with self.assertRaises(ValueError):
            kw[1, 1, 1, 1]

        kw.assign(99)

        self.assertEqual(kw[0, 0, 0], 77)
        self.assertEqual(kw[0, 0, 1], 99)

    def test_setitem(self):
        actnum = IntVector(default_value=1, initial_size=1000)
        for i in range(100):
            actnum[i] = 0

        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1), actnum=actnum)
        kw = Resdata3DKW("KW", grid, ResDataType.RD_FLOAT, default_value=77)

        with self.assertRaises(IndexError):
            kw[1000]

        with self.assertRaises(IndexError):
            kw[0, 10, 100]

        with self.assertRaises(ValueError):
            kw[1, 1]

        with self.assertRaises(ValueError):
            kw[1, 1, 1, 1]

        kw.assign(99)
        self.assertEqual(kw[0, 0, 0], 77)
        self.assertEqual(kw[0, 0, 1], 99)

        with self.assertRaises(ValueError):
            kw[0, 0, 0] = 88

        kw[0, 0, 1] = 100
        self.assertEqual(kw[0, 0, 1], 100)

    def test_cast(self):
        actnum = IntVector(default_value=1, initial_size=1000)
        for i in range(100):
            actnum[i] = 0

        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1), actnum=actnum)
        kw_wrong_size = ResdataKW("KW", 27, ResDataType.RD_FLOAT)
        kw_global_size = ResdataKW("KW", grid.getGlobalSize(), ResDataType.RD_FLOAT)
        kw_active_size = ResdataKW("KW", grid.getNumActive(), ResDataType.RD_FLOAT)

        with self.assertRaises(ValueError):
            Resdata3DKW.castFromKW(kw_wrong_size, grid)

        Resdata3DKW.castFromKW(kw_global_size, grid)
        self.assertTrue(isinstance(kw_global_size, Resdata3DKW))

        Resdata3DKW.castFromKW(kw_active_size, grid, default_value=66)
        self.assertTrue(isinstance(kw_active_size, Resdata3DKW))

        self.assertEqual(kw_active_size[0, 0, 0], 66)
        with self.assertRaises(ValueError):
            kw_active_size[0, 0, 0] = 88

    def test_default(self):
        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1))
        kw = Resdata3DKW("KW", grid, ResDataType.RD_FLOAT)
        kw.setDefault(55)
        self.assertTrue(55, kw.getDefault())

    def test_compressed_copy(self):
        actnum = IntVector(default_value=1, initial_size=1000)
        for i in range(500):
            actnum[2 * i + 1] = 0

        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1), actnum=actnum)
        kw = Resdata3DKW("KW", grid, ResDataType.RD_INT, global_active=True)
        for i in range(len(kw)):
            kw[i] = i

        kw_copy = kw.compressedCopy()
        self.assertTrue(isinstance(kw_copy, ResdataKW))

        self.assertEqual(len(kw_copy), 500)
        for i in range(len(kw_copy)):
            self.assertEqual(kw_copy[i], 2 * i)

    def test_global_copy(self):
        actnum = IntVector(default_value=1, initial_size=1000)
        for i in range(500):
            actnum[2 * i + 1] = 0

        grid = Grid.createRectangular((10, 10, 10), (1, 1, 1), actnum=actnum)
        kw = Resdata3DKW("KW", grid, ResDataType.RD_INT, global_active=False)
        for i in range(len(kw)):
            kw[i] = i

        kw.setDefault(177)
        kw_copy = kw.globalCopy()
        self.assertTrue(isinstance(kw_copy, ResdataKW))

        self.assertEqual(len(kw_copy), 1000)
        for i in range(len(kw)):
            self.assertEqual(kw_copy[2 * i], i)
            self.assertEqual(kw_copy[2 * i + 1], kw.getDefault())
