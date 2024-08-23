#!/usr/bin/env python

import warnings

from resdata.summary import Summary, SummaryVector

from tests import ResdataTest, equinor_test


@equinor_test()
class SummaryVectorTest(ResdataTest):
    def setUp(self):
        self.test_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.SMSPEC")
        self.rd_sum = Summary(self.test_file)

    def test_reportOnly_warns(self):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")

            _vector = SummaryVector(self.rd_sum, "FOPT", True)
            self.assertEqual(len(w), 1)
            assert issubclass(w[-1].category, DeprecationWarning)

    def test_basic(self):
        self.assertEqual(512, len(self.rd_sum.keys()))
        pfx = "Summary(name"
        self.assertEqual(pfx, repr(self.rd_sum)[: len(pfx)])
        it = iter(self.rd_sum)
        # t = self.rd_sum[it.next()] # SummaryVector
        t = self.rd_sum[next(it)]  # SummaryVector
        self.assertEqual(63, len(t))
        self.assertEqual("BARSA", t.unit)
        pfx = "SummaryVector(key = "
        self.assertEqual(pfx, repr(t)[: len(pfx)])
