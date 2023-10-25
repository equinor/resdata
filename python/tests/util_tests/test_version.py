import resdata
from resdata.util.util import ResdataVersion, Version
from tests import ResdataTest


class VersionTest(ResdataTest):
    def setUp(self):
        pass

    def test_create(self):
        v1 = Version(1, 8, 6)
        self.assertFalse(v1.isDevelVersion())

        self.assertEqual(v1.versionString(), "1.8.6")
        self.assertEqual(v1.versionTuple(), (1, 8, 6))

        v2 = Version(2, 0, "X")
        self.assertTrue(v2.isDevelVersion())

    def test_eq(self):
        v1 = Version(1, 2, 3)
        v2 = Version(1, 2, 3)

        self.assertTrue(v1 == v2)
        self.assertEqual(v1, v2)
        self.assertEqual(str(v1), str(v2))
        self.assertEqual(repr(v1), repr(v2))
        self.assertFalse(v1 != v2)

        v1 = Version(1, 2, "X")
        v2 = Version(1, 2, "Y")
        self.assertTrue(v1 != v2)
        self.assertFalse(v1 == v2)

        v1 = Version(1, 2, "X")
        v2 = Version(1, 2, 0)
        self.assertTrue(v1 != v2)
        self.assertFalse(v1 == v2)

        v1 = Version(1, 2, "X")
        v2 = Version(1, 3, "X")
        self.assertTrue(v1 != v2)
        self.assertFalse(v1 == v2)

        v1 = Version(1, 2, "X")
        v2 = (1, 3, "X")
        self.assertTrue(v1 != v2)
        self.assertFalse(v1 == v2)

    def test_ge(self):
        v1 = Version(1, 2, 3)
        v2 = Version(1, 2, 3)
        v3 = (1, 2, 2)

        self.assertEqual(str(v1), str(v2))
        self.assertEqual(repr(v1), repr(v2))

        self.assertTrue(v1 >= v2)
        self.assertFalse(v1 < v2)

        self.assertTrue(v1 >= v3)
        self.assertFalse(v1 < v3)

        v1 = Version(1, 2, "X")
        v2 = Version(1, 1, 9)
        self.assertTrue(v1 > v2)

        v2 = Version(1, 2, "X")
        self.assertTrue(v1 >= v2)

        v2 = Version(1, 2, 0)
        self.assertFalse(v1 >= v2)

        self.assertNotEqual(str(v1), str(v2))
        self.assertNotEqual(repr(v1), repr(v2))

    def test_current(self):
        current = ResdataVersion()
        self.assertTrue(current > (0, 0, 0))
        pfx = "Version(major="
        self.assertEqual(pfx, repr(current)[: len(pfx)])

    def test_root_version(self):
        cv = ResdataVersion()
        self.assertEqual(resdata.__version__, cv.versionString())
