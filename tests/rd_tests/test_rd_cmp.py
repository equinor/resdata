from resdata.summary import ResdataCmp
from tests import ResdataTest, equinor_test
from tests.util import TestAreaContext
from tests.util.mock import createSummary

import datetime
import warnings
import pytest


@equinor_test()
class ResdataCmpTest(ResdataTest):
    def setUp(self):
        self.root1 = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE")
        self.root2 = self.createTestPath("Equinor/ECLIPSE/Oseberg/F8MLT/F8MLT-F4")

    def test_not_existing(self):
        with self.assertRaises(IOError):
            rd_cmp = ResdataCmp("missing/case1", "missing/case2")

        with self.assertRaises(IOError):
            rd_cmp = ResdataCmp("missing/case1", self.root1)

        with self.assertRaises(IOError):
            rd_cmp = ResdataCmp(self.root1, "missing/case1")

        rd_cmp = ResdataCmp(self.root1, self.root1)
        rd_cmp = ResdataCmp(self.root2, self.root2)

    def test_different_start(self):
        with self.assertRaises(ValueError):
            rd_cmp = ResdataCmp(self.root1, self.root2)

    def test_summary_cmp(self):
        rd_cmp = ResdataCmp(self.root1, self.root1)
        self.assertEqual((False, False), rd_cmp.has_summary_vector("MISSING"))
        self.assertEqual((True, True), rd_cmp.has_summary_vector("FOPT"))

        with self.assertRaises(KeyError):
            diff = rd_cmp.cmp_summary_vector("MISSING")

        diff_sum, ref_sum = rd_cmp.cmp_summary_vector("FOPT")
        self.assertEqual(diff_sum, 0.0)
        self.assertTrue(rd_cmp.endTimeEqual())

    def test_wells(self):
        rd_cmp = ResdataCmp(self.root1, self.root1)
        wells = rd_cmp.test_wells()

        well_set = set(["OP_1", "OP_2", "OP_3", "OP_4", "OP_5", "WI_1", "WI_2", "WI_3"])
        self.assertEqual(len(wells), len(well_set))
        for well in wells:
            self.assertTrue(well in well_set)


def _make_case(name, sim_start=datetime.date(2010, 1, 1)):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", DeprecationWarning)
        case = createSummary(
            name,
            [("FOPT", None, 0, "SM3"), ("WOPT", "OP1", 0, "SM3")],
            sim_start=sim_start,
        )
    case.fwrite()
    return case


def test_cmp_same_case(tmpdir):
    with tmpdir.as_cwd():
        _make_case("CASE1")
        rd_cmp = ResdataCmp("CASE1", "CASE1")
        assert rd_cmp.endTimeEqual()
        diff_sum, ref_sum = rd_cmp.cmpSummaryVector("FOPT")
        assert diff_sum == 0.0
        assert (True, True) == rd_cmp.hasSummaryVector("FOPT")


def test_cmp_different_start(tmpdir):
    with tmpdir.as_cwd():
        _make_case("CASE1", sim_start=datetime.date(2010, 1, 1))
        _make_case("CASE2", sim_start=datetime.date(2011, 1, 1))
        with pytest.raises(
            ValueError, match="The two cases do not start at the same time"
        ):
            ResdataCmp("CASE1", "CASE2")
