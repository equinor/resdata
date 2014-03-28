#  Copyright (C) 2011  Statoil ASA, Norway. 
#   
#  The file 'tvector.py' is part of ERT - Ensemble based Reservoir Tool. 
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
"""
Typed vectors IntVector, DoubleVector and BoolVector.

This module implements a quite simple typed vector which will grow
transparently as needed. The vector is created with a default value,
which will be used for not explicitly set indices.

   vec = IntVector( default_value = 66 )
   vec[0] = 10
   vec[2] = 10    

After the 'vec[2] = 10' statement the vector has grown to contain
three elements. The element vec[1] has not been explicitly assigned by
the user, in that case the implementation has 'filled the hole' with
the default value (i.e. 66 in this case). So the statement

   print vec[1]

will give '66'. The main part of the implementation is in terms of an
"abstract base class" TVector. The TVector class should be not
instantiated directly, instead the child classes IntVector,
DoubleVector or BoolVector should be used. 

The C-level has implementations for several fundamental types like
float and size_t not currently implemented in the Python version.
"""

#THIS CLASS IS HERE FOR LEGACY SUPPORT HAS BEEN REPLACED BY VectorTemplate

from .vector_template import VectorTemplate as TVector
from .double_vector import DoubleVector
from .int_vector import IntVector
from .bool_vector import BoolVector
from .time_vector import TimeVector
