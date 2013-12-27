#  Copyright (C) 2013  Statoil ASA, Norway. 
#   
#  The file 'ecl_npv.py' is part of ERT - Ensemble based Reservoir Tool. 
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

import re
from ert.ecl import EclSum


class EclNPV(object):
    sumKeyRE = re.compile("[[]([\w:,]+)[]]")


    def __init__(self , baseCase):
        sum = EclSum( baseCase )
        if sum:
            self.baseCase = sum
        else:
            raise Error("Failed to open ECLIPSE sumamry case:%s" % baseCase)
        self.expression = None
        self.keyList = []

    
    def eval(self):
        if self.expression is None:
            raise ValueError("Can not eval with an expression to evaluate")
        pass


    def getExpression(self):
        return self.expression


    def setExpression(self , expression):
        self.compiled_expr = self.compile( expression )
        self.expression = expression


    def getKeyList(self):
        return self.keyList


    def compile(self , expression):
        if expression.count("[") != expression.count("]"):
            raise ValueError("Expression:%s invalid - not mathcing [ and ]" % expression)

        self.keyList = []
        for key in self.sumKeyRE.findall( expression ):
            if self.baseCase.has_key( key ):
                smspec = self.baseCase.smspec_node( key )
                if not smspec.is_total:
                    raise KeyError("Key:%s is not a total quantity" % key)
            else:
                raise KeyError("Summary case does not have key:%s" % key)
            self.keyList.append( key )
        
