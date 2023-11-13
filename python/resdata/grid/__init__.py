"""
  rd_grid/Grid: This will load a GRID or EGRID file, and
     can then subsequently be used for queries about the grid.

  rd_region/ResdataRegion: Convenience class to support selecting cells
     in a grid based on a wide range of criteria. Can be used as a
     mask in operations on ResdataKW instances.

  rd_grid_generator/GridGenerator: This can be used to generate various
    grids.
"""

from .cell import Cell
from .rd_grid import Grid
from .rd_grid_generator import GridGenerator
from .rd_region import ResdataRegion

__all__ = [
    "Cell",
    "Grid",
    "GridGenerator",
    "ResdataRegion",
]
