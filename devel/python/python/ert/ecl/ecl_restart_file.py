#  Copyright (C) 2015  Statoil ASA, Norway. 
#   
#  The file 'ecl_restart_file.py' is part of ERT - Ensemble based Reservoir Tool. 
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

from ert.util import CTime
from ert.ecl import ECL_LIB , EclFile, Ecl3DKW , Ecl3DFile
from ert.cwrap import CWrapper, BaseCClass

class EclRestartHead(BaseCClass):
    def __init__(self , kw_arg = None , rst_arg = None):
        if kw_arg is None and rst_arg is None:
            raise Exception("Invalid arguments")

        if not kw_arg is None:
            report_step , intehead_kw , doubhead_kw , logihead_kw = kw_arg
            c_ptr = EclRestartHead.cNamespace().alloc_from_kw( report_step , intehead_kw , doubhead_kw , logihead_kw )
        else:
            rst_file , occurence = rst_arg
            c_ptr = EclRestartHead.cNamespace().alloc( rst_file , occurence )

        super(EclRestartHead, self).__init__(c_ptr)

        
    def free(self):
        EclRestartHead.cNamespace().free( self )

    def getReportStep(self):
        return EclRestartHead.cNamespace().get_report_step( self )

    def getSimDate(self):
        ct = CTime( EclRestartHead.cNamespace().get_sim_time( self ) )
        return ct.datetime( )

    def getSimDays(self):
        return EclRestartHead.cNamespace().get_sim_days( self )
        
        

class EclRestartFile(Ecl3DFile):
    
    def __init__(self , grid , filename , flags = 0):
        super(EclRestartFile , self).__init__( grid, filename , flags)
        self.rst_headers = None

        if "SEQNUM" in self:
            self.is_unified = True
        else:
            self.is_unified = False

        
        
    def unified(self):
        return self.is_unified

    
    def assertHeaders(self):
        if self.rst_headers is None:
            self.rst_headers = []
            if self.unified():
                for index in range(self.num_named_kw("SEQNUM")):
                    self.rst_headers.append( EclRestartHead( rst_arg = (self , index )))
            else:
                file_type , report_step , name = EclFile.getFileType( self.getFilename() )
                intehead_kw = self["INTEHEAD"][0]
                doubhead_kw = self["DOUBHEAD"][0]
                if "LOGIHEAD" in self:
                    logihead_kw = self["LOGIHEAD"][0]
                else:
                    logihead_kw = None

                self.rst_headers.append( EclRestartHead( kw_arg = (report_step , intehead_kw , doubhead_kw , logihead_kw) ))
                
            
    def timeList(self):
        self.assertHeaders()
        time_list = []
        for header in self.rst_headers:
            time_list.append( (header.getReportStep() , header.getSimDate( ) , header.getSimDays( )) )

        return time_list

    

            
CWrapper.registerObjectType("ecl_rsthead", EclRestartHead)
cwrapper = CWrapper(ECL_LIB)
EclRestartHead.cNamespace().alloc           = cwrapper.prototype("c_void_p ecl_rsthead_ialloc(ecl_file , int )")
EclRestartHead.cNamespace().alloc_from_kw   = cwrapper.prototype("c_void_p ecl_rsthead_alloc_from_kw(int , ecl_kw , ecl_kw , ecl_kw )")
EclRestartHead.cNamespace().free            = cwrapper.prototype("void ecl_rsthead_free(ecl_rsthead)")
EclRestartHead.cNamespace().get_report_step = cwrapper.prototype("int ecl_rsthead_get_report_step(ecl_rsthead)")
EclRestartHead.cNamespace().get_sim_time    = cwrapper.prototype("time_t ecl_rsthead_get_sim_time(ecl_rsthead)")
EclRestartHead.cNamespace().get_sim_days    = cwrapper.prototype("double ecl_rsthead_get_sim_days(ecl_rsthead)")
            
