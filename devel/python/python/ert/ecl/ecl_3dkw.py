#  Copyright (C) 2015  Statoil ASA, Norway. 
#   
#  The file 'ecl_3dkw.py' is part of ERT - Ensemble based Reservoir Tool. 
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


from ecl_kw import EclKW

class Ecl3DKW(EclKW):
    """
    Class for working with Eclipse keywords defined over a grid
    """
    
    @classmethod
    def create(cls , kw , grid , value_type , default_value = 0 , global_active = False):
        if global_active:
            size = grid.getGlobalSize()
        else:
            size = grid.getNumActive( ) 

        new_kw = super(Ecl3DKW , cls).create( kw , size , value_type)
        new_kw.grid = grid
        new_kw.default_value = default_value
        new_kw.global_active = global_active
        return new_kw
        
        
        
    
    def __getitem__(self , index):
        """
        Will return item [g] or [i,j,k].
        """
        if isinstance(index , tuple):
            global_index = self.grid.get_global_index( ijk = index )
            if self.global_active:
                index = global_index
            else:
                if not self.grid.active( global_index = global_index):
                    return self.default_value
                else:
                    index = self.grid.get_active_index( ijk = index )
                
        
        return super(Ecl3DKW , self).__getitem__( index )




    def __setitem__(self , index , value):
        if isinstance(index , tuple):
            global_index = self.grid.get_global_index( ijk = index )
            if self.global_active:
                index = global_index
            else:
                if not self.grid.active( global_index = global_index):
                    raise ValueError("Tried to assign value to inactive cell: (%d,%d,%d)" % index)
                else:
                    index = self.grid.get_active_index( ijk = index )
                
        
        return super(Ecl3DKW , self).__setitem__( index , value )
            

    @classmethod
    def castFromKW(cls , kw , grid , default_value = 0):
        if len(kw) == grid.getGlobalSize():
            kw.global_active = True
        elif len(kw) == grid.getNumActive():
            kw.global_active = False
            kw.default_value = default_value
        else:
            raise ValueError("Size mismatch - must have size matching global/active size of grid")

            
        kw.__class__ = cls
        kw.default_value = default_value
        kw.grid = grid
        if len(kw) == grid.getGlobalSize():
            kw.global_active = True
        else:
            kw.global_active = False
            
        return kw


    def dims(self):
        return (self.grid.getNX() , self.grid.getNY() , self.grid.getNZ())
        
        
