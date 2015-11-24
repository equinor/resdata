#  Copyright (C) 2015  Statoil ASA, Norway. 
#   
#  The file 'ecl_cmp.py' is part of ERT - Ensemble based Reservoir Tool. 
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

from ert.ecl import EclSum


class EclCase(object):
    def __init__(self , case):
        self.case = case

        self.grid = None
        self.restart = None
        self.init = None
        self.summary = None
        
        self.loadSummary( )


    def __contains__(self , key):
        return key in self.summary

    
    def keys(self):
        return self.summary.keys()


    def wells(self):
        return self.summary.wells( )
            
    def loadSummary(self):
        self.summary = EclSum( self.case )


    def startTimeEqual(self , other):
        if self.summary.getDataStartTime() == other.summary.getDataStartTime():
            return True
        else:
            return False


    def cmpSummaryVector(self , other , key , sample = 100):
        if key in self and key in other:
            days_total = min( self.summary.getSimulationLength() , other.summary.getSimulationLength() )
            dt = days_total / (sample - 1)
            days = [ x * dt for x in range(sample) ]
            
            ref_data = self.summary.get_interp_vector( key , days_list = days )
            test_data = other.summary.get_interp_vector( key , days_list = days )
            diff_data = ref_data - test_data

            ref_sum = sum(ref_data)
            diff_sum = sum( abs(diff_data) )
            return (diff_sum , ref_sum)
        else:
            raise KeyError("Key:%s was not present in both cases" % key)


        
        

class EclCmp(object):

    def __init__(self , test_case , ref_case):
        self.test_case = EclCase( test_case )
        self.ref_case = EclCase( ref_case )

        self.initCheck( )


    def initCheck(self):
        if not self.test_case.startTimeEqual( self.ref_case ):
            raise ValueError("The two cases do not start at the same time - can not be compared")

        
    def hasSummaryVector(self , key):
        return (key in self.test_case , key in self.ref_case)

    def cmpSummaryVector(self , key , sample = 100):
        return self.test_case.cmpSummaryVector( self.ref_case , key , sample = sample )


    def testKeys(self):
        return self.test_case.keys()

    def testWells(self):
        return self.test_case.wells()
    
