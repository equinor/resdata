#!/usr/bin/env python
from ecl.grid import EclGrid
from ecl.util.test import TestAreaContext
from tests import EclTest


class FKTest(EclTest):
    def test_cell_containment(self):
        grid_location = "local/ECLIPSE/faarikaal/faarikaal%d.EGRID"
        well_location = "local/ECLIPSE/faarikaal/faarikaal%d.txt"

        for i in range(1, 8):
            grid_file = self.createTestPath(grid_location % i)
            well_file = self.createTestPath(well_location % i)

            grid = EclGrid(grid_file)

            # Load well data
            with open(well_file, "r") as f:
                lines = [line.split() for line in f.readlines()]

            points = [map(float, line[:3:]) for line in lines]
            exp_cells = [tuple(map(int, line[3::])) for line in lines]

            msg = "Expected point %s to be in cell %s, was in %s."
            for point, exp_cell in zip(points, exp_cells):
                reported_cell = grid.find_cell(*point)
                self.assertEqual(
                    exp_cell,
                    reported_cell,
                    msg % (str(point), str(exp_cell), str(reported_cell)),
                )
