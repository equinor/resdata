#  Copyright (C) 2016  Statoil ASA, Norway.
#
#  The file 'test_rft.py' is part of ERT - Ensemble based Reservoir Tool.
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


import datetime
from tests import EclTest
from ecl.test import TestAreaContext
from ecl.ecl import EclGridGenerator as GridGen
from ecl.ecl import EclGrid
import ecl.ecl as ecl

class LoadTest(EclTest):

    def test_load(self):
        with self.assertRaises(IOError):
            ecl.load("File/does/not/exist")

        with TestAreaContext("ecl_load"):
            with open("file.txt","w") as f:
                f.write("A text file - not recognized by Eclipse")

            with self.assertRaises(ValueError):
                ecl.load("file.txt")



        def test_load_grid(self):
            with TestAreaContext("grid_load"):
                grid = GridGen.create_rectangular((10,10,10), (10,10,10))
                grid.save_EGRID("TEST.EGRID")
                grid.save_GRID("TEST.GRID")

                g1 = ecl.load("TEST.EGRID")
                g2 = ecl.load("TEST.GRID")

                self.assertTrue( isinstance( g1, EclGrid ))
                self.assertTrue( isinstance( g2, EclGrid ))
