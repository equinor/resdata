#  Copyright (C) 2018  Equinor ASA, Norway.
#
#  The file 'test_grid_equinor_coarse.py' is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details.
import math

from ecl.util.test import TestAreaContext
from ecl.grid import EclGrid

from tests import EclTest, equinor_test


@equinor_test()
class GridCoarceTest(EclTest):
    def lgc_grid(self):
        return EclGrid(
            self.createTestPath("Equinor/ECLIPSE/LGCcase/LGC_TESTCASE2.EGRID")
        )

    def test_save_roundtrip(self):
        with TestAreaContext("python/grid-test/testCoarse"):
            g1 = self.lgc_grid()

            g1.save_EGRID("LGC.EGRID")
            g2 = EclGrid("LGC.EGRID")
            self.assertTrue(g1.equal(g2, verbose=True))
            self.assertEqual(g1.coarse_groups(), g2.coarse_groups())

    def test_lgc_number_of_groups(self):
        with TestAreaContext("python/grid-test/testCoarse"):
            g1 = self.lgc_grid()

            self.assertTrue(g1.coarse_groups() == 3384)
