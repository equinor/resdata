#!/usr/bin/env python
import datetime
from ecl.util.util import CTime
from ecl.rft import EclRFTFile, EclRFTCell, EclPLTCell, EclRFT, WellTrajectory
from tests import EclTest


class RFTTest(EclTest):
    def test_create(self):
        rft = EclRFT("WELL", "RFT", datetime.date(2015, 10, 1), 100)
        self.assertEqual(len(rft), 0)

        with self.assertRaises(IndexError):
            cell = rft[5]

    def test_repr(self):
        rft = EclRFT("WELL", "RFT", datetime.date(2015, 10, 1), 100)
        pfx = "EclRFT(completed_cells = 0, date = 2015-10-01, RFT)"
        self.assertEqual(pfx, repr(rft)[: len(pfx)])
