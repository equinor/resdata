from ecl import EclDataType
from ecl.eclfile import EclKW
from ecl.grid import EclGrid, EclRegion
from ecl.grid.faults import Layer
from ecl.util.util import IntVector
from tests import EclTest


class RegionTest(EclTest):
    def test_equal(self):
        grid = EclGrid.createRectangular((10, 10, 1), (1, 1, 1))
        kw_int = EclKW("INT", grid.getGlobalSize(), EclDataType.ECL_INT)
        kw_float = EclKW("FLOAT", grid.getGlobalSize(), EclDataType.ECL_FLOAT)

        kw_int[0:49] = 1
        region = EclRegion(grid, False)
        region.select_equal(kw_int, 1)
        glist = region.getGlobalList()
        for g in glist:
            self.assertEqual(kw_int[g], 1)

        with self.assertRaises(ValueError):
            region.select_equal(kw_float, 1)

    def test_sum(self):
        grid = EclGrid.createRectangular((10, 10, 1), (1, 1, 1))
        kw_mask = EclKW("INT", grid.getGlobalSize(), EclDataType.ECL_INT)
        int_value = EclKW("INT", grid.getGlobalSize(), EclDataType.ECL_INT)
        float_value = EclKW("FLOAT", grid.getGlobalSize(), EclDataType.ECL_FLOAT)
        double_value = EclKW("DOUBLE", grid.getGlobalSize(), EclDataType.ECL_DOUBLE)
        bool_value = EclKW("BOOL", grid.getGlobalSize(), EclDataType.ECL_BOOL)

        kw_mask[0:50] = 1

        for i in range(len(int_value)):
            float_value[i] = i
            double_value[i] = i
            int_value[i] = i
            bool_value[i] = True

        region = EclRegion(grid, False)
        region.select_equal(kw_mask, 1)

        self.assertEqual(int_value.sum(), 99 * 100 / 2)
        self.assertEqual(int_value.sum(mask=region), 49 * 50 / 2)
        self.assertEqual(double_value.sum(mask=region), 1.0 * 49 * 50 / 2)
        self.assertEqual(float_value.sum(mask=region), 1.0 * 49 * 50 / 2)
        self.assertEqual(bool_value.sum(mask=region), 50)

    def test_truth_and_size(self):
        actnum = IntVector(initial_size=100, default_value=0)
        actnum[0:50] = 1
        grid = EclGrid.createRectangular((10, 10, 1), (1, 1, 1), actnum=actnum)
        region = EclRegion(grid, False)

        self.assertFalse(region)
        self.assertEqual(0, region.active_size())
        self.assertEqual(0, region.global_size())

        region.select_all()
        self.assertTrue(region)
        self.assertEqual(50, region.active_size())
        self.assertEqual(100, region.global_size())

        region.deselect_all()
        self.assertFalse(region)
        self.assertEqual(0, region.active_size())
        self.assertEqual(0, region.global_size())

        region = EclRegion(grid, False)
        region.select_inactive()
        self.assertTrue(region)
        self.assertEqual(0, region.active_size())
        self.assertEqual(50, region.global_size())
