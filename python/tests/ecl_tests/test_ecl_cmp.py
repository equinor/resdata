from ecl.util.test import TestAreaContext
from ecl.util.test.ecl_mock import createEclSum
from ecl.summary import EclCmp
from tests import EclTest, equinor_test


@equinor_test()
class EclCmpTest(EclTest):
    def setUp(self):
        self.root1 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE")
        self.root2 = self.createTestPath("Equinor/ECLIPSE/Oseberg/F8MLT/F8MLT-F4")

    def test_not_existing(self):
        with self.assertRaises(IOError):
            ecl_cmp = EclCmp("missing/case1", "missing/case2")

        with self.assertRaises(IOError):
            ecl_cmp = EclCmp("missing/case1", self.root1)

        with self.assertRaises(IOError):
            ecl_cmp = EclCmp(self.root1, "missing/case1")

        ecl_cmp = EclCmp(self.root1, self.root1)
        ecl_cmp = EclCmp(self.root2, self.root2)

    def test_different_start(self):
        with self.assertRaises(ValueError):
            ecl_cmp = EclCmp(self.root1, self.root2)

    def test_summary_cmp(self):
        ecl_cmp = EclCmp(self.root1, self.root1)
        self.assertEqual((False, False), ecl_cmp.hasSummaryVector("MISSING"))
        self.assertEqual((True, True), ecl_cmp.hasSummaryVector("FOPT"))

        with self.assertRaises(KeyError):
            diff = ecl_cmp.cmpSummaryVector("MISSING")

        diff_sum, ref_sum = ecl_cmp.cmpSummaryVector("FOPT")
        self.assertEqual(diff_sum, 0.0)
        self.assertTrue(ecl_cmp.endTimeEqual())

    def test_wells(self):
        ecl_cmp = EclCmp(self.root1, self.root1)
        wells = ecl_cmp.testWells()

        well_set = set(["OP_1", "OP_2", "OP_3", "OP_4", "OP_5", "WI_1", "WI_2", "WI_3"])
        self.assertEqual(len(wells), len(well_set))
        for well in wells:
            self.assertTrue(well in well_set)
