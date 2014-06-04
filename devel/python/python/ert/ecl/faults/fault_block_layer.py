#  Copyright (C) 2014  Statoil ASA, Norway. 
#   
#  The file 'fault_block_layer.py' is part of ERT - Ensemble based Reservoir Tool. 
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


class FaultBlockLayer(BaseCClass):

    def __init__(self , grid , fault_block_kw , k):
        c_pointer = self.cNamespace().alloc( grid , fault_block_kw , k)
        if c_pointer:
            super(FaultBlockLayer, self).__init__(c_pointer)
        else:
            raise ValueError("Invalid input - failed to create FaultBlockLayer")

        # The underlying C implementation uses lazy evaluation and needs to hold on
        # to the grid and kw references. We therefor take references to them here, to protect
        # against premature garbage collection.
        self.grid_ref = grid
        self.kw_ref = fault_block_kw


    def __len__(self):
        return self.cNamespace().size(self)
        

    def __getitem__(self , index):
        """
        @rtype: FaultBlock
        """
        if isinstance(index, int):
            if index < 0:
                index += len(self)
                
            if 0 <= index < len(self):
                return self.cNamespace().iget_block( self , index ).setParent(self)
            else:
                raise IndexError("Index:%d out of range: [0,%d)" % (index , len(self)))
        else:
            raise TypeError("Index should be integer type")

    def __contains__(self , block_id):
        return self.cNamespace().has_block( self , block_id)


    def getBlock(self , block_id):
        """
        @rtype: FaultBlock
        """
        if block_id in self:
            return self.cNamespace().get_block( self , block_id).setParent(self)
        else:
            raise KeyError("No blocks with ID:%d in this layer" % block_id)


    def deleteBlock(self , block_id):
        if block_id in self:
            self.cNamespace().del_block( self , block_id)
        else:
            raise KeyError("No blocks with ID:%d in this layer" % block_id)

    def addBlock(self , block_id):
        if block_id in self:
            raise KeyError("Layer already contains block with ID:%s" % block_id)
        else:
            return self.cNamespace().add_block( self , block_id).setParent(self)
            

    def free(self):
        self.cNamespace().free(self)



    

cwrapper = CWrapper(ECL_LIB)
CWrapper.registerObjectType("fault_block_layer", FaultBlockLayer)


FaultBlockLayer.cNamespace().alloc      = cwrapper.prototype("c_void_p         fault_block_layer_alloc(ecl_grid , ecl_kw , int)")
FaultBlockLayer.cNamespace().free       = cwrapper.prototype("void             fault_block_layer_free(fault_block_layer)")
FaultBlockLayer.cNamespace().size       = cwrapper.prototype("int              fault_block_layer_get_size(fault_block_layer)")
FaultBlockLayer.cNamespace().iget_block = cwrapper.prototype("fault_block_ref  fault_block_layer_iget_block(fault_block_layer, int)")
FaultBlockLayer.cNamespace().add_block  = cwrapper.prototype("fault_block_ref  fault_block_layer_add_block(fault_block_layer, int)")
FaultBlockLayer.cNamespace().get_block  = cwrapper.prototype("fault_block_ref  fault_block_layer_get_block(fault_block_layer, int)")
FaultBlockLayer.cNamespace().del_block  = cwrapper.prototype("void  fault_block_layer_del_block(fault_block_layer, int)")
FaultBlockLayer.cNamespace().has_block  = cwrapper.prototype("bool  fault_block_layer_has_block(fault_block_layer, int)")
