#  Copyright (C) 2017  Statoil ASA, Norway.
#
#  This file is part of ERT - Ensemble based Reservoir Tool.
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

from cwrap import BaseCClass
from ecl.util import UtilPrototype


class PermutationVector(BaseCClass):
    TYPE_NAME = "permutation_vector"
    _free = UtilPrototype("void   perm_vector_free( permutation_vector )")
    _size = UtilPrototype("int    perm_vector_get_size( permutation_vector )")
    _iget = UtilPrototype("int    perm_vector_iget( permutation_vector , int)")


    def __init__(self):
        raise NotImplementedError("Can not instantiate PermutationVector directly")


    def __len__(self):
        return self._size( )


    def __str__(self):
        s = "("
        for index in self:
            s += " %d" % index
        return s + ")"

    def __repr__(self):
        return self.create_repr('size=%d' % len(self))


    def __getitem__(self, index):
        if index < 0:
            index += len(self)

        if 0 <= index < len(self):
            return self._iget( index )
        else:
            raise IndexError("Invalid index:%d" % index)


    def free(self):
        self._free( )
