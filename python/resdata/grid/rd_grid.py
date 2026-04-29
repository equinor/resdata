"""
Module to load and query GRID/EGRID files.

The rd_grid module contains functionality to load and query an
grid file; it is currently not possible to manipulate or let
alone create a grid with rd_grid module. The functionality is
implemented in the Grid class. The rd_grid module is a thin
wrapper around the rd_grid.c implementation from the resdata library.
"""

import ctypes

import warnings
import numpy as np
import pandas as pd
import sys
import os.path
import math
import itertools
from collections.abc import Sequence
from cwrap import CFILE, BaseCClass, load, open as copen
from typing_extensions import deprecated

from resdata import ResdataPrototype
from resdata.util.util import monkey_the_camel
from resdata.util.util import IntVector
from resdata import ResDataType, UnitSystem, ResdataTypeEnum
from resdata.resfile import ResdataKW, FortIO
from resdata.grid import Cell
import resdata.grid._grid as _grid


class Grid(BaseCClass):
    """
    Class for loading and internalizing GRID/EGRID files.
    """

    TYPE_NAME = "rd_grid"

    @classmethod
    def load_from_grdecl(cls, filename):
        """Will create a new Grid instance from grdecl file.

        This function will scan the input file @filename and look for
        the keywords required to build a grid. The following keywords
        are required:

              SPECGRID   ZCORN   COORD

        In addition the function will look for and use the ACTNUM and
        MAPAXES keywords if they are found; if ACTNUM is not found all
        cells are assumed to be active.

        Slightly more exotic grid concepts like dual porosity, NNC
        mapping, LGR and coarsened cells will be completely ignored;
        if you need such concepts you must have an EGRID file and use
        the default Grid() constructor - that is also considerably
        faster.
        """

        if os.path.isfile(filename):
            with copen(filename) as f:
                specgrid = ResdataKW.read_grdecl(
                    f, "SPECGRID", rd_type=ResDataType.RD_INT, strict=False
                )
                zcorn = ResdataKW.read_grdecl(f, "ZCORN")
                coord = ResdataKW.read_grdecl(f, "COORD")
                try:
                    actnum = ResdataKW.read_grdecl(
                        f, "ACTNUM", rd_type=ResDataType.RD_INT
                    )
                except ValueError:
                    actnum = None

                try:
                    mapaxes = ResdataKW.read_grdecl(f, "MAPAXES")
                except ValueError:
                    mapaxes = None

            return Grid.create(specgrid, zcorn, coord, actnum, mapaxes)
        else:
            raise IOError("No such file:%s" % filename)

    @classmethod
    def load_from_file(cls, filename):
        """
        Will inspect the @filename argument and create a new Grid instance.
        """
        if FortIO.isFortranFile(filename):
            return Grid(filename)
        else:
            return Grid.loadFromGrdecl(filename)

    @classmethod
    def _python_object_from_ptr(cls, ptr):
        if not ptr:
            return None
        return cls.createPythonObject(ptr)

    def _reference_from_ptr(self, ptr):
        if not ptr:
            return None
        return self.createCReference(ptr, parent=self)

    @classmethod
    def create(cls, specgrid, zcorn, coord, actnum, mapaxes=None):
        """
        Create a new grid instance from existing keywords.

        This is a class method which can be used to create an Grid
        instance based on the ResdataKW instances @specgrid, @zcorn,
        @coord and @actnum. An EGRID file contains the
        SPECGRID, ZCORN, COORD and ACTNUM keywords, so a somewhat
        involved way to create a Grid instance could be:

          >> file = resdata.ResdataFile("CASE.EGRID")
          >> specgrid_kw = file.iget_named_kw("SPECGRID", 0)
          >> zcorn_kw = file.iget_named_kw("ZCORN", 0)
          >> coord_kw = file.iget_named_kw("COORD", 0)
          >> actnum_kw = file.iget_named_kw("ACTNUM", 0)

          >> grid = Grid.create(specgrid_kw, zcorn_kw, coord_kw, actnum_kw)

        If you are so inclined ...
        """
        return cls._python_object_from_ptr(
            _grid._grdecl_create(
                specgrid[0], specgrid[1], specgrid[2], zcorn, coord, actnum, mapaxes
            )
        )

    @classmethod
    @deprecated(
        "Grid.createRectangular is deprecated. It will be removed in version 7. "
        "Please use the similar method: GridGenerator.createRectangular.",
    )
    def create_rectangular(cls, dims, dV, actnum: Sequence[int] | None = None):
        """
        Will create a new rectangular grid. @dims = (nx,ny,nz)  @dVg = (dx,dy,dz)

        With the default value @actnum == None all cells will be active,
        """
        if actnum is None:
            rd_grid = _grid._alloc_rectangular(
                dims[0], dims[1], dims[2], dV[0], dV[1], dV[2], None
            )
        else:
            if not len(actnum) == dims[0] * dims[1] * dims[2]:
                raise ValueError(
                    "ACTNUM size mismatch: len(ACTNUM):%d  Expected:%d"
                    % (len(actnum), dims[0] * dims[1] * dims[2])
                )
            rd_grid = _grid._alloc_rectangular(
                dims[0], dims[1], dims[2], dV[0], dV[1], dV[2], list(actnum)
            )
        if not rd_grid:
            raise MemoryError("Failed to allocated regualar grid")
        return cls.createPythonObject(rd_grid)

    def __init__(self, filename, apply_mapaxes=True):
        """
        Will create a grid structure from an EGRID or GRID file.
        """
        c_ptr = _grid._fread_alloc(filename, apply_mapaxes)
        if c_ptr:
            super(Grid, self).__init__(c_ptr)
        else:
            raise IOError("Loading grid from:%s failed" % filename)

    def free(self):
        _grid._free(self)

    def _nicename(self):
        """name is often full path to grid, if so, output basename, else name"""
        name = self.getName()
        if os.path.isfile(name):
            name = os.path.basename(name)
        return name

    def __repr__(self):
        """Returns, e.g.:
        Grid("NORNE_ATW2013.EGRID", 46x112x22, global_size=113344, active_size=44431) at 0x28c4a70
        """
        name = self._nicename()
        if name:
            name = '"%s", ' % name
        g_size = self.getGlobalSize()
        a_size = self.getNumActive()
        xyz_s = "%dx%dx%d" % (self.getNX(), self.getNY(), self.getNZ())
        return self._create_repr(
            "%s%s, global_size=%d, active_size=%d" % (name, xyz_s, g_size, a_size)
        )

    def __len__(self):
        """
        len(grid) wil return the total number of cells.
        """
        return _grid._get_global_size(self)

    def equal(self, other, include_lgr=True, include_nnc=False, verbose=False):
        """
        Compare the current grid with the other grid.
        """
        if not isinstance(other, Grid):
            raise TypeError("The other argument must be an Grid instance")
        return _grid._equal(self, other, include_lgr, include_nnc, verbose)

    def dual_grid(self):
        """Is this grid dual porosity model?"""
        return _grid._dual_grid(self)

    def get_dims(self):
        """A tuple of four elements: (nx, ny, nz, nactive)."""
        return (self.getNX(), self.getNY(), self.getNZ(), self.getNumActive())

    @property
    def nx(self):
        return _grid._get_nx(self)

    def get_nx(self):
        """The number of elements in the x direction"""
        return _grid._get_nx(self)

    @property
    def ny(self):
        return _grid._get_ny(self)

    def get_ny(self):
        """The number of elements in the y direction"""
        return _grid._get_ny(self)

    @property
    def nz(self):
        return _grid._get_nz(self)

    def get_nz(self):
        """The number of elements in the z direction"""
        return _grid._get_nz(self)

    def get_global_size(self):
        """Returns the total number of cells in this grid"""
        return _grid._get_global_size(self)

    def get_num_active(self):
        """The number of active cells in the grid."""
        return _grid._get_active(self)

    def get_num_active_fracture(self):
        """The number of active cells in the grid."""
        return _grid._get_active_fracture(self)

    def get_bounding_box_2d(self, layer=0, lower_left=None, upper_right=None):
        if 0 <= layer <= self.getNZ():
            x = ctypes.c_double()
            y = ctypes.c_double()
            z = ctypes.c_double()

            if lower_left is None:
                i1 = 0
                j1 = 0
            else:
                i1, j1 = lower_left
                if not 0 < i1 < self.getNX():
                    raise ValueError("lower_left i coordinate invalid")

                if not 0 < j1 < self.getNY():
                    raise ValueError("lower_left j coordinate invalid")

            if upper_right is None:
                i2 = self.getNX()
                j2 = self.getNY()
            else:
                i2, j2 = upper_right

                if not 1 < i2 <= self.getNX():
                    raise ValueError("upper_right i coordinate invalid")

                if not 1 < j2 <= self.getNY():
                    raise ValueError("upper_right j coordinate invalid")

            if not i1 < i2:
                raise ValueError("Must have lower_left < upper_right")

            if not j1 < j2:
                raise ValueError("Must have lower_left < upper_right")

            x, y, _ = _grid._get_corner_xyz(self, i1, j1, layer)
            p0 = (x, y)

            x, y, _ = _grid._get_corner_xyz(self, i2, j1, layer)
            p1 = (x, y)

            x, y, _ = _grid._get_corner_xyz(self, i2, j2, layer)
            p2 = (x, y)

            x, y, _ = _grid._get_corner_xyz(self, i1, j2, layer)
            p3 = (x, y)

            return (p0, p1, p2, p3)
        else:
            raise ValueError(
                "Invalid layer value:%d  Valid range: [0,%d]" % (layer, self.getNZ())
            )

    def get_name(self):
        """
        Name of the current grid, returns a string.

        For the main grid this is the filename given to the
        constructor when loading the grid; for an LGR this is the name
        of the LGR. If the grid instance has been created with the
        create() classmethod this can be None.
        """
        n = _grid._get_name(self)
        return str(n) if n else ""

    def cell(self, global_index=None, active_index=None, i=None, j=None, k=None):
        if global_index is not None:
            return Cell(self, global_index)
        if active_index is not None:
            return Cell(self, self.global_index(active_index=active_index))
        if i is not None:
            return Cell(self, self.global_index(ijk=(i, j, k)))

    def __getitem__(self, global_index):
        if isinstance(global_index, tuple):
            i, j, k = global_index
            return self.cell(i=i, j=j, k=k)
        return self.cell(global_index=global_index)

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def cells(self, active=False):
        """Iterator over all the (active) cells"""
        if not active:
            for c in self:
                yield c
        else:
            for i in range(self.get_num_active()):
                yield self.cell(active_index=i)

    def global_index(self, active_index=None, ijk=None):
        """
        Will convert either active_index or (i,j,k) to global index.
        """
        return self.__global_index(active_index=active_index, ijk=ijk)

    def __global_index(self, active_index=None, global_index=None, ijk=None):
        """
        Will convert @active_index or @ijk to global_index.

        This method will convert @active_index or @ijk to a global
        index. Exactly one of the arguments @active_index,
        @global_index or @ijk must be supplied.

        The method is used extensively internally in the Grid
        class; most methods which take coordinate input pass through
        this method to normalize the coordinate representation.
        """

        set_count = 0
        if not active_index is None:
            set_count += 1

        if not global_index is None:
            set_count += 1

        if ijk:
            set_count += 1

        if not set_count == 1:
            raise ValueError(
                "Exactly one of the keyword arguments active_index, global_index or ijk must be set"
            )

        if not active_index is None:
            global_index = _grid._get_global_index1A(self, active_index)
        elif ijk:
            nx = self.getNX()
            ny = self.getNY()
            nz = self.getNZ()

            i, j, k = ijk

            if not 0 <= i < nx:
                raise IndexError("Invalid value i:%d  Range: [%d,%d)" % (i, 0, nx))

            if not 0 <= j < ny:
                raise IndexError("Invalid value j:%d  Range: [%d,%d)" % (j, 0, ny))

            if not 0 <= k < nz:
                raise IndexError("Invalid value k:%d  Range: [%d,%d)" % (k, 0, nz))

            global_index = _grid._get_global_index3(self, i, j, k)
        else:
            if not 0 <= global_index < self.getGlobalSize():
                raise IndexError(
                    "Invalid value global_index:%d  Range: [%d,%d)"
                    % (global_index, 0, self.getGlobalSize())
                )
        return global_index

    def get_active_index(self, ijk=None, global_index=None):
        """
        Lookup active index based on ijk or global index.

        Will determine the active_index of a cell, based on either
        @ijk = (i,j,k) or @global_index. If the cell specified by the
        input arguments is not active the function will return -1.
        """
        gi = self.__global_index(global_index=global_index, ijk=ijk)
        return _grid._get_active_index1(self, gi)

    def get_active_fracture_index(self, ijk=None, global_index=None):
        """
        For dual porosity - get the active fracture index.
        """
        gi = self.__global_index(global_index=global_index, ijk=ijk)
        return _grid._get_active_fracture_index1(self, gi)

    def get_global_index1F(self, active_fracture_index):
        """
        Will return the global index corresponding to active fracture index.

        Returns None if there is no active_fracture with that index
        """
        result = _grid._get_global_index1F(self, active_fracture_index)
        if result == -1:
            return None
        return result

    def cell_invalid(self, ijk=None, global_index=None, active_index=None):
        """
        Tries to check if a cell is invalid.

        Cells which are used to represent numerical aquifers are
        typically located in UTM position (0,0); these cells have
        completely whacked up shape and size, and should **NOT** be
        used in calculations involving real world coordinates. To
        protect against this a heuristic is used identify such cells
        and mark them as invalid. There might be other sources than
        numerical aquifers to this problem.
        """
        gi = self.__global_index(
            global_index=global_index, ijk=ijk, active_index=active_index
        )
        return _grid._invalid_cell(self, gi)

    def valid_cell_geometry(self, ijk=None, global_index=None, active_index=None):
        """Checks if the cell has valid geometry.

        There are at least two reasons why a cell might have invalid
        gemetry:

          1. In the case of GRID files it is not necessary to supply
             the geometry for all the cells; in that case this
             function will return false for cells which do not have
             valid coordinates.

          2. Cells which are used to represent numerical aquifers are
             typically located in UTM position (0,0); these cells have
             completely whacked up shape and size; these cells are
             identified by a heuristic - which might fail

        If the validCellGeometry() returns false for a particular
        cell functions which calculate cell volumes, real world
        coordinates and so on - should not be used.
        """
        gi = self.__global_index(
            global_index=global_index, ijk=ijk, active_index=active_index
        )
        return _grid._valid_cell(self, gi)

    def active(self, ijk=None, global_index=None):
        """
        Is the cell active?

        See documentation og get_xyz() for explanation of parameters
        @ijk and @global_index.
        """
        gi = self.__global_index(global_index=global_index, ijk=ijk)
        active_index = _grid._get_active_index1(self, gi)
        if active_index >= 0:
            return True
        else:
            return False

    def get_global_index(self, ijk=None, active_index=None):
        """
        Lookup global index based on ijk or active index.
        """
        gi = self.__global_index(active_index=active_index, ijk=ijk)
        return gi

    def get_ijk(self, active_index=None, global_index=None):
        """
        Lookup (i,j,k) for a cell, based on either active index or global index.

        The return value is a tuple with three elements (i,j,k).
        """
        gi = self.__global_index(active_index=active_index, global_index=global_index)
        return _grid._get_ijk1(self, gi)

    def get_xyz(self, active_index=None, global_index=None, ijk=None):
        """
        Find true position of cell center.

        Will return world position of the center of a cell in the
        grid. The return value is a tuple of three elements:
        (utm_x, utm_y, depth).

        The cells of a grid can be specified in three different ways:

           (i,j,k)      : As a tuple of i,j,k values.

           global_index : A number in the range [0,nx*ny*nz). The
                          global index is related to (i,j,k) as:

                            global_index = i + j*nx + k*nx*ny

           active_index : A number in the range [0,nactive).

        For many of the Grid methods a cell can be specified using
        any of these three methods. Observe that one and only method is
        allowed:

        OK:
            pos1 = grid.get_xyz(active_index=100)
            pos2 = grid.get_xyz(ijk=(10,20,7))

        Crash and burn:
            pos3 = grid.get_xyz(ijk=(10,20,7), global_index=10)
            pos4 = grid.get_xyz()

        All the indices in the Grid() class are zero offset, this
        is in contrast to ECLIPSE which has an offset 1 interface.
        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )

        return _grid._get_xyz1(self, gi)

    def get_node_pos(self, i, j, k):
        """Will return the (x,y,z) for the node given by (i,j,k).

        Observe that this method does not consider cells, but the
        nodes in the grid. This means that the valid input range for
        i,j and k are are upper end inclusive. To get the four
        bounding points of the lower layer of the grid:

           p0 = grid.getNodePos(0, 0, 0)
           p1 = grid.getNodePos(grid.getNX(), 0, 0)
           p2 = grid.getNodePos(0, grid.getNY(), 0)
           p3 = grid.getNodePos(grid.getNX(), grid.getNY(), 0)

        """
        if not 0 <= i <= self.getNX():
            raise IndexError(
                "Invalid I value:%d - valid range: [0,%d]" % (i, self.getNX())
            )

        if not 0 <= j <= self.getNY():
            raise IndexError(
                "Invalid J value:%d - valid range: [0,%d]" % (j, self.getNY())
            )

        if not 0 <= k <= self.getNZ():
            raise IndexError(
                "Invalid K value:%d - valid range: [0,%d]" % (k, self.getNZ())
            )

        return _grid._get_corner_xyz(self, i, j, k)

    def get_cell_corner(
        self, corner_nr, active_index=None, global_index=None, ijk=None
    ):
        """
        Will look up xyz of corner nr @corner_nr


        lower layer:   upper layer

         2---3           6---7
         |   |           |   |
         0---1           4---5

        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        return _grid._get_cell_corner_xyz1(self, gi, corner_nr)

    def get_node_xyz(self, i, j, k):
        """
        This function returns the position of Vertex (i,j,k).

        The coordinates are in the inclusive interval [0,nx] x [0,ny] x [0,nz].
        """
        nx = self.getNX()
        ny = self.getNY()
        nz = self.getNZ()

        corner = 0

        if i == nx:
            i -= 1
            corner += 1

        if j == ny:
            j -= 1
            corner += 2

        if k == nz:
            k -= 1
            corner += 4

        if _grid._ijk_valid(self, i, j, k):
            return self.getCellCorner(corner, global_index=i + j * nx + k * nx * ny)
        else:
            raise IndexError("Invalid coordinates: (%d,%d,%d) " % (i, j, k))

    def get_layer_xyz(self, xy_corner, layer):
        nx = self.getNX()

        j, i = divmod(xy_corner, nx + 1)
        k = layer
        return self.getNodeXYZ(i, j, k)

    def distance(self, global_index1, global_index2):
        return _grid._get_distance(self, global_index1, global_index2)

    def depth(self, active_index=None, global_index=None, ijk=None):
        """
        Depth of the center of a cell.

        Returns the depth of the center of the cell given by
        @active_index, @global_index or @ijk. See method get_xyz() for
        documentation of @active_index, @global_index and @ijk.
        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        return _grid._get_depth(self, gi)

    def top(self, i, j):
        """
        Top of the reservoir; in the column (@i, @j).
        Returns average depth of the four top corners.
        """
        return _grid._get_top(self, i, j)

    def top_active(self, i, j):
        """
        Top of the active part of the reservoir; in the column (@i, @j).
        Raises ValueError if (i,j) column is inactive.
        """
        for k in range(self.getNZ()):
            a_idx = self.get_active_index(ijk=(i, j, k))
            if a_idx >= 0:
                return _grid._get_top1A(self, a_idx)
        raise ValueError("No active cell in column (%d,%d)" % (i, j))

    def bottom(self, i, j):
        """
        Bottom of the reservoir; in the column (@i, @j).
        """
        return _grid._get_bottom(self, i, j)

    def locate_depth(self, depth, i, j):
        """
        Will locate the k value of cell containing specified depth.

        Will scan through the grid column specified by the input
        arguments @i and @j and search for a cell containing the depth
        given by input argument @depth. The return value is the k
        value of cell containing @depth.

        If @depth is above the top of the reservoir the function will
        return -1, and if @depth is below the bottom of the reservoir
        the function will return -nz.
        """
        return _grid._locate_depth(self, depth, i, j)

    def find_cell(self, x, y, z, start_ijk=None):
        """
        Lookup cell containg true position (x,y,z).

        Will locate the cell in the grid which contains the true
        position (@x,@y,@z), the return value is as a triplet
        (i,j,k). The underlying C implementation is not veeery
        efficient, and can potentially take quite long time. If you
        provide a good intial guess with the parameter @start_ijk (a
        tuple (i,j,k)) things can speed up quite substantially.

        If the location (@x,@y,@z) can not be found in the grid, the
        method will return None.
        """
        start_index = 0
        if start_ijk:
            start_index = self.__global_index(ijk=start_ijk)

        global_index = _grid._get_ijk_xyz(self, x, y, z, start_index)
        if global_index >= 0:
            return _grid._get_ijk1(self, global_index)
        return None

    def cell_contains(self, x, y, z, active_index=None, global_index=None, ijk=None):
        """
        Will check if the cell contains point given by world
        coordinates (x,y,z).

        See method get_xyz() for documentation of @active_index,
        @global_index and @ijk.
        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        return _grid._cell_contains(self, gi, x, y, z)

    def find_cell_xy(self, x, y, k):
        """Will find the i,j of cell with utm coordinates x,y.

        The @k input is the layer you are interested in, the allowed
        values for k are [0,nz]. If the coordinates (x,y) are found to
        be outside the grid a ValueError exception is raised.
        """
        if 0 <= k <= self.getNZ():
            return _grid._get_ij_xy(self, x, y, k)
        else:
            raise IndexError("Invalid layer value:%d" % k)

    def find_cell_corner_xy(self, x, y, k):
        """Will find the corner nr of corner closest to utm coordinates x,y.

        The @k input is the layer you are interested in, the allowed
        values for k are [0,nz]. If the coordinates (x,y) are found to
        be outside the grid a ValueError exception is raised.
        """
        i, j = self.findCellXY(x, y, k)
        if k == self.getNZ():
            k -= 1
            corner_shift = 4
        else:
            corner_shift = 0

        nx = self.getNX()
        x0, y0, z0 = self.getCellCorner(corner_shift, ijk=(i, j, k))
        d0 = math.sqrt((x0 - x) * (x0 - x) + (y0 - y) * (y0 - y))
        c0 = i + j * (nx + 1)

        x1, y1, z1 = self.getCellCorner(1 + corner_shift, ijk=(i, j, k))
        d1 = math.sqrt((x1 - x) * (x1 - x) + (y1 - y) * (y1 - y))
        c1 = i + 1 + j * (nx + 1)

        x2, y2, z2 = self.getCellCorner(2 + corner_shift, ijk=(i, j, k))
        d2 = math.sqrt((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y))
        c2 = i + (j + 1) * (nx + 1)

        x3, y3, z3 = self.getCellCorner(3 + corner_shift, ijk=(i, j, k))
        d3 = math.sqrt((x3 - x) * (x3 - x) + (y3 - y) * (y3 - y))
        c3 = i + 1 + (j + 1) * (nx + 1)

        l = [(d0, c0), (d1, c1), (d2, c2), (d3, c3)]
        l.sort(key=lambda k: k[0])
        return l[0][1]

    def cell_regular(self, active_index=None, global_index=None, ijk=None):
        """
        The grid models often contain various degenerate cells,
        which are twisted, have overlapping corners or what not. This
        function gives a moderate sanity check on a cell, essentially
        what the function does is to check if the cell contains it's
        own centerpoint - which is actually not as trivial as it
        sounds.
        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        return _grid._cell_regular(self, gi)

    def cell_volume(self, active_index=None, global_index=None, ijk=None):
        """
        Calculate the volume of a cell.

        Will calculate the total volume of the cell. See method
        get_xyz() for documentation of @active_index, @global_index
        and @ijk.
        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        return _grid._get_cell_volume(self, gi)

    def cell_dz(self, active_index=None, global_index=None, ijk=None):
        """
        The thickness of a cell.

        Will calculate the (average) thickness of the cell. See method
        get_xyz() for documentation of @active_index, @global_index
        and @ijk.
        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        return _grid._get_cell_thickness(self, gi)

    def get_cell_dims(self, active_index=None, global_index=None, ijk=None):
        """Will return a tuple (dx,dy,dz) for cell dimension.

        The dx and dy values are best effor estimates of the cell size
        along the i and j directions respectively. The three values
        are guaranteed to satisfy:

              dx * dy * dz = dV

        See method get_xyz() for documentation of @active_index,
        @global_index and @ijk.

        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        dx = _grid._get_cell_dx(self, gi)
        dy = _grid._get_cell_dy(self, gi)
        dz = _grid._get_cell_thickness(self, gi)
        return (dx, dy, dz)

    def get_num_lgr(self):
        """
        How many LGRs are attached to this main grid?

        How many LGRs are attached to this main grid; the grid
        instance doing the query must itself be a main grid.
        """
        return _grid._num_lgr(self)

    def has_lgr(self, lgr_name):
        """
        Query if the grid has an LGR with name @lgr_name.
        """
        if _grid._has_named_lgr(self, lgr_name):
            return True
        else:
            return False

    def get_lgr(self, lgr_key):
        """Get Grid instance with LGR content.

        Return an Grid instance based on the LGR @lgr, the input
        argument can either be the name of an LGR or the grid number
        of the LGR. The LGR grid instance is mostly like an ordinary
        grid instance; the only difference is that it can not be used
        for further queries about LGRs.

        If the grid does not contain an LGR with this name/nr
        exception KeyError will be raised.

        """
        lgr = None
        if isinstance(lgr_key, int):
            if _grid._has_numbered_lgr(self, lgr_key):
                lgr = self._reference_from_ptr(_grid._get_numbered_lgr(self, lgr_key))
        else:
            if _grid._has_named_lgr(self, lgr_key):
                lgr = self._reference_from_ptr(_grid._get_named_lgr(self, lgr_key))

        if lgr is None:
            raise KeyError("No such LGR: %s" % lgr_key)

        lgr.setParent(self)
        return lgr

    def get_cell_lgr(self, active_index=None, global_index=None, ijk=None):
        """
        Get Grid instance located in cell.

        Will query the current grid instance if the cell given by
        @active_index, @global_index or @ijk has been refined with an
        LGR. Will return None if the cell in question has not been
        refined, the return value can be used for further queries.

        See get_xyz() for documentation of the input parameters.
        """
        gi = self.__global_index(
            ijk=ijk, active_index=active_index, global_index=global_index
        )
        lgr = self._reference_from_ptr(_grid._get_cell_lgr(self, gi))
        if lgr:
            return lgr
        else:
            raise IndexError("No LGR defined for this cell")

    def grid_value(self, kw, i, j, k):
        """
        Will evalute @kw in location (@i,@j,@k).

        The grid properties and solution vectors are stored in
        restart and init files as 1D vectors of length nx*nx*nz or
        nactive. The grid_value() method is a minor convenience
        function to convert the (@i,@j,@k) input values to an
        appropriate 1D index.

        Depending on the length of kw the input arguments are
        converted either to an active index or to a global index. If
        the length of kw does not fit with either the global size of
        the grid or the active size of the grid things will fail hard.
        """
        return _grid._grid_value(self, kw, i, j, k)

    def load_column(self, kw, i, j, column):
        """
        Load the values of @kw from the column specified by (@i,@j).

        The method will scan through all k values of the input field
        @kw for fixed values of i and j. The size of @kw must be
        either nactive or nx*ny*nz.

        The input argument @column should be a DoubleVector instance,
        observe that if size of @kw == nactive k values corresponding
        to inactive cells will not be modified in the @column
        instance; in that case it is important that @column is
        initialized with a suitable default value.
        """
        _grid._load_column(self, kw, i, j, column)

    def create_kw(self, array, kw_name, pack):
        """
        Creates an ResdataKW instance based on existing 3D numpy object.

        The method create3D() does the inverse operation; creating a
        3D numpy object from an ResdataKW instance. If the argument @pack
        is true the resulting keyword will have length 'nactive',
        otherwise the element will have length nx*ny*nz.
        """
        if array.ndim == 3:
            dims = array.shape
            if (
                dims[0] == self.getNX()
                and dims[1] == self.getNY()
                and dims[2] == self.getNZ()
            ):
                dtype = array.dtype
                if dtype == np.int32:
                    type = ResDataType.RD_INT
                elif dtype == np.float32:
                    type = ResDataType.RD_FLOAT
                elif dtype == np.float64:
                    type = ResDataType.RD_DOUBLE
                else:
                    sys.exit("Do not know how to create rd_kw from type:%s" % dtype)

                if pack:
                    size = self.getNumActive()
                else:
                    size = self.getGlobalSize()

                if len(kw_name) > 8:
                    # Silently truncate to length 8
                    kw_name = kw_name[0:8]

                kw = ResdataKW(kw_name, size, type)
                active_index = 0
                global_index = 0
                for k in range(self.getNZ()):
                    for j in range(self.getNY()):
                        for i in range(self.getNX()):
                            if pack:
                                if self.active(global_index=global_index):
                                    kw[active_index] = array[i, j, k]
                                    active_index += 1
                            else:
                                if dtype == np.int32:
                                    kw[global_index] = int(array[i, j, k])
                                else:
                                    kw[global_index] = array[i, j, k]

                            global_index += 1
                return kw
        raise ValueError("Wrong size / dimension on array")

    def coarse_groups(self):
        """
        Will return the number of coarse groups in this grid.
        """
        return _grid._num_coarse_groups(self)

    def in_coarse_group(self, global_index=None, ijk=None, active_index=None):
        """
        Will return True or False if the cell is part of coarse group.
        """
        global_index = self.__global_index(
            active_index=active_index, ijk=ijk, global_index=global_index
        )
        return _grid._in_coarse_group1(self, global_index)

    def create_3d(self, rd_kw, default=0):
        """
        Creates a 3D numpy array object with the data from  @rd_kw.

        Observe that 3D numpy object is a copy of the data in the
        ResdataKW instance, i.e. modification to the numpy object will not
        be reflected in the keyword.

        The methods createKW() does the inverse operation; creating an
        ResdataKW instance from a 3D numpy object.

        Alternative: Creating the numpy array object is not very
        efficient; if you only need a limited number of elements from
        the rd_kw instance it might be wiser to use the grid_value()
        method:

           value = grid.grid_value(rd_kw, i, j, k)

        """
        if len(rd_kw) == self.getNumActive() or len(rd_kw) == self.getGlobalSize():
            array = np.ones([self.getGlobalSize()], dtype=rd_kw.dtype) * default
            kwa = rd_kw.array
            if len(rd_kw) == self.getGlobalSize():
                for i in range(kwa.size):
                    array[i] = kwa[i]
            else:
                for global_index in range(self.getGlobalSize()):
                    active_index = _grid._get_active_index1(self, global_index)
                    array[global_index] = kwa[active_index]

            array = array.reshape([self.getNX(), self.getNY(), self.getNZ()], order="F")
            return array
        else:
            err_msg_fmt = 'Keyword "%s" has invalid size %d; must be either nactive=%d or nx*ny*nz=%d'
            err_msg = err_msg_fmt % (
                rd_kw,
                len(rd_kw),
                self.getNumActive(),
                self.getGlobalSize(),
            )
            raise ValueError(err_msg)

    def save_grdecl(self, pyfile, output_unit=UnitSystem.METRIC):
        """
        Will write the the grid content as grdecl formatted keywords.

        Will only write the main grid.
        """
        cfile = CFILE(pyfile)
        _grid._fprintf_grdecl2(self, cfile, int(output_unit))

    def save_EGRID(self, filename, output_unit=None):
        if output_unit is None:
            output_unit = self.unit_system
        _grid._fwrite_EGRID2(self, filename, int(output_unit))

    def save_GRID(self, filename, output_unit=UnitSystem.METRIC):
        """
        Will save the current grid as a GRID file.
        """
        _grid._fwrite_GRID2(self, filename, int(output_unit))

    def write_grdecl(self, rd_kw, pyfile, special_header=None, default_value=0):
        """
        Writes an ResdataKW instance as an ECLIPSE grdecl formatted file.

        The input argument @rd_kw must be an ResdataKW instance of size
        nactive or nx*ny*nz. If the size is nactive the inactive cells
        will be filled with @default_value; hence the function will
        always write nx*ny*nz elements.

        The data in the @rd_kw argument can be of type integer,
        float, double or bool. In the case of bool the default value
        must be specified as 1 (True) or 0 (False).

        The input argument @pyfile should be a valid python filehandle
        opened for writing; i.e.

           pyfile = open("PORO.GRDECL", "w")
           grid.write_grdecl(poro_kw , pyfile, default_value=0.0)
           grid.write_grdecl(permx_kw, pyfile, default_value=0.0)
           pyfile.close()

        """

        if len(rd_kw) == self.getNumActive() or len(rd_kw) == self.getGlobalSize():
            cfile = CFILE(pyfile)
            _grid._fwrite_grdecl(self, rd_kw, special_header, cfile, default_value)
        else:
            raise ValueError(
                "Keyword: %s has invalid size(%d), must be either nactive:%d  or nx*ny*nz:%d"
                % (
                    rd_kw.getName(),
                    len(rd_kw),
                    self.getNumActive(),
                    self.getGlobalSize(),
                )
            )

    def exportACTNUM(self):
        return IntVector.createPythonObject(_grid._init_actnum(self))

    def compressed_kw_copy(self, kw):
        if len(kw) == self.getNumActive():
            return kw.copy()
        elif len(kw) == self.getGlobalSize():
            kw_copy = ResdataKW(kw.getName(), self.getNumActive(), kw.data_type)
            _grid._compressed_kw_copy(self, kw_copy, kw)
            return kw_copy
        else:
            raise ValueError(
                "The input keyword must have nx*n*nz or nactive elements. Size:%d invalid"
                % len(kw)
            )

    def global_kw_copy(self, kw, default_value):
        if len(kw) == self.getGlobalSize():
            return kw.copy()
        elif len(kw) == self.getNumActive():
            kw_copy = ResdataKW(kw.getName(), self.getGlobalSize(), kw.data_type)
            kw_copy.assign(default_value)
            _grid._global_kw_copy(self, kw_copy, kw)
            return kw_copy
        else:
            raise ValueError(
                "The input keyword must have nx*n*nz or nactive elements. Size:%d invalid"
                % len(kw)
            )

    def export_ACTNUM_kw(self):
        return ResdataKW.createPythonObject(_grid._init_actnum_kw(self))

    def create_volume_keyword(self, active_size=True):
        """Will create a ResdataKW initialized with cell volumes.

        The purpose of this method is to create a ResdataKW instance which
        is initialized with all the cell volumes, this can then be
        used to perform volume summation; i.e. to calculate the total
        oil volume:

           soil = 1 - sgas - swat
           cell_volume = grid.createVolumeKeyword()
           tmp = cell_volume * soil
           oip = tmp.sum()

        The oil in place calculation shown above could easily be
        implemented by iterating over the soil kw, however using the
        volume keyword has two advantages:

          1. The calculation of cell volumes is quite time consuming,
             by storing the results in a kw they can be reused.

          2. By using the compact form 'oip = cell_volume * soil' the
             inner loop iteration will go in C - which is faster.

        By default the kw will only have values for the active cells,
        but by setting the optional variable @active_size to False you
        will get volume values for all cells in the grid.
        """

        return ResdataKW.createPythonObject(
            _grid._create_volume_keyword(self, active_size)
        )

    def export_index(self, active_only=False):
        """
        Exports a pandas dataframe containing index data of grid cells.

        The global_index of the cells is used as index in the pandas frame.
        columns 0, 1, 2 are i, j, k, respectively
        column 3 contains the active_index
        if active_only == True, only active cells are listed,
        otherwise all cells are listed.
        This index frame should typically be passed to the epxport_data(),
        export_volume() and export_corners() functions.
        """
        if active_only:
            size = self.get_num_active()
        else:
            size = self.get_global_size()
        indx, data = _grid._export_index_frame(self, active_only)
        return pd.DataFrame(data=data, index=indx, columns=["i", "j", "k", "active"])

    def export_data(self, index_frame, kw, default=0):
        """
        Exports keywoard data to a numpy vector.

        Index_fram must be a pandas dataframe with the same structure
        as obtained from export_index.
        kw must have size of either global_size or num_active.
        The length of the numpy vector is the number of rows in index_frame.
        If kw is of length num_active, values in the output vector
        corresponding to inactive cells are set to default.
        """
        if not isinstance(index_frame, pd.DataFrame):
            raise TypeError("index_frame must be pandas.DataFrame")
        if len(kw) == self.get_global_size():
            index = index_frame.index.to_numpy(dtype=np.int32, copy=True)
        elif len(kw) == self.get_num_active():
            index = index_frame["active"].to_numpy(dtype=np.int32, copy=True)
        else:
            raise ValueError("The keyword must have a 3D compatible length")

        if kw.type is ResdataTypeEnum.RD_INT_TYPE:
            data = np.full(len(index), default, dtype=np.int32)
            _grid._export_data_as_int(len(index), index, kw, data)
            return data
        elif (
            kw.type is ResdataTypeEnum.RD_FLOAT_TYPE
            or kw.type is ResdataTypeEnum.RD_DOUBLE_TYPE
        ):
            data = np.full(len(index), default, dtype=np.float64)
            _grid._export_data_as_double(len(index), index, kw, data)
            return data
        else:
            raise TypeError("Keyword must be either int, float or double.")

    def export_volume(self, index_frame):
        """
        Exports cell volume data to a numpy vector.

        Index_fram must be a pandas dataframe with the same structure
        as obtained from export_index.
        """
        if not isinstance(index_frame, pd.DataFrame):
            raise TypeError("index_frame must be pandas.DataFrame")
        index = index_frame.index.to_numpy(dtype=np.int32, copy=True)
        return _grid._export_volume(self, index)

    def export_position(self, index_frame):
        """Exports cell position coordinates to a numpy vector (matrix), with columns
        0, 1, 2 denoting coordinates x, y, and z, respectively.

        Index_fram must be a pandas dataframe with the same structure
        as obtained from export_index.
        """
        if not isinstance(index_frame, pd.DataFrame):
            raise TypeError("index_frame must be pandas.DataFrame")
        index = index_frame.index.to_numpy(dtype=np.int32, copy=True)
        return _grid._export_position(self, index)

    def export_corners(self, index_frame):
        """Exports cell corner position coordinates to a numpy vector (matrix).

        Index_fram must be a pandas dataframe with the same structure
        as obtained from export_index.
        Example of a row of the output matrix:
        0   1   2  ....   21   22   23
        x1  y1  z1 ....   x8   y8   z8

        In total there are eight 8 corners. They are described as follows:
        The corners in a cell are numbered 0 - 7, where corners 0-3 constitute
        one layer and the corners 4-7 consitute the other layer. Observe
        that the numbering does not follow a consistent rotation around the face:


                                        j
        6---7                        /|\\
        |   |                         |
        4---5                         |
                                      |
                                      o---------->  i
        2---3
        |   |
        0---1

        Many grids are left-handed, i.e. the direction of increasing z will
        point down towards the center of the earth. Hence in the figure above
        the layer 4-7 will be deeper down in the reservoir than layer 0-3, and
        also have higher z-value.

        Warning: The main author of this code suspects that the coordinate
        system can be right-handed as well, giving a z axis which will
        increase 'towards the sky'; the safest way is probably to check this
        explicitly if it matters for the case at hand.
        """
        if not isinstance(index_frame, pd.DataFrame):
            raise TypeError("index_frame must be pandas.DataFrame")
        index = index_frame.index.to_numpy(dtype=np.int32, copy=True)
        return _grid._export_corners(self, index)

    def export_coord(self):
        return ResdataKW.createPythonObject(_grid._export_coord(self))

    def export_zcorn(self):
        return ResdataKW.createPythonObject(_grid._export_zcorn(self))

    def export_actnum(self):
        return ResdataKW.createPythonObject(_grid._export_actnum(self))

    def export_mapaxes(self):
        if not _grid._use_mapaxes(self):
            return None

        return ResdataKW.createPythonObject(_grid._export_mapaxes(self))

    @property
    def unit_system(self):
        return UnitSystem(_grid._get_unit_system(self))


monkey_the_camel(Grid, "loadFromGrdecl", Grid.load_from_grdecl, classmethod)
monkey_the_camel(Grid, "loadFromFile", Grid.load_from_file, classmethod)
monkey_the_camel(Grid, "createRectangular", Grid.create_rectangular, classmethod)
monkey_the_camel(Grid, "dualGrid", Grid.dual_grid)
monkey_the_camel(Grid, "getDims", Grid.get_dims)
monkey_the_camel(Grid, "getNX", Grid.get_nx)
monkey_the_camel(Grid, "getNY", Grid.get_ny)
monkey_the_camel(Grid, "getNZ", Grid.get_nz)
monkey_the_camel(Grid, "getGlobalSize", Grid.get_global_size)
monkey_the_camel(Grid, "getNumActive", Grid.get_num_active)
monkey_the_camel(Grid, "getNumActiveFracture", Grid.get_num_active_fracture)
monkey_the_camel(Grid, "getBoundingBox2D", Grid.get_bounding_box_2d)
monkey_the_camel(Grid, "getName", Grid.get_name)
monkey_the_camel(Grid, "validCellGeometry", Grid.valid_cell_geometry)
monkey_the_camel(Grid, "getNodePos", Grid.get_node_pos)
monkey_the_camel(Grid, "getCellCorner", Grid.get_cell_corner)
monkey_the_camel(Grid, "getNodeXYZ", Grid.get_node_xyz)
monkey_the_camel(Grid, "getLayerXYZ", Grid.get_layer_xyz)
monkey_the_camel(Grid, "findCellXY", Grid.find_cell_xy)
monkey_the_camel(Grid, "findCellCornerXY", Grid.find_cell_corner_xy)
monkey_the_camel(Grid, "getCellDims", Grid.get_cell_dims)
monkey_the_camel(Grid, "getNumLGR", Grid.get_num_lgr)
monkey_the_camel(Grid, "createKW", Grid.create_kw)
monkey_the_camel(Grid, "create3D", Grid.create_3d)
monkey_the_camel(Grid, "compressedKWCopy", Grid.compressed_kw_copy)
monkey_the_camel(Grid, "globalKWCopy", Grid.global_kw_copy)
monkey_the_camel(Grid, "exportACTNUMKw", Grid.export_ACTNUM_kw)
monkey_the_camel(Grid, "createVolumeKeyword", Grid.create_volume_keyword)
