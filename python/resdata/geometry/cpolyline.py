"""
Create a polygon
"""

import os.path

from cwrap import BaseCClass

import resdata.geometry._cpolyline as _cpolyline

from .geometry_tools import GeometryTools


class CPolyline(BaseCClass):
    TYPE_NAME = "rd_geo_polygon"

    def __init__(self, name=None, init_points=()):
        c_ptr = _cpolyline._alloc_new(name)
        super().__init__(c_ptr)
        for xc, yc in init_points:
            self.addPoint(xc, yc)

    @classmethod
    def createFromXYZFile(cls, filename, name=None):
        if not os.path.isfile(filename):
            raise OSError("No such file:%s" % filename)

        polyline = cls.createPythonObject(_cpolyline._fread_alloc_irap(filename))
        if name is not None:
            _cpolyline._set_name(polyline, name)
        return polyline

    def __str__(self):
        name = self.getName()
        if name:
            string = "%s [" % name
        else:
            string = "["

        for index, p in enumerate(self):
            string += "(%g,%g)" % p
            if index < len(self) - 1:
                string += ","
        string += "]"
        return string

    def __repr__(self):
        return str(self)

    def __len__(self):
        return _cpolyline._size(self)

    def __getitem__(self, index):
        if not isinstance(index, int):
            raise TypeError("Index argument must be integer. Index:%s invalid" % index)

        if index < 0:
            index += len(self)

        if 0 <= index < len(self):
            return _cpolyline._iget_xy(self, index)
        else:
            raise IndexError(
                "Invalid index:%d valid range: [0,%d)" % (index, len(self))
            )

    def segmentIntersects(self, p1, p2):
        return _cpolyline._segment_intersects(self, p1[0], p1[1], p2[0], p2[1])

    def intersects(self, polyline):
        if len(self) > 1:
            for index, p2 in enumerate(polyline):
                if index == 0:
                    continue

                p1 = polyline[index - 1]
                if self.segmentIntersects(p1, p2):
                    return True
        return False

    def __iadd__(self, other):
        for p in other:
            self.addPoint(p[0], p[1])
        return self

    def __add__(self, other):
        copy = CPolyline(init_points=self)
        copy.__iadd__(other)
        return copy

    def __radd__(self, other):
        copy = CPolyline(init_points=other)
        copy.__iadd__(self)
        return copy

    def __eq__(self, other):
        if not isinstance(other, CPolyline):
            return NotImplemented
        if super().__eq__(other):
            return True
        else:
            return _cpolyline._equal(self, other)

    def segmentLength(self):
        if len(self) == 0:
            raise ValueError("Can not measure length of zero point polyline")

        return _cpolyline._segment_length(self)

    def extendToBBox(self, bbox, start=True):
        if start:
            p0 = self[1]
            p1 = self[0]
        else:
            p0 = self[-2]
            p1 = self[-1]

        ray_dir = GeometryTools.lineToRay(p0, p1)
        intersections = GeometryTools.rayPolygonIntersections(p1, ray_dir, bbox)
        if intersections:
            p2 = intersections[0][1]
            if self.getName():
                name = "Extend:%s" % self.getName()
            else:
                name = None

            return CPolyline(name=name, init_points=[(p1[0], p1[1]), p2])
        else:
            raise ValueError("Logical error - must intersect with bounding box")

    def addPoint(self, xc, yc, front=False):
        if front:
            _cpolyline._add_point_front(self, xc, yc)
        else:
            _cpolyline._add_point(self, xc, yc)

    def getName(self):
        return _cpolyline._get_name(self)

    def free(self):
        _cpolyline._free(self)

    def unzip(self):
        x_list = []
        y_list = []
        for x, y in self:
            x_list.append(x)
            y_list.append(y)

        return (x_list, y_list)

    def unzip2(self):
        return self.unzip()

    def connect(self, target):
        end1 = self[0]
        end2 = self[-1]

        p1 = GeometryTools.nearestPointOnPolyline(end1, target)
        p2 = GeometryTools.nearestPointOnPolyline(end2, target)

        d1 = GeometryTools.distance(p1, end1)
        d2 = GeometryTools.distance(p2, end2)

        if d1 < d2:
            return [end1, p1]
        else:
            return [end2, p2]
