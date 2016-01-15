#  Copyright (C) 2016  Statoil ASA, Norway. 
#   
#  The file 'surface' is part of ERT - Ensemble based Reservoir Tool. 
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
import os.path

from ert.cwrap import BaseCClass
from ert.geo import GeoPrototype


class Surface(BaseCClass):
    TYPE_NAME = "surface"

    _alloc       = GeoPrototype("void*  geo_surface_fload_alloc_irap( char* , bool )")
    _free        = GeoPrototype("void   geo_surface_free( surface )")
    _get_nx      = GeoPrototype("int    geo_surface_get_nx( surface )")
    _get_ny      = GeoPrototype("int    geo_surface_get_ny( surface )")
    _iget_zvalue = GeoPrototype("double geo_surface_iget_zvalue( surface , int)")
    _iset_zvalue = GeoPrototype("void   geo_surface_iset_zvalue( surface , int , double)")
    _write       = GeoPrototype("void   geo_surface_fprintf_irap( surface , char* )")
    _equal       = GeoPrototype("bool   geo_surface_equal( surface , surface )")

    
    def __init__(self, filename):
        """
        This will load a irap surface from file. The surface should
        consist of a header and a set z values.
        """
        if os.path.isfile( filename ):
            c_ptr = self._alloc(filename , True)
            super(Surface , self).__init__(c_ptr)
        else:
            raise IOError("No such file: %s" % filename)

    
    def __eq__(self , other):
        """
        Compares two Surface instances, both header and data must be equal
        to compare as equal.
        """
        if isinstance( other , Surface):
            return self._equal(self, other)
        else:
            return False

        

    def __len__(self):
        """
        The number of values in the surface.
        """
        return self.getNX() * self.getNY() 


    def write(self , filename):
        """
        Will write the surface as an ascii formatted file to @filename.
        """
        self._write( self , filename )

    
    def __setitem__(self , index , value):
        if isinstance(index , int):
            if index >= len(self):
                raise IndexError("Invalid index:%d - valid range [0,%d)" % (index , len(self)))
            if index < 0:
                index += len(self)

            self._iset_zvalue(self , index , value)
        else:
             raise TypeError("Invalid index type:%s - must be integer" % index)

         

    def __getitem__(self , index):
        if isinstance(index , int):
            if index >= len(self):
                raise IndexError("Invalid index:%d - valid range [0,%d)" % (index , len(self)))
            if index < 0:
                index += len(self)

            return self._iget_zvalue(self , index)
        else:
             raise TypeError("Invalid index type:%s - must be integer" % index)

    def getNX(self):
        return self._get_nx( self )


    def getNY(self):
        return self._get_ny( self )
        

    def free(self):
        self._free( self )
