#  Copyright (C) 2011  Statoil ASA, Norway. 
#   
#  The file 'ctime.py' is part of ERT - Ensemble based Reservoir Tool. 
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


import math
import ctypes
import types
import datetime
import time
from ert.cwrap import CWrapper


class ctime(object):
    def __init__(self, value):
        if isinstance(value, types.IntType):
            self.value = value
        elif isinstance(value , ctime):
            self.value = value.value
        else:
            try:
                # Input value is assumed to be datetime.datetime instance
                self.value = int(math.floor(time.mktime(
                    (value.year, value.month, value.day, value.hour, value.minute, value.second, 0, 0, -1 ))))
            except (OverflowError, ValueError, AttributeError):
                # Input value is assumed to be datetime.date instance
                self.value = int(math.floor(time.mktime((value.year, value.month, value.day, 0, 0, 0, 0, 0, -1 ))))


    @classmethod
    def from_param(cls,obj):
        return ctypes.c_long( obj.value )

    def ctime(self):
        """ @rtype: int """
        return self.value

    def time(self):
        """Return this time_t as a time.localtime() object"""
        return time.localtime(self.value)

    def date(self):
        """Return this time_t as a datetime.date([year, month, day])"""
        return datetime.date(*self.time()[0:3])

    def datetime(self):
        return datetime.datetime(*self.time()[0:6])

    def __str__(self):
        return "%s" % (str(self.datetime()))

    def __ge__(self, other):
        if isinstance(other , ctime):
            return self.value >= other.value
        else:
            return self >= ctime(other)

    def __lt__(self, other):
        if isinstance(other , ctime):
            return self.value < other.value
        else:
            return self < ctime(other)

    def __eq__(self, other):
        if isinstance(other , ctime):
            return self.value == other.value
        else:
            return self == ctime(other)

            
    def __imul__(self , other):
        value = int( self.value * other )
        self.value = value
        return self

    def __hash__(self):
        return hash(self.value)

    def __iadd__(self , other):
        if isinstance(other , ctime):
            self.value += other.value
            return self
        else:
            self += ctime(other)

    def __add__(self,other):
        copy = ctime( self )
        copy += other
        return copy

    def __radd__(self,other):
        return self + other


    def __mul__(self , other):
        copy = ctime( self )
        copy *= other
        return copy

    def __rmul__(self , other):
        return self * other

        
    @property
    def stripped(self):
        return time.strptime(self, "%Y-%m-%d %H:%M:S%")


cwrapper = CWrapper(None)
cwrapper.registerType("time_t"  , ctime)
cwrapper.registerType("time_t*" , ctypes.POINTER(ctypes.c_long))

