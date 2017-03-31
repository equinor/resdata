from ert.geo import GeoRegion, GeoPointset
from ert.test import ExtendedTestCase, TestAreaContext


class GeoRegionTest(ExtendedTestCase):

    def test_init(self):
        ps = GeoPointset()
        gp = GeoRegion(ps)
        self.assertEqual(0, len(gp))

    def test_repr(self):
        ps = GeoPointset()
        gp = GeoRegion(ps)
        self.assertTrue(repr(gp).startswith('GeoRegion'))
