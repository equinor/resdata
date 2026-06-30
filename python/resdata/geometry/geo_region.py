from cwrap import BaseCClass

import resdata.geometry._geo_region as _geo_region
from resdata.util.util import IntVector

from .cpolyline import CPolyline


class GeoRegion(BaseCClass):
    TYPE_NAME = "rd_geo_region"

    def __init__(self, pointset, preselect=False):
        self._preselect = True if preselect else False
        c_ptr = _geo_region._alloc(pointset, self._preselect)
        if c_ptr:
            super().__init__(c_ptr)
        else:
            raise ValueError(
                "Could not construct GeoRegion from pointset %s." % pointset
            )

    def getActiveList(self) -> IntVector:
        return IntVector.createCReference(_geo_region._get_index_list(self), self)

    def _assert_polygon(self, polygon):
        if not isinstance(polygon, CPolyline):
            raise ValueError("Need to select with a CPolyline, not %s." % type(polygon))

    def _construct_cline(self, line):
        """Takes a line ((x1,y1), (x2,y2)) and returns two double[2]* but
        reordered to (x1x2, y1y2).
        """
        try:
            p1, p2 = line
            x1, y1 = map(float, p1)
            x2, y2 = map(float, p2)
        except Exception as err:
            err_msg = "Select with pair ((x1,y1), (x2,y2)), not %s (%s)."
            raise ValueError(err_msg % (line, err))
        return [x1, x2], [y1, y2]

    def select_inside(self, polygon):
        self._assert_polygon(polygon)
        _geo_region._select_inside_polygon(self, polygon)

    def select_outside(self, polygon):
        self._assert_polygon(polygon)
        _geo_region._select_outside_polygon(self, polygon)

    def deselect_inside(self, polygon):
        self._assert_polygon(polygon)
        _geo_region._deselect_inside_polygon(self, polygon)

    def deselect_outside(self, polygon):
        self._assert_polygon(polygon)
        _geo_region._deselect_outside_polygon(self, polygon)

    def select_above(self, line):
        x_ptr, y_ptr = self._construct_cline(line)
        _geo_region._select_above_line(self, x_ptr, y_ptr)

    def select_below(self, line):
        x_ptr, y_ptr = self._construct_cline(line)
        _geo_region._select_below_line(self, x_ptr, y_ptr)

    def deselect_above(self, line):
        x_ptr, y_ptr = self._construct_cline(line)
        _geo_region._deselect_above_line(self, x_ptr, y_ptr)

    def deselect_below(self, line):
        x_ptr, y_ptr = self._construct_cline(line)
        _geo_region._deselect_below_line(self, x_ptr, y_ptr)

    def __len__(self):
        """Returns the size of the active list, not the size of the
        underlying pointset"""
        return len(self.getActiveList())

    def __repr__(self):
        ls = len(self)
        il = repr(self.getActiveList())
        pres = "preselected" if self._preselect else "not preselected"
        return self._create_repr("size=%d, active_list=<%s>, %s" % (ls, il, pres))

    def free(self):
        _geo_region._free(self)
