#!/usr/bin/env python
import datetime

from resdata.rft import (
    ResdataRFT,
)

from tests import ResdataTest


class RFTTest(ResdataTest):
    def test_create(self):
        rft = ResdataRFT("WELL", "RFT", datetime.date(2015, 10, 1), 100)
        self.assertEqual(len(rft), 0)
        self.assertEqual(rft.get_well_name(), "WELL")

        with self.assertRaises(IndexError):
            _cell = rft[5]

    def test_repr(self):
        rft = ResdataRFT("WELL", "RFT", datetime.date(2015, 10, 1), 100)
        pfx = "ResdataRFT(completed_cells = 0, date = 2015-10-01, RFT)"
        self.assertEqual(pfx, repr(rft)[: len(pfx)])
