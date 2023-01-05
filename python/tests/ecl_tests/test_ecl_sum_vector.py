#!/usr/bin/env python
try:
    from unittest2 import skipIf
except ImportError:
    from unittest import skipIf

import warnings

from ecl.summary import EclSumVector, EclSum
from tests import EclTest, equinor_test


@equinor_test()
class EclSumVectorTest(EclTest):
    def setUp(self):
        self.test_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.SMSPEC")
        self.ecl_sum = EclSum(self.test_file)

    def test_reportOnly_warns(self):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")

            vector = EclSumVector(self.ecl_sum, "FOPT", True)
            self.assertEqual(len(w), 1)
            assert issubclass(w[-1].category, DeprecationWarning)

    def test_basic(self):
        self.assertEqual(512, len(self.ecl_sum.keys()))
        pfx = "EclSum(name"
        self.assertEqual(pfx, repr(self.ecl_sum)[: len(pfx)])
        it = iter(self.ecl_sum)
        # t = self.ecl_sum[it.next()] # EclSumVector
        t = self.ecl_sum[next(it)]  # EclSumVector
        self.assertEqual(63, len(t))
        self.assertEqual("BARSA", t.unit)
        pfx = "EclSumVector(key = "
        self.assertEqual(pfx, repr(t)[: len(pfx)])
