#  Copyright (C) 2013  Statoil ASA, Norway. 
#   
#  The file 'test_work_area.py' is part of ERT - Ensemble based Reservoir Tool. 
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

import libtest_util
from   ert.cwrap.cwrap  import *
from   ert.cwrap.cclass import CClass

class TestArea(CClass):

    def __init__(self , test_name , store_area = False):
        c_ptr = cfunc.test_area_allo( test_name , store_area)
        self.init_cobj(c_ptr , cfunc.free)


    def install_file( self , file ):
        cfunc.install_file( self , file )


    def copy_directory( self , directory ):
        cfunc.copy_directory( self , directory )


    def copy_directory_content( self , directory ):
        cfunc.copy_directory_content( self , directory )
        

CWrapper.registerType( "test_area" , TestArea )



cwrapper = CWrapper( libtest_util.lib )
cfunc    = CWrapperNameSpace("TestArea")    
cfunc.test_area_alloc = cwrapper.prototype("c_void_p test_work_area_alloc( char* , bool)")
cfunc.free            = cwrapper.prototype("void test_work_area_free( test_area )")
cfunc.install_file    = cwrapper.prototype("void test_work_area_install_file( test_area , char* )")
cfunc.copy_directory  = cwrapper.prototype("void test_work_area_copy_directory( test_area , char* )")
cfunc.copy_directory_content  = cwrapper.prototype("void test_work_area_copy_directory_content( test_area , char* )")
