#!/usr/bin/env python
import os
import random
from resdata import ResDataType, FileMode
from resdata.resfile import ResdataKW, ResdataFile, FortIO

from resdata.util.test import TestAreaContext
from tests import ResdataTest, equinor_test


def copy_long():
    src = ResdataKW("NAME", 100, ResDataType.RD_FLOAT)
    copy = src.sub_copy(0, 2000)


def copy_offset():
    src = ResdataKW("NAME", 100, ResDataType.RD_FLOAT)
    copy = src.sub_copy(200, 100)


@equinor_test()
class KWTest(ResdataTest):
    def test_fortio_size(self):
        unrst_file_path = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNRST")
        unrst_file = ResdataFile(unrst_file_path)
        size = 0
        for kw in unrst_file:
            size += kw.fortIOSize()

        stat = os.stat(unrst_file_path)
        self.assertTrue(size == stat.st_size)

    def test_sub_copy(self):
        unrst_file_path = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNRST")
        unrst_file = ResdataFile(unrst_file_path)
        swat = unrst_file["SWAT"][0]

        swat1 = swat.sub_copy(0, -1)
        swat2 = swat.sub_copy(0, len(swat))

        self.assertTrue(swat.equal(swat1))
        self.assertTrue(swat.equal(swat2))

        swat3 = swat.sub_copy(20000, 100, new_header="swat")
        self.assertTrue(swat3.getName() == "swat")
        self.assertTrue(len(swat3) == 100)
        equal = True
        for i in range(len(swat3)):
            if swat3[i] != swat[i + 20000]:
                equal = False
        self.assertTrue(equal)

        self.assertRaises(IndexError, copy_long)
        self.assertRaises(IndexError, copy_offset)

    def test_equal(self):
        kw1 = ResdataKW("TEST", 3, ResDataType.RD_CHAR)
        kw1[0] = "Test1"
        kw1[1] = "Test13"
        kw1[2] = "Test15"

        kw2 = ResdataKW("TEST", 3, ResDataType.RD_CHAR)
        kw2[0] = "Test1"
        kw2[1] = "Test13"
        kw2[2] = "Test15"

        self.assertTrue(kw1.equal(kw2))
        self.assertTrue(kw1.equal_numeric(kw2))

        kw2[2] = "Test15X"
        self.assertFalse(kw1.equal(kw2))
        self.assertFalse(kw1.equal_numeric(kw2))

        unrst_file_path = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNRST")
        unrst_file = ResdataFile(unrst_file_path)
        kw1 = unrst_file["PRESSURE"][0]
        kw2 = kw1.deep_copy()

        self.assertTrue(kw1.equal(kw2))
        self.assertTrue(kw1.equal_numeric(kw2))

        kw2 *= 1.00001
        self.assertFalse(kw1.equal(kw2))
        self.assertFalse(kw1.equal_numeric(kw2, epsilon=1e-8))
        self.assertTrue(kw1.equal_numeric(kw2, epsilon=1e-2))

        kw1 = unrst_file["ICON"][10]
        kw2 = kw1.deep_copy()
        self.assertTrue(kw1.equal(kw2))
        self.assertTrue(kw1.equal_numeric(kw2))

        kw1[-1] += 1
        self.assertFalse(kw1.equal(kw2))
        self.assertFalse(kw1.equal_numeric(kw2))
