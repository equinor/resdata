#!/usr/bin/env python
#  Copyright (C) 2017  Statoil ASA, Norway. 
#   
#  The file 'test_grid_generator.py' is part of ERT - Ensemble based Reservoir Tool.
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

from ecl.ecl import EclGrid
from ecl.ecl import EclGridGenerator as GridGen
from ecl.test import ExtendedTestCase, TestAreaContext
from itertools import product as prod
import operator

dims = (6,6,6)

zcorn_count = [144, 288, 288, 288, 288, 288, 144]
zcorn = [x for l in [[i]*c for i,c in
            enumerate(zcorn_count)] for x in l]

coord = [1.0, 1.0, 0.0, 1.0, 1.0, 6.0, 2.0, 1.0, 0.0, 2.0, 1.0, 6.0, 3.0, 1.0,
        0.0, 3.0, 1.0, 6.0, 4.0, 1.0, 0.0, 4.0, 1.0, 6.0, 5.0, 1.0, 0.0, 5.0,
        1.0, 6.0, 6.0, 1.0, 0.0, 6.0, 1.0, 6.0, 7.0, 1.0, 0.0, 7.0, 1.0, 6.0,
        1.0, 2.0, 0.0, 1.0, 2.0, 6.0, 2.0, 2.0, 0.0, 2.0, 2.0, 6.0, 3.0, 2.0,
        0.0, 3.0, 2.0, 6.0, 4.0, 2.0, 0.0, 4.0, 2.0, 6.0, 5.0, 2.0, 0.0, 5.0,
        2.0, 6.0, 6.0, 2.0, 0.0, 6.0, 2.0, 6.0, 7.0, 2.0, 0.0, 7.0, 2.0, 6.0,
        1.0, 3.0, 0.0, 1.0, 3.0, 6.0, 2.0, 3.0, 0.0, 2.0, 3.0, 6.0, 3.0, 3.0,
        0.0, 3.0, 3.0, 6.0, 4.0, 3.0, 0.0, 4.0, 3.0, 6.0, 5.0, 3.0, 0.0, 5.0,
        3.0, 6.0, 6.0, 3.0, 0.0, 6.0, 3.0, 6.0, 7.0, 3.0, 0.0, 7.0, 3.0, 6.0,
        1.0, 4.0, 0.0, 1.0, 4.0, 6.0, 2.0, 4.0, 0.0, 2.0, 4.0, 6.0, 3.0, 4.0,
        0.0, 3.0, 4.0, 6.0, 4.0, 4.0, 0.0, 4.0, 4.0, 6.0, 5.0, 4.0, 0.0, 5.0,
        4.0, 6.0, 6.0, 4.0, 0.0, 6.0, 4.0, 6.0, 7.0, 4.0, 0.0, 7.0, 4.0, 6.0,
        1.0, 5.0, 0.0, 1.0, 5.0, 6.0, 2.0, 5.0, 0.0, 2.0, 5.0, 6.0, 3.0, 5.0,
        0.0, 3.0, 5.0, 6.0, 4.0, 5.0, 0.0, 4.0, 5.0, 6.0, 5.0, 5.0, 0.0, 5.0,
        5.0, 6.0, 6.0, 5.0, 0.0, 6.0, 5.0, 6.0, 7.0, 5.0, 0.0, 7.0, 5.0, 6.0,
        1.0, 6.0, 0.0, 1.0, 6.0, 6.0, 2.0, 6.0, 0.0, 2.0, 6.0, 6.0, 3.0, 6.0,
        0.0, 3.0, 6.0, 6.0, 4.0, 6.0, 0.0, 4.0, 6.0, 6.0, 5.0, 6.0, 0.0, 5.0,
        6.0, 6.0, 6.0, 6.0, 0.0, 6.0, 6.0, 6.0, 7.0, 6.0, 0.0, 7.0, 6.0, 6.0,
        1.0, 7.0, 0.0, 1.0, 7.0, 6.0, 2.0, 7.0, 0.0, 2.0, 7.0, 6.0, 3.0, 7.0,
        0.0, 3.0, 7.0, 6.0, 4.0, 7.0, 0.0, 4.0, 7.0, 6.0, 5.0, 7.0, 0.0, 5.0,
        7.0, 6.0, 6.0, 7.0, 0.0, 6.0, 7.0, 6.0, 7.0, 7.0, 0.0, 7.0, 7.0, 6.0]


class GridGeneratorTest(ExtendedTestCase):

    def setUp(self):
        ispace = [(l, u) for l in range(dims[0]) for u in range(l, dims[0])]
        jspace = [(l, u) for l in range(dims[0]) for u in range(l, dims[0])]
        kspace = [(l, u) for l in range(dims[0]) for u in range(l, dims[0])]
        self.ijk_space = prod(ispace, jspace, kspace)

    def test_extract_grid_decomposition_change(self):
        for ijk in self.ijk_space:
            if sum(zip(*ijk)[0])%2 is 0:
                GridGen.extract_grid(dims, coord, zcorn, ijk)
            else:
                with self.assertRaises(ValueError):
                    GridGen.extract_grid(dims, coord, zcorn, ijk)

            GridGen.extract_grid(dims,
                                 coord, zcorn,
                                 ijk,
                                 decomposition_change=True)

    def test_extract_grid_invalid_bounds(self):
        with self.assertRaises(ValueError):
            GridGen.extract_grid(dims, coord, zcorn, ((-1,0), (2,2), (2,2)))

        with self.assertRaises(ValueError):
            GridGen.extract_grid(dims, coord, zcorn, ((1,6), (2,2), (2,2)))
                
        with self.assertRaises(ValueError):
            GridGen.extract_grid(dims, coord, zcorn, ((1,6), (1,2), (2,2)))

    def test_extract_grid_slice_spec(self):
        for ijk in self.ijk_space:
            ijk = list(ijk)
            for i in range(3):
                if len(set(ijk[i])) == 1:
                    ijk[i] = ijk[i][0]

            GridGen.extract_grid(dims,
                                 coord, zcorn,
                                 ijk,
                                 decomposition_change=True)
