"""
  ecl_grid/EclGrid: This will load an ECLIPSE GRID or EGRID file, and
     can then subsequently be used for queries about the grid.

  ecl_region/EclRegion: Convenience class to support selecting cells
     in a grid based on a wide range of criteria. Can be used as a
     mask in operations on EclKW instances.

  ecl_grid_generator/EclGridGenerator: This can be used to generate various
    grids.
"""

import ecl.util.util
import ecl.util.geometry

from .cell import Cell
from .ecl_grid import EclGrid
from .ecl_region import EclRegion
from .ecl_grid_generator import EclGridGenerator
