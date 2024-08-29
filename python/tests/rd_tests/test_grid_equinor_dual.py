
from resdata.util.test import TestAreaContext
from resdata.grid import Grid

from tests import ResdataTest, equinor_test


@equinor_test()
class GridDualTest(ResdataTest):
    def egrid_file(self):
        return self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.EGRID")

    def grid_file(self):
        return self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.GRID")

    def test_dual(self):
        with TestAreaContext("python/grid-test/testDual"):
            grid = Grid(self.egrid_file())
            self.assertFalse(grid.dualGrid())
            self.assertTrue(grid.getNumActiveFracture() == 0)

            grid2 = Grid(self.grid_file())
            self.assertFalse(grid.dualGrid())
            self.assertTrue(grid.getNumActiveFracture() == 0)

            dgrid = Grid(
                self.createTestPath("Equinor/ECLIPSE/DualPoro/DUALPOR_MSW.EGRID")
            )
            self.assertTrue(dgrid.getNumActive() == dgrid.getNumActiveFracture())
            self.assertTrue(dgrid.getNumActive() == 46118)

            dgrid2 = Grid(
                self.createTestPath("Equinor/ECLIPSE/DualPoro/DUALPOR_MSW.GRID")
            )
            self.assertTrue(dgrid.getNumActive() == dgrid.getNumActiveFracture())
            self.assertTrue(dgrid.getNumActive() == 46118)
            self.assertTrue(dgrid.equal(dgrid2))

            # The DUAL_DIFF grid has been manipulated to create a
            # situation where some cells are only matrix active, and some
            # cells are only fracture active.
            dgrid = Grid(
                self.createTestPath("Equinor/ECLIPSE/DualPoro/DUAL_DIFF.EGRID")
            )
            self.assertTrue(dgrid.getNumActive() == 106)
            self.assertTrue(dgrid.getNumActiveFracture() == 105)

            self.assertTrue(dgrid.get_active_fracture_index(global_index=0) == -1)
            self.assertTrue(dgrid.get_active_fracture_index(global_index=2) == -1)
            self.assertTrue(dgrid.get_active_fracture_index(global_index=3) == 0)
            self.assertTrue(dgrid.get_active_fracture_index(global_index=107) == 104)

            self.assertTrue(dgrid.get_active_index(global_index=1) == 1)
            self.assertTrue(dgrid.get_active_index(global_index=105) == 105)
            self.assertTrue(dgrid.get_active_index(global_index=106) == -1)
            self.assertTrue(dgrid.get_global_index1F(2) == 5)

            dgrid.save_EGRID("DUAL_DIFF.EGRID")
            dgrid2 = Grid("DUAL_DIFF.EGRID")
            self.assertTrue(dgrid.equal(dgrid2, verbose=True))
