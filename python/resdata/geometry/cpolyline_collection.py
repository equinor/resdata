"""
Create a polygon
"""

from cwrap import BaseCClass

import resdata.geometry._cpolyline_collection as _cpolyline_collection
from resdata.geometry import CPolyline


class CPolylineCollection(BaseCClass):
    TYPE_NAME = "rd_geo_polygon_collection"

    def __init__(self):
        c_ptr = _cpolyline_collection._alloc_new()
        super().__init__(c_ptr)
        self.parent_ref = None

    def __contains__(self, name):
        return _cpolyline_collection._has_polyline(self, name)

    def __len__(self):
        return _cpolyline_collection._size(self)

    def __iter__(self):
        index = 0

        while index < len(self):
            yield self[index]
            index += 1

    def __getitem__(self, index):
        if isinstance(index, int):
            if index < 0:
                index += len(self)

            if 0 <= index < len(self):
                return _cpolyline_collection._iget(self, index)
            else:
                raise IndexError(
                    "Invalid index:%d - valid range: [0,%d)" % (index, len(self))
                )
        elif isinstance(index, str):
            if index in self:
                return _cpolyline_collection._get(self, index)
            else:
                raise KeyError("No polyline named:%s" % index)
        else:
            raise TypeError("The index argument must be string or integer")

    def shallowCopy(self):
        copy = CPolylineCollection()
        for pl in self:
            _cpolyline_collection._add_polyline(copy, pl, False)

        # If we make a shallow copy we must ensure that source, owning
        # all the polyline objects does not go out of scope.
        copy.parent_ref = self
        return copy

    def addPolyline(self, polyline, name=None):
        if not isinstance(polyline, CPolyline):
            polyline = CPolyline(init_points=polyline, name=name)
        else:
            if name is not None:
                raise ValueError(
                    "The name keyword argument can only be supplied when add not CPOlyline object"
                )

        name = polyline.getName()
        if name and name in self:
            raise KeyError("The polyline collection already has an object:%s" % name)

        if polyline.isReference():
            _cpolyline_collection._add_polyline(self, polyline, False)
        else:
            polyline.convertToCReference(self)
            _cpolyline_collection._add_polyline(self, polyline, True)

    def createPolyline(self, name=None):
        if name and name in self:
            raise KeyError("The polyline collection already has an object:%s" % name)

        return _cpolyline_collection._create_polyline(self, name)

    def free(self):
        _cpolyline_collection._free(self)
