"""
Create a polygon
"""
import ctypes

from cwrap import BaseCClass
from resdata import ResdataPrototype
from resdata.geometry import CPolyline


class CPolylineCollection(BaseCClass):
    TYPE_NAME = "rd_geo_polygon_collection"

    _alloc_new = ResdataPrototype("void* geo_polygon_collection_alloc(  )", bind=False)
    _free = ResdataPrototype(
        "void geo_polygon_collection_free(rd_geo_polygon_collection)"
    )
    _size = ResdataPrototype(
        "int geo_polygon_collection_size(rd_geo_polygon_collection)"
    )
    _create_polyline = ResdataPrototype(
        "rd_geo_polygon_ref geo_polygon_collection_create_polygon(rd_geo_polygon_collection, char*)"
    )
    _has_polyline = ResdataPrototype(
        "bool geo_polygon_collection_has_polygon(rd_geo_polygon_collection, char*)"
    )
    _iget = ResdataPrototype(
        "rd_geo_polygon_ref geo_polygon_collection_iget_polygon(rd_geo_polygon_collection, int)"
    )
    _get = ResdataPrototype(
        "rd_geo_polygon_ref geo_polygon_collection_get_polygon(rd_geo_polygon_collection, char*)"
    )
    _add_polyline = ResdataPrototype(
        "void geo_polygon_collection_add_polygon(rd_geo_polygon_collection, rd_geo_polygon, bool)"
    )

    def __init__(self):
        c_ptr = self._alloc_new()
        super(CPolylineCollection, self).__init__(c_ptr)
        self.parent_ref = None

    def __contains__(self, name):
        return self._has_polyline(name)

    def __len__(self):
        return self._size()

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
                return self._iget(index).setParent(self)
            else:
                raise IndexError(
                    "Invalid index:%d - valid range: [0,%d)" % (index, len(self))
                )
        elif isinstance(index, str):
            if index in self:
                return self._get(index)
            else:
                raise KeyError("No polyline named:%s" % index)
        else:
            raise TypeError("The index argument must be string or integer")

    def shallowCopy(self):
        copy = CPolylineCollection()
        for pl in self:
            copy._add_polyline(pl, False)

        # If we make a shallow copy we must ensure that source, owning
        # all the polyline objects does not go out of scope.
        copy.parent_ref = self
        return copy

    def addPolyline(self, polyline, name=None):
        if not isinstance(polyline, CPolyline):
            polyline = CPolyline(init_points=polyline, name=name)
        else:
            if not name is None:
                raise ValueError(
                    "The name keyword argument can only be supplied when add not CPOlyline object"
                )

        name = polyline.getName()
        if name and name in self:
            raise KeyError("The polyline collection already has an object:%s" % name)

        if polyline.isReference():
            self._add_polyline(polyline, False)
        else:
            polyline.convertToCReference(self)
            self._add_polyline(polyline, True)

    def createPolyline(self, name=None):
        if name and name in self:
            raise KeyError("The polyline collection already has an object:%s" % name)

        polyline = self._create_polyline(name)
        polyline.setParent(parent=self)
        return polyline

    def free(self):
        self._free()
