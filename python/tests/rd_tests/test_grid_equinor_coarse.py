import itertools

from resdata.grid import Grid
from resdata.resfile import ResdataRestartFile
from resdata.util.test import TestAreaContext

from tests import ResdataTest, equinor_test


@equinor_test()
class GridCoarceTest(ResdataTest):
    def lgc_grid(self):
        return Grid(self.createTestPath("Equinor/ECLIPSE/LGCcase/LGC_TESTCASE2.EGRID"))

    def lgc_restart(self, grid):
        return ResdataRestartFile(
            grid, self.createTestPath("Equinor/ECLIPSE/LGCcase/LGC_TESTCASE2.UNRST")
        )

    def test_save_roundtrip(self):
        with TestAreaContext("python/grid-test/testCoarse"):
            g1 = self.lgc_grid()

            g1.save_EGRID("LGC.EGRID")
            g2 = Grid("LGC.EGRID")
            self.assertTrue(g1.equal(g2, verbose=True))
            self.assertEqual(g1.coarse_groups(), g2.coarse_groups())

    def test_lgc_number_of_groups(self):
        with TestAreaContext("python/grid-test/testCoarse"):
            g1 = self.lgc_grid()

            self.assertTrue(g1.coarse_groups() == 3384)

    def test_create_3d_agrees_with_get_value_on_coarse_grid(self):
        with TestAreaContext("python/grid-test/testCoarse"):
            grid = self.lgc_grid()
            kw = self.lgc_restart(grid)["SGAS"][0]
            nx = grid.getNX()
            ny = grid.getNY()
            nz = grid.getNZ()

            # limit amount of points tested by
            # only testing every 3rd point
            x_space = range(0, nx, 3)
            y_space = range(0, ny, 3)
            z_space = range(0, nz, 3)

            numpy_3d = grid.create3D(kw)
            for k, j, i in itertools.product(z_space, y_space, x_space):
                if grid.active(ijk=(i, j, k)):
                    self.assertAlmostEqual(
                        numpy_3d[i, j, k], grid.grid_value(kw, i, j, k)
                    )
                else:
                    self.assertEqual(numpy_3d[i, j, k], 0.0)
