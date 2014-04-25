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


        






class FaultLine(object):
    def __init__(self , grid , k):
        self.__grid = grid
        self.__k = k
        self.__segment_list = []


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
                assert current.getC2( ) == next_segment.getC1( )
                current = next_segment
                

    def append(self , segment):
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
            assert tail.getC2() == segment.getC1()
        self.__segment_list.append( segment )


    def getK(self):
        return self.__k


    
