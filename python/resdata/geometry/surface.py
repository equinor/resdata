"""
Create a polygon
"""

import os.path

from cwrap import BaseCClass

import resdata.geometry._surface as _surface
from resdata.geometry import GeoPointset


class Surface(BaseCClass):
    TYPE_NAME = "rd_surface"

    def __init__(
        self,
        filename=None,
        nx=None,
        ny=None,
        xinc=None,
        yinc=None,
        xstart=None,
        ystart=None,
        angle=None,
    ):
        """
        This will load a irap surface from file. The surface should
        consist of a header and a set z values.
        """
        if filename is not None:
            filename = str(filename)
            if os.path.isfile(filename):
                c_ptr = _surface._alloc(filename, True)
                super().__init__(c_ptr)
            else:
                raise OSError('No such file "%s".' % filename)
        else:
            s_args = [nx, ny, xinc, yinc, xstart, ystart, angle]
            if None in s_args:
                raise ValueError(
                    "Missing argument for creating surface, all values must be set, was: %s"
                    % str(s_args)
                )
            c_ptr = _surface._new(*s_args)
            super().__init__(c_ptr)

    def __eq__(self, other):
        """
        Compares two Surface instances, both header and data must be equal
        to compare as equal.
        """
        if isinstance(other, Surface):
            return _surface._equal(self, other)
        else:
            return False

    def headerEqual(self, other):
        return _surface._header_equal(self, other)

    def __iadd__(self, other):
        if isinstance(other, Surface):
            if self.headerEqual(other):
                _surface._iadd(self, other)
            else:
                raise ValueError("Tried to add incompatible surfaces")
        else:
            _surface._shift(self, other)
        return self

    def __isub__(self, other):
        if isinstance(other, Surface):
            if self.headerEqual(other):
                _surface._isub(self, other)
            else:
                raise ValueError("Tried to subtract incompatible surfaces")
        else:
            _surface._shift(self, -other)
        return self

    def __imul__(self, other):
        if isinstance(other, Surface):
            if self.headerEqual(other):
                _surface._imul(self, other)
            else:
                raise ValueError("Tried to multiply incompatible surfaces")
        else:
            _surface._scale(self, other)
        return self

    def __itruediv__(self, other):
        _surface._scale(self, 1.0 / other)
        return self

    def __idiv__(self, other):
        return self.__itruediv__(other)

    def __add__(self, other):
        copy = self.copy()
        copy += other
        return copy

    def __mul__(self, other):
        copy = self.copy()
        copy *= other
        return copy

    def __sub__(self, other):
        copy = self.copy()
        copy -= other
        return copy

    def __truediv__(self, other):
        copy = self.copy()
        copy /= other
        return copy

    def __div__(self, other):
        return self.__truediv__(other)

    def __len__(self):
        """
        The number of values in the surface.
        """
        return self.getNX() * self.getNY()

    def inplaceSqrt(self):
        """
        Will do an inplace sqrt operation.
        """
        _surface._isqrt(self)
        return self

    def sqrt(self):
        """
        Will return a new surface where all elements have been sqrt{ .. }.
        """
        copy = self.copy()
        copy.inplaceSqrt()
        return copy

    def copy(self, copy_data=True):
        """Will create a deep copy of self, if copy_data is set to False the
        copy will have all z-values set to zero.
        """
        return Surface.createPythonObject(_surface._copy(self, copy_data))

    def write(self, filename):
        """
        Will write the surface as an ascii formatted file to @filename.
        """
        _surface._write(self, filename)

    def assign(self, value):
        """
        Will set all the values in the surface to @value"
        """
        _surface._assign(self, value)

    def __setitem__(self, index, value):
        if isinstance(index, int):
            if index >= len(self):
                raise IndexError(
                    "Invalid index:%d - valid range [0,%d)" % (index, len(self))
                )
            if index < 0:
                index += len(self)

            _surface._iset_zvalue(self, index, value)
        else:
            raise TypeError("Invalid index type:%s - must be integer" % index)

    def __getitem__(self, index):
        if isinstance(index, int):
            idx = index
            ls = len(self)
            if idx < 0:
                idx += ls
            if 0 <= idx < ls:
                return _surface._iget_zvalue(self, idx)
            else:
                raise IndexError(
                    "Invalid index:%d - valid range [0,%d)" % (index, len(self))
                )
        else:
            raise TypeError("Invalid index type:%s - must be integer" % index)

    def getXY(self, index):
        """Gets the index'th (x,y) coordinate"""
        if isinstance(index, int):
            idx = index
            if idx < 0:
                idx += len(self)
            if not 0 <= idx < len(self):
                raise IndexError(
                    "Invalid index:%d - valid range [0,%d)" % (index, len(self))
                )
            index = idx
        else:
            raise TypeError("Invalid index type:%s - must be integer" % index)

        return _surface._iget_xy(self, index)

    def getNX(self):
        return _surface._get_nx(self)

    def getNY(self):
        return _surface._get_ny(self)

    def getPointset(self) -> GeoPointset:
        return _surface._get_pointset(self).setParent(self)

    def _assert_idx_or_i_and_j(self, idx, i, j):
        if idx is None:
            if i is None or j is None:
                raise ValueError(
                    "idx is None, i and j must be ints, was %s and %s." % (i, j)
                )
        else:
            if i is not None or j is not None:
                raise ValueError(
                    "idx is set, i and j must be None, was %s and %s." % (i, j)
                )

    def getXYZ(self, idx=None, i=None, j=None):
        """Returns a tuple of 3 floats, (x,y,z) for given global index, or i and j."""
        self._assert_idx_or_i_and_j(idx, i, j)
        if idx is None:
            nx, ny = self.getNX(), self.getNY()
            i_idx, j_idx = i, j
            if i_idx < 0:
                i_idx += self.getNX()
            if j_idx < 0:
                j_idx += self.getNY()
            if 0 <= i_idx < self.getNX() and 0 <= j_idx < self.getNY():
                idx = j_idx * self.getNX() + i_idx
            else:
                fmt = "Index error: i=%d not in [0,nx=%d) or j=%d not in [0,ny=%d)."
                raise IndexError(fmt % (i, nx, j, ny))
        x, y = self.getXY(idx)
        z = self[idx]
        return (x, y, z)

    def free(self):
        _surface._free(self)

    def __repr__(self):
        cnt = "nx=%d, ny=%d" % (self.getNX(), self.getNY())
        return self._create_repr(cnt)
