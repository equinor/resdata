from resdata.summary import ResdataCmp

from tests import ResdataTest, equinor_test


@equinor_test()
class ResdataCmpTest(ResdataTest):
    def setUp(self):
        self.root1 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE")
        self.root2 = self.createTestPath("Equinor/ECLIPSE/Oseberg/F8MLT/F8MLT-F4")

    def test_not_existing(self):
        with self.assertRaises(IOError):
            _rd_cmp = ResdataCmp("missing/case1", "missing/case2")

        with self.assertRaises(IOError):
            _rd_cmp = ResdataCmp("missing/case1", self.root1)

        with self.assertRaises(IOError):
            _rd_cmp = ResdataCmp(self.root1, "missing/case1")

        _rd_cmp = ResdataCmp(self.root1, self.root1)
        _rd_cmp = ResdataCmp(self.root2, self.root2)

    def test_different_start(self):
        with self.assertRaises(ValueError):
            _rd_cmp = ResdataCmp(self.root1, self.root2)

    def test_summary_cmp(self):
        rd_cmp = ResdataCmp(self.root1, self.root1)
        self.assertEqual((False, False), rd_cmp.hasSummaryVector("MISSING"))
        self.assertEqual((True, True), rd_cmp.hasSummaryVector("FOPT"))

        with self.assertRaises(KeyError):
            _diff = rd_cmp.cmpSummaryVector("MISSING")

        diff_sum, _ref_sum = rd_cmp.cmpSummaryVector("FOPT")
        self.assertEqual(diff_sum, 0.0)
        self.assertTrue(rd_cmp.endTimeEqual())

    def test_wells(self):
        rd_cmp = ResdataCmp(self.root1, self.root1)
        wells = rd_cmp.testWells()

        well_set = {"OP_1", "OP_2", "OP_3", "OP_4", "OP_5", "WI_1", "WI_2", "WI_3"}
        self.assertEqual(len(wells), len(well_set))
        for well in wells:
            self.assertTrue(well in well_set)
