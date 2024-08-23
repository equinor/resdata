#!/usr/bin/env python
from __future__ import print_function

import datetime

from resdata.rft import ResdataPLTCell, ResdataRFTCell, ResdataRFTFile, WellTrajectory

from tests import ResdataTest, equinor_test


@equinor_test()
class RFTTest(ResdataTest):
    def setUp(self):
        self.RFT_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.RFT")
        self.PLT_file = self.createTestPath("Equinor/ECLIPSE/RFT/TEST1_1A.RFT")

    def test_RFT_load(self):
        rftFile = ResdataRFTFile(self.RFT_file)

        rft = rftFile[0]
        cell = rft.ijkget((32, 53, 0))
        self.assertIsInstance(cell, ResdataRFTCell)

        self.assertEqual(2, rftFile.size())
        self.assertEqual(0, rftFile.size(well="OP*"))
        self.assertEqual(0, rftFile.size(well="XXX"))
        self.assertEqual(1, rftFile.size(date=datetime.date(2000, 6, 1)))
        self.assertEqual(0, rftFile.size(date=datetime.date(2000, 6, 17)))

        cell = rft.ijkget((30, 20, 1880))
        self.assertIsNone(cell)

        for rft in rftFile:
            self.assertTrue(rft.is_RFT())
            self.assertFalse(rft.is_SEGMENT())
            self.assertFalse(rft.is_PLT())
            self.assertFalse(rft.is_MSW())

            for cell in rft:
                self.assertIsInstance(cell, ResdataRFTCell)

        for h in rftFile.getHeaders():
            print(h)
            self.assertIsInstance(h[1], datetime.date)

    def test_PLT_load(self):
        pltFile = ResdataRFTFile(self.PLT_file)
        plt = pltFile[11]
        self.assertTrue(plt.is_PLT())
        self.assertFalse(plt.is_SEGMENT())
        self.assertFalse(plt.is_RFT())
        self.assertFalse(plt.is_MSW())

        for cell in plt:
            self.assertIsInstance(cell, ResdataPLTCell)

    def test_exceptions(self):
        with self.assertRaises(IndexError):
            rftFile = ResdataRFTFile(self.RFT_file)
            _rft = rftFile[100]

    def test_basics(self):
        wt = WellTrajectory(
            self.createTestPath("Equinor/spotfire/gendata_rft_zone/E-3H.txt")
        )
        self.assertEqual(len(wt), 38)
        self.assertTrue(isinstance(str(wt), str))
        self.assertTrue(isinstance(repr(wt), str))
        self.assertEqual("WellTrajectory(len=38)", repr(wt))

    def test_trajectory(self):
        with self.assertRaises(IOError):
            WellTrajectory("/does/no/exist")

        with self.assertRaises(UserWarning):
            WellTrajectory(
                self.createTestPath(
                    "Equinor/spotfire/gendata_rft_zone/invalid_float.txt"
                )
            )

        with self.assertRaises(UserWarning):
            WellTrajectory(
                self.createTestPath(
                    "Equinor/spotfire/gendata_rft_zone/missing_item.txt"
                )
            )

        wt = WellTrajectory(
            self.createTestPath("Equinor/spotfire/gendata_rft_zone/E-3H.txt")
        )
        self.assertEqual(len(wt), 38)

        with self.assertRaises(IndexError):
            _p = wt[38]

        p0 = wt[0]
        self.assertEqual(p0.utm_x, 458920.671)
        self.assertEqual(p0.utm_y, 7324939.077)
        self.assertEqual(p0.measured_depth, 2707.5000)

        pm1 = wt[-1]
        p37 = wt[37]
        self.assertEqual(p37, pm1)

    def test_PLT(self):
        rft_file = ResdataRFTFile(
            self.createTestPath("Equinor/ECLIPSE/Heidrun/RFT/2C3_MR61.RFT")
        )

        rft0 = rft_file[0]
        rft1 = rft_file[1]
        rft2 = rft_file[2]
        rft3 = rft_file[3]

        self.assertTrue(rft0.is_RFT())
        self.assertTrue(rft1.is_RFT())
        self.assertTrue(rft2.is_PLT())
        self.assertTrue(rft3.is_PLT())

        self.assertEqual(len(rft0), 42)
        self.assertEqual(len(rft1), 37)
        self.assertEqual(len(rft2), 42)
        self.assertEqual(len(rft3), 37)

        self.assertFloatEqual(rft0[0].pressure, 0.22919502e03)
        self.assertFloatEqual(rft0[0].depth, 0.21383721e04)

        self.assertFloatEqual(rft1[0].pressure, 0.22977950e03)
        self.assertFloatEqual(rft1[0].depth, 0.21384775e04)

        self.assertFloatEqual(rft2[0].pressure, 0.19142435e03)
        self.assertFloatEqual(rft2[0].depth, 0.21383721e04)
