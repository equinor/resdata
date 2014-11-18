#  Copyright (C) 2011  Statoil ASA, Norway. 
#   
#  The file 'ecl_kw.py' is part of ERT - Ensemble based Reservoir Tool. 
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
"""
Create a polygon
"""
import ctypes
import os.path

from ert.cwrap import BaseCClass, CWrapper
from ert.geo import ERT_GEOMETRY_LIB


class CPolyline(BaseCClass):
    def __init__(self, name = None , init_points = []):
        c_ptr = CPolyline.cNamespace().alloc_new( name )
        super(CPolyline , self).__init__( c_ptr )
        for (xc, yc) in init_points:
            self.addPoint(xc, yc)


    @staticmethod 
    def createFromXYZFile(filename):
        if not os.path.isfile(filename):
            raise IOError("No such file:%s" % filename)
            
        return CPolyline.cNamespace().fread_alloc_irap( filename )


    def __len__(self):
        return CPolyline.cNamespace().size( self )


    def __getitem__(self , index):
        if not isinstance(index,int):
            raise TypeError("Index argument must be integer")

        if index < 0:
            index += len(self)

        if 0 <= index < len(self):
            x = ctypes.c_double()
            y = ctypes.c_double()
            CPolyline.cNamespace().iget_xy( self , index , ctypes.byref(x) , ctypes.byref(y) )
            
            return (x.value , y.value)
        else:
            raise IndexError("Invalid index:%d valid range: [0,%d)" % (index , len(self)))


    def segmentIntersects(self, p1 , p2):
        return CPolyline.cNamespace().segment_intersects(self , p1[0] , p1[0] , p2[0] , p2[1])
            

    def __iadd__(self , other ):
        for p in other:
            self.addPoint( p[0] , p[1] )
        return self


    def __add__(self , other ):
        copy = CPolyline( init_points = self)
        copy.__iadd__(other)
        return copy


    def __radd__(self , other ):
        copy = CPolyline( init_points = other )
        copy.__iadd__(self)
        return copy


            
    def addPoint( self, xc, yc , front = False):
        if front:
            CPolyline.cNamespace().add_point_front(self, xc, yc)
        else:
            CPolyline.cNamespace().add_point(self, xc, yc)


    def name(self):
        return CPolyline.cNamespace().get_name( self )


    def free(self):
        self.cNamespace().free(self)


#################################################################

cwrapper = CWrapper(ERT_GEOMETRY_LIB)
cwrapper.registerObjectType("geo_polygon", CPolyline)

CPolyline.cNamespace().alloc_new          = cwrapper.prototype("c_void_p        geo_polygon_alloc( char* )")
CPolyline.cNamespace().fread_alloc_irap   = cwrapper.prototype("geo_polygon_obj geo_polygon_fload_alloc_irap( char* )")
CPolyline.cNamespace().add_point          = cwrapper.prototype("void     geo_polygon_add_point( geo_polygon , double , double )")
CPolyline.cNamespace().add_point_front    = cwrapper.prototype("void     geo_polygon_add_point_front( geo_polygon , double , double )")
CPolyline.cNamespace().free               = cwrapper.prototype("void     geo_polygon_free( geo_polygon )")
CPolyline.cNamespace().size               = cwrapper.prototype("int      geo_polygon_get_size( geo_polygon )")
CPolyline.cNamespace().iget_xy            = cwrapper.prototype("void     geo_polygon_iget_xy( geo_polygon , int , double* , double* )")
CPolyline.cNamespace().segment_intersects = cwrapper.prototype("bool     geo_polygon_segment_intersects( geo_polygon , double , double, double , double)")
CPolyline.cNamespace().get_name           = cwrapper.prototype("char*    geo_polygon_get_name( geo_polygon  )")
