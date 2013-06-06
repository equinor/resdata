#  Copyright (C) 2011  Statoil ASA, Norway. 
#   
#  The file 'ecl_rft_cell.py' is part of ERT - Ensemble based Reservoir Tool. 
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


import libecl
import ctypes
import types
from   ert.cwrap.cwrap       import *
from   ert.cwrap.cclass      import CClass

RFT = 1
PLT = 2

class RFTCell(CClass):

    def get_i(self):
        return cfunc.get_i( self )

    def get_j(self):
        return cfunc.get_j( self )

    def get_k(self):
        return cfunc.get_k( self )

    @property
    def pressure(self):
        return cfunc.get_pressure( self )

    @property
    def depth(self):
        return cfunc.get_depth( self )


#################################################################


class EclRFTCell(RFTCell):
    
    @classmethod
    def new(self , i , j , k , depth , pressure , swat , sgas ):
        cell = EclRFTCell()
        c_ptr = cfunc.alloc_RFT( i,j,k,depth,pressure,swat,sgas)
        cell.init_cobj( c_ptr , cfunc.free )
        return cell

    @property
    def swat(self):
        return cfunc.get_swat( self )

    @property
    def sgas(self):
        return cfunc.get_sgas( self )

    @property
    def soil(self):
        return 1 - (cfunc.get_sgas( self ) + cfunc.get_swat( self ))
    
    
#################################################################


class EclPLTCell(RFTCell):

    @classmethod
    def new(self , i , j , k , depth , pressure , orat , grat , wrat , conn_start , flowrate , oil_flowrate , gas_flowrate , water_flowrate ):
        cell = EclPLTCell()
        c_ptr = cfunc.alloc_PLT( i,j,k,depth,pressure,orat , grat , wrat , conn_start , flowrate , oil_flowrate , gas_flowrate , water_flowrate)
        cell.init_cobj( c_ptr , cfunc.free )
        return cell

    @property
    def orat(self):
        return cfunc.get_orat( self )

    @property
    def grat(self):
        return cfunc.get_grat( self )

    @property
    def wrat(self):
        return cfunc.get_wrat( self )

    @property
    def conn_start(self):
        return cfunc.get_conn_start( self )

    @property
    def flowrate(self):
        return cfunc.get_flowrate( self )

    @property
    def oil_flowrate(self):
        return cfunc.get_oil_flowrate( self )

    @property
    def gas_flowrate(self):
        return cfunc.get_gas_flowrate( self )

    @property
    def water_flowrate(self):
        return cfunc.get_water_flowrate( self )


#################################################################


cwrapper = CWrapper( libecl.lib )
cwrapper.registerType( "rft_cell"     , RFTCell)
cwrapper.registerType( "ecl_rft_cell" , EclRFTCell )
cwrapper.registerType( "ecl_plt_cell" , EclPLTCell )

cfunc = CWrapperNameSpace("ecl_rft_cell")

cfunc.alloc_RFT    = cwrapper.prototype("c_void_p ecl_rft_cell_alloc_RFT( int, int , int , double , double , double , double)")
cfunc.alloc_PLT    = cwrapper.prototype("c_void_p ecl_rft_cell_alloc_PLT( int, int , int , double , double , double , double, double , double , double , double , double , double )")
cfunc.free         = cwrapper.prototype("void ecl_rft_cell_free( rft_cell )")

cfunc.get_pressure = cwrapper.prototype("double ecl_rft_cell_get_pressure( rft_cell )")
cfunc.get_depth    = cwrapper.prototype("double ecl_rft_cell_get_depth( rft_cell )")
cfunc.get_i        = cwrapper.prototype("int ecl_rft_cell_get_i( rft_cell )")
cfunc.get_j        = cwrapper.prototype("int ecl_rft_cell_get_j( rft_cell )")
cfunc.get_k        = cwrapper.prototype("int ecl_rft_cell_get_k( rft_cell )")

cfunc.get_swat = cwrapper.prototype("double ecl_rft_cell_get_swat( ecl_rft_cell )")
cfunc.get_soil = cwrapper.prototype("double ecl_rft_cell_get_soil( ecl_rft_cell )")
cfunc.get_sgas = cwrapper.prototype("double ecl_rft_cell_get_sgas( ecl_rft_cell )")

cfunc.get_orat = cwrapper.prototype("double ecl_rft_cell_get_orat( ecl_plt_cell )")
cfunc.get_grat = cwrapper.prototype("double ecl_rft_cell_get_grat( ecl_plt_cell )")
cfunc.get_wrat = cwrapper.prototype("double ecl_rft_cell_get_wrat( ecl_plt_cell )")

cfunc.get_conn_start = cwrapper.prototype("double ecl_rft_cell_get_connection_start( ecl_plt_cell )")

cfunc.get_flowrate       = cwrapper.prototype("double ecl_rft_cell_get_flowrate( ecl_plt_cell )")
cfunc.get_oil_flowrate   = cwrapper.prototype("double ecl_rft_cell_get_oil_flowrate( ecl_plt_cell )")
cfunc.get_gas_flowrate   = cwrapper.prototype("double ecl_rft_cell_get_gas_flowrate( ecl_plt_cell )")
cfunc.get_water_flowrate = cwrapper.prototype("double ecl_rft_cell_get_water_flowrate( ecl_plt_cell )")


