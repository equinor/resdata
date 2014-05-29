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


from ert.cwrap import BaseCClass, CWrapper
from ert.ecl import ECL_LIB


class FaultBlock(BaseCClass):

    def __init__(self , grid , k , block_id):
        c_pointer = self.cNamespace().alloc( grid , k , block_id)
        super(FaultBlock, self).__init__(c_pointer)
# The underlying C implementation uses lazy evaluation and needs to hold on
# to the grid reference. We therefor take a reference to it here, to protect
# against premature garbage collection of the grid.
        self.grid_ref = grid


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



cwrapper = CWrapper(ECL_LIB)
CWrapper.registerObjectType("fault_block", FaultBlock)

FaultBlock.cNamespace().alloc      = cwrapper.prototype("c_void_p fault_block_alloc(ecl_grid , int)")
FaultBlock.cNamespace().free       = cwrapper.prototype("void     fault_block_free(fault_block)")
FaultBlock.cNamespace().add_cell   = cwrapper.prototype("void     fault_block_add_cell(fault_block , int , int)")
FaultBlock.cNamespace().get_xc     = cwrapper.prototype("double   fault_block_get_xc(fault_block)")
FaultBlock.cNamespace().get_yc     = cwrapper.prototype("double   fault_block_get_yc(fault_block)")
FaultBlock.cNamespace().get_block_id = cwrapper.prototype("int    fault_block_get_id(fault_block)")

