#!/usr/bin/env python
import datetime
import math
import os
import warnings

from resdata.summary import NPVPriceVector, ResdataNPV, Summary
from resdata.util.util import CTime, DoubleVector, StringList, TimeVector

from tests import ResdataTest, equinor_test
from tests.util.mock import createSummary

base = "ECLIPSE"
path = "Equinor/ECLIPSE/Gurbat"
case = "%s/%s" % (path, base)


def _callable(x):
    return 1


def linear1(x):
    return x


def linear2(x):
    return 2 * x


class NPVMockTest(ResdataTest):
    """Non-equinor tests for ResdataNPV / NPVPriceVector using mock summaries."""

    def _make_case(self, name="CASE1"):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            case = createSummary(
                name,
                [("FOPT", None, 0, "SM3"), ("WOPT", "OP1", 0, "SM3")],
            )
        case.fwrite()
        return case

    def test_parse_and_compile(self):
        tmpdir = self.tmp_path_factory.mktemp("npv_mock_parse", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            self._make_case("CASE1")
            npv = ResdataNPV("CASE1")
            parsed = npv.parse_expression("[FOPT]")
            self.assertEqual(parsed, "FOPT[i]")
            self.assertIn("FOPT", npv.get_key_list())

            npv.compile("[FOPT]")
            value = npv.eval_npv()
            self.assertIsNotNone(value)

    def test_price_vector_mock(self):
        vec = NPVPriceVector(
            [
                ("01/01/2000", 100),
                ("01/02/2000", 200),
                ("01/03/2000", 300),
            ]
        )
        self.assertEqual(300, vec.eval(datetime.date(2000, 4, 1)))
        self.assertEqual(100, vec.eval(datetime.date(2000, 1, 10)))
        self.assertEqual(200, vec.eval(datetime.date(2000, 2, 15)))


@equinor_test()
class NPVTest(ResdataTest):
    def setUp(self):
        self.case = self.createTestPath(case)

    def test_create(self):
        with self.assertRaises(Exception):
            npv = ResdataNPV("/does/not/exist")

        npv = ResdataNPV(self.case)

    def test_eval_npv(self):
        npv = ResdataNPV(self.case)
        with self.assertRaises(ValueError):
            npv.eval()

    def test_expression(self):
        npv = ResdataNPV(self.case)
        self.assertIsNone(npv.get_expression())
        npv.set_expression("[FOPT]*$OIL_PRICE - [FGIT]*$GAS_PRICE")
        self.assertEqual(npv.get_expression(), "[FOPT]*$OIL_PRICE - [FGIT]*$GAS_PRICE")
        self.assertIn("FOPT", npv.get_key_list())
        self.assertIn("FGIT", npv.get_key_list())

        with self.assertRaises(ValueError):
            npv.parse_expression("[FOPT")

        with self.assertRaises(ValueError):
            npv.parse_expression("FOPT]")

        with self.assertRaises(KeyError):
            npv.parse_expression("[FoPT]")

        with self.assertRaises(ValueError):
            npv.parse_expression("[FOPR]")

        parsedExpression = npv.parse_expression("[FOPT]")
        self.assertEqual(parsedExpression, "FOPT[i]")
        self.assertEqual(1, len(npv.get_key_list()))

        parsedExpression = npv.parse_expression("[FOPT]*2 + [FGPT] - [WOPT:OP_1]")
        self.assertEqual(parsedExpression, "FOPT[i]*2 + FGPT[i] - WOPT_OP_1[i]")
        keyList = npv.get_key_list()
        self.assertEqual(3, len(keyList))
        self.assertIn("FOPT", keyList)
        self.assertIn("FGPT", keyList)
        self.assertIn("WOPT:OP_1", keyList)

    def test_period(self):
        npv = ResdataNPV(self.case)
        self.assertIsNone(npv.start)
        self.assertIsNone(npv.end)
        self.assertEqual("1Y", npv.interval)

    def test_eval(self):
        npv = ResdataNPV(self.case)
        npv.compile("[FOPT]")
        npv1 = npv.eval_npv()

        npv2 = 0
        summary = Summary(self.case)
        trange = summary.time_range()
        fopr = summary.blocked_production("FOPT", trange)
        for v in fopr:
            npv2 += v
        self.assertAlmostEqual(npv1, npv2)

        npv.compile("[FOPT] - 0.5*[FOPT] - 0.5*[FOPT]")
        npv1 = npv.eval_npv()
        self.assertTrue(abs(npv1) < 1e-2)

        npv.compile("[WOPT:OP_1] - 0.5*[WOPT:OP_1] - 0.5*[WOPT:OP_1]")
        npv1 = npv.eval_npv()
        self.assertTrue(abs(npv1) < 1e-2)

    def test_price_vector(self):
        with self.assertRaises(ValueError):
            NPVPriceVector("NotList")

        with self.assertRaises(ValueError):
            NPVPriceVector(1.25)

        with self.assertRaises(ValueError):
            NPVPriceVector((1, 25))

        with self.assertRaises(ValueError):
            NPVPriceVector([1, 2, 3])

        with self.assertRaises(ValueError):
            NPVPriceVector([(1, 25), ("String", 100, 100)])

        with self.assertRaises(ValueError):
            NPVPriceVector([(1, 25), ("String", 100)])

        NPVPriceVector([(datetime.datetime(2010, 1, 1, 0, 0, 0), 100)])
        NPVPriceVector([(datetime.date(2010, 1, 1), 100)])
        NPVPriceVector([("19/06/2010", 100)])

        with self.assertRaises(ValueError):
            NPVPriceVector([("01/01/2000", 100), ("01/01/1999", 100)])

        with self.assertRaises(ValueError):
            NPVPriceVector([("01/01/2000", "String")])

        NPVPriceVector([("01/01/2000", 100)])
        NPVPriceVector([("01/01/2000", 77.99)])
        NPVPriceVector([("01/01/2000", _callable)])

        vec = NPVPriceVector(
            [("01/01/2000", 100), ("01/02/2000", 200), ("01/03/2000", 300)]
        )

        with self.assertRaises(ValueError):
            vec.eval(datetime.date(1999, 1, 1))

        self.assertEqual(
            datetime.date(2000, 1, 1),
            NPVPriceVector.assert_date(datetime.date(2000, 1, 1)),
        )
        self.assertEqual(
            datetime.date(2000, 1, 1),
            NPVPriceVector.assert_date(CTime(datetime.date(2000, 1, 1))),
        )

        self.assertEqual(100, vec.eval(datetime.date(2000, 1, 10)))
        self.assertEqual(100, vec.eval(datetime.datetime(2000, 1, 10, 0, 0, 0)))
        self.assertEqual(100, vec.eval(CTime(datetime.datetime(2000, 1, 10, 0, 0, 0))))

        self.assertEqual(300, vec.eval(datetime.date(2000, 4, 1)))

        vec = NPVPriceVector(
            [("01/01/2000", linear1), ("01/02/2000", linear2), ("01/03/2000", 300)]
        )

        self.assertEqual(300, vec.eval(datetime.date(2000, 3, 10)))
        self.assertEqual(0, vec.eval(datetime.date(2000, 1, 1)))
        self.assertEqual(10, vec.eval(datetime.date(2000, 1, 11)))
        self.assertEqual(20, vec.eval(datetime.date(2000, 1, 21)))

        self.assertEqual(0, vec.eval(datetime.date(2000, 2, 1)))
        self.assertEqual(20, vec.eval(datetime.date(2000, 2, 11)))
        self.assertEqual(40, vec.eval(datetime.date(2000, 2, 21)))
