#  Copyright (C) 2014  Statoil ASA, Norway. 
#   
#  The file 'fault_line.py' is part of ERT - Ensemble based Reservoir Tool. 
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


        
import sys
from ert.geo import Polyline




class FaultLine(object):
    def __init__(self , grid , k):
        self.__grid = grid
        self.__k = k
        self.__segment_list = []
        self.__polyline = None

    def __len__(self):
        return len(self.__segment_list)


    def __getitem__(self , index):
        return self.__segment_list[index]

    def __iter__(self):
        return iter(self.__segment_list)


    def verify(self):
        if len(self.__segment_list) > 1:
            current = self.__segment_list[0]
            for next_segment in self.__segment_list[1:]:
                if not current.getC2( ) == next_segment.getC1( ):
                    sys.stdout.write("Current:   %d ---- %d \n" % (current.getC1()      , current.getC2()))
                    sys.stdout.write("Next   :   %d ---- %d \n" % (next_segment.getC1() , next_segment.getC2()))
                    assert current.getC2( ) == next_segment.getC1( )
                current = next_segment
                

    def tryAppend(self , segment):
        if len(self.__segment_list) > 0:
            tail = self.__segment_list[-1]
            if tail.getC2() != segment.getC1():
                if len(self.__segment_list) == 1:
                    if tail.getC2() == segment.getC2():
                        segment.swap()
                    else:
                        tail.swap()
                        if tail.getC2() == segment.getC2():
                            segment.swap()
                else:
                    segment.swap()

            if not tail.getC2( ) == segment.getC1( ):
                return False

        self.__segment_list.append( segment )
        self.__polyline = None
        return True


    def getK(self):
        return self.__k


    def __initPolyline(self):
        pl = Polyline()
        for segment in self:
            (x,y,z) = self.__grid.getLayerXYZ( segment.getC1( ) , self.__k )
            pl.addPoint( x,y,z )

        segment = self[-1]
        (x,y,z) = self.__grid.getLayerXYZ( segment.getC2( ) , self.__k )
        pl.addPoint( x,y,z )
        self.__polyline = pl
        

    def getPolyline(self):
        if self.__polyline is None:
            self.__initPolyline()
        return self.__polyline
            

    def dump(self):
        print "-----------------------------------------------------------------"
        for segment in self:
            C1 = segment.getC1()
            C2 = segment.getC2()
            (J1 , I1) = divmod(C1 , self.__grid.getNX() + 1)
            (J2 , I2) = divmod(C2 , self.__grid.getNX() + 1)
            print "[Corner:%5d   IJ:(%3d,%d)] -> [Corner:%5d   IJ:(%3d,%d)]" % (C1 , I1, J1 ,C2 , I2 , J2)



