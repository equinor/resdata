#!/usr/bin/env python
#  Copyright (C) 2018  Statoil ASA, Norway.
#
#  The file 'test_grid.pandas' is part of ERT - Ensemble based Reservoir Tool.
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

import numpy as np
import pandas as pd

from ecl.grid import EclGrid

from tests import EclTest

class GridPandasTest(EclTest):

  def test_dataframe(self):
    grid = EclGrid.create_rectangular( (2,3,1), (1,1,1) , actnum=[1, 1, 0, 0, 1, 1])
    df = grid.export_index_frame(True)
    index_matrix = np.array([ [0, 0, 0, 0],
                              [1, 0, 0, 1], 
                              [0, 2, 0, 2],
                              [1, 2, 0, 3] ])
    assert( np.array_equal(df.values, index_matrix) )

