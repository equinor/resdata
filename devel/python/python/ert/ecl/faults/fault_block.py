#  Copyright (C) 2014  Statoil ASA, Norway. 
#   
#  The file 'fault_block.py' is part of ERT - Ensemble based Reservoir Tool. 
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

import ctypes
from ert.cwrap import BaseCClass, CWrapper
from ert.ecl import ECL_LIB


class FaultBlockCell(object):
    def __init__(self , i,j,k , x,y,z):

        self.i = i
        self.j = j
        self.k = k

        self.x = x
        self.y = y
        self.z = z

        
        



class FaultBlock(BaseCClass):

    def __init__(self , grid , k , block_id):
        c_pointer = self.cNamespace().alloc( grid , k , block_id)
        super(FaultBlock, self).__init__(c_pointer)
        # The underlying C implementation uses lazy evaluation and needs to hold on
        # to the grid reference. We therefor take a reference to it here, to protect
        # against premature garbage collection of the grid.
        self.grid_ref = grid

    def __getitem__(self , index):
        if isinstance(index, int):
            if index < 0:
                index += len(self)
                
            if 0 <= index < len(self):
                x = ctypes.c_double()
                y = ctypes.c_double()
                z = ctypes.c_double()

                i = ctypes.c_int()
                j = ctypes.c_int()
                k = ctypes.c_int()
                
                self.cNamespace().export_cell(self , index , ctypes.byref(i) , ctypes.byref(j) , ctypes.byref(k) , ctypes.byref(x) , ctypes.byref(y) , ctypes.byref(z))
                return FaultBlockCell( i.value , j.value , k.value , x.value , y.value , z.value )
            else:
                raise IndexError("Index:%d out of range: [0,%d)" % (index , len(self)))
        else:
            raise TypeError("Index:%s wrong type - integer expected")
        

    def __len__(self):
        return self.cNamespace().get_size( self )

    def free(self):
        self.cNamespace().free(self)

    def addCell(self , i , j ):
        self.cNamespace().add_cell( self , i , j )

    def getCentroid(self):
        xc = self.cNamespace().get_xc( self )
        yc = self.cNamespace().get_yc( self )
        return (xc,yc)

    def getBlockID(self):
        return self.cNamespace().get_block_id(self)


    def getGlobalIndexList(self):
        g_list = self.cNamespace().get_global_index_list(self)
        # We return a copy; i.e. it is not possible to update the
        # internal state of the fault block with this handle.
        return g_list.copy()


    def assignToRegion(self , region_id):
        self.cNamespace().assign_to_region(self , region_id)
        

    def getRegionList(self):
        regionList = self.cNamespace().get_region_list(self)
        return regionList.copy()



cwrapper = CWrapper(ECL_LIB)
CWrapper.registerObjectType("fault_block", FaultBlock)

FaultBlock.cNamespace().alloc                 = cwrapper.prototype("c_void_p fault_block_alloc(ecl_grid , int)")
FaultBlock.cNamespace().free                  = cwrapper.prototype("void     fault_block_free(fault_block)")
FaultBlock.cNamespace().add_cell              = cwrapper.prototype("void     fault_block_add_cell(fault_block , int , int)")
FaultBlock.cNamespace().get_xc                = cwrapper.prototype("double   fault_block_get_xc(fault_block)")
FaultBlock.cNamespace().get_yc                = cwrapper.prototype("double   fault_block_get_yc(fault_block)")
FaultBlock.cNamespace().get_block_id          = cwrapper.prototype("int    fault_block_get_id(fault_block)")
FaultBlock.cNamespace().get_size              = cwrapper.prototype("int      fault_block_get_size(fault_block)")
FaultBlock.cNamespace().get_global_index_list = cwrapper.prototype("int_vector_ref fault_block_get_global_index_list(fault_block)")
FaultBlock.cNamespace().export_cell           = cwrapper.prototype("void    fault_block_export_cell(fault_block , int , int* , int* , int* , double* , double* , double*)")
FaultBlock.cNamespace().assign_to_region      = cwrapper.prototype("void          fault_block_assign_to_region(fault_block , int)")
FaultBlock.cNamespace().get_region_list       = cwrapper.prototype("int_vector_ref  fault_block_get_region_list(fault_block)")
