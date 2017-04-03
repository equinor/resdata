#  Copyright (C) 2017  Statoil ASA, Norway.
#
#  This file is part of ERT - Ensemble based Reservoir Tool.
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
from cwrap import BaseCClass
from ert.util import IntVector
from ert.geo import GeoPrototype

class GeoRegion(BaseCClass):
    TYPE_NAME = "geo_region"

    _alloc = GeoPrototype("void* geo_region_alloc(geo_pointset, bool)", bind=False)
    _free  = GeoPrototype("void  geo_region_free(geo_region)")
    _reset = GeoPrototype("void  geo_region_reset(geo_region)")
    #_select_inside_polygon    = GeoPrototype("void geo_region_select_inside_polygon(geo_region,  geo_polygon)")
    #_select_outside_polygon   = GeoPrototype("void geo_region_select_outside_polygon(geo_region,  geo_polygon)")
    #_deselect_inside_polygon  = GeoPrototype("void geo_region_deselect_inside_polygon(geo_region,  geo_polygon)")
    #_deselect_outside_polygon = GeoPrototype("void geo_region_deselect_outside_polygon(geo_region,  geo_polygon)")
    #_select_above_line        = GeoPrototype("void geo_region_select_above_line(geo_region, const double xcoords[2], const double ycoords[2])")
    #_select_below_line        = GeoPrototype("void geo_region_select_below_line(geo_region, const double xcoords[2], const double ycoords[2])")
    #_deselect_above_line      = GeoPrototype("void geo_region_deselect_above_line(geo_region, const double xcoords[2], const double ycoords[2])")
    #_deselect_below_line      = GeoPrototype("void geo_region_deselect_below_line(geo_region, const double xcoords[2], const double ycoords[2])")
    _get_index_list           = GeoPrototype("int_vector_ref geo_region_get_index_list(geo_region)")


    def __init__(self, pointset, preselect=False):
        self._preselect = True if preselect else False
        c_ptr = self._alloc(pointset, self._preselect)
        if c_ptr:
            super(GeoRegion, self).__init__(c_ptr)
        else:
            raise ValueError('Could not construct GeoRegion from pointset %s.' % pointset)

    def getActiveList(self):
        return self._get_index_list()


    def __len__(self):
        """Returns the size of the active list, not the size of the
        underlying pointset"""
        return len(self._get_index_list())

    def __repr__(self):
        ls = len(self)
        il = repr(self.getActiveList())
        pres = 'preselected' if self._preselect else 'not preselected'
        return self._create_repr('size=%d, active_list=<%s>, %s' % (ls, il, pres))

    def free(self):
        self._free()
