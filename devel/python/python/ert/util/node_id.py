#  Copyright (C) 2013  Statoil ASA, Norway. 
#   
#  The file 'node_id.py' is part of ERT - Ensemble based Reservoir Tool. 
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

from ctypes import *
#import  ctypes
from    ert.cwrap.cwrap       import *
from    ert.cwrap.cclass      import CClass
from    ert.ert.c_enums import *
import libutil
class NodeId(Structure): pass
NodeId._fields_ = [
    ("report_step", c_int),
    ("state", c_uint),
    ("iens", c_int)
    ]

    
cwrapper = CWrapper( libutil.lib )
cwrapper.registerType( "node_id" , NodeId )
