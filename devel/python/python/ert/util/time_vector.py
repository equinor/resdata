#  Copyright (C) 2014  Statoil ASA, Norway.
#   
#  The file 'vector_template.py' is part of ERT - Ensemble based Reservoir Tool.
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

from ert.cwrap import CWrapper
from ert.util import UTIL_LIB, VectorTemplate, ctime


class TimeVector(VectorTemplate):
    default_format = "%d"

    def __init__(self, default_value=0, initial_size=0):
        super(TimeVector, self).__init__(default_value, initial_size)


#################################################################

cwrapper = CWrapper(UTIL_LIB)

CWrapper.registerObject("time_t_vector", TimeVector)


TimeVector.cNamespace().alloc               = cwrapper.prototype("c_void_p time_t_vector_alloc(int, time_t )")
TimeVector.cNamespace().alloc_copy          = cwrapper.prototype("time_t_vector_obj time_t_vector_alloc_copy(time_t_vector )")
TimeVector.cNamespace().strided_copy        = cwrapper.prototype("time_t_vector_obj time_t_vector_alloc_strided_copy(time_t_vector , time_t , time_t , time_t)")
TimeVector.cNamespace().free                = cwrapper.prototype("void   time_t_vector_free( time_t_vector )")
TimeVector.cNamespace().iget                = cwrapper.prototype("time_t   time_t_vector_iget( time_t_vector , int )")
TimeVector.cNamespace().safe_iget           = cwrapper.prototype("time_t   time_t_vector_safe_iget( time_t_vector , int )")
TimeVector.cNamespace().iset                = cwrapper.prototype("time_t   time_t_vector_iset( time_t_vector , int , time_t)")
TimeVector.cNamespace().size                = cwrapper.prototype("int      time_t_vector_size( time_t_vector )")
TimeVector.cNamespace().append              = cwrapper.prototype("void     time_t_vector_append( time_t_vector , time_t )")
TimeVector.cNamespace().idel_block          = cwrapper.prototype("void     time_t_vector_idel_block( time_t_vector , int , int )")
TimeVector.cNamespace().fprintf             = cwrapper.prototype("void     time_t_vector_fprintf( time_t_vector , FILE , char* , char*)")
TimeVector.cNamespace().sort                = cwrapper.prototype("void     time_t_vector_sort( time_t_vector )")
TimeVector.cNamespace().rsort               = cwrapper.prototype("void     time_t_vector_rsort( time_t_vector )")
TimeVector.cNamespace().reset               = cwrapper.prototype("void     time_t_vector_reset( time_t_vector )")
TimeVector.cNamespace().set_read_only       = cwrapper.prototype("void     time_t_vector_set_read_only( time_t_vector , bool )")
TimeVector.cNamespace().get_read_only       = cwrapper.prototype("bool     time_t_vector_get_read_only( time_t_vector )")
TimeVector.cNamespace().get_max             = cwrapper.prototype("time_t   time_t_vector_get_max( time_t_vector )")
TimeVector.cNamespace().get_min             = cwrapper.prototype("time_t   time_t_vector_get_min( time_t_vector )")
TimeVector.cNamespace().get_max_index       = cwrapper.prototype("int      time_t_vector_get_max_index( time_t_vector , bool)")
TimeVector.cNamespace().get_min_index       = cwrapper.prototype("int      time_t_vector_get_min_index( time_t_vector , bool)")
TimeVector.cNamespace().shift               = cwrapper.prototype("void     time_t_vector_shift( time_t_vector , time_t )")
TimeVector.cNamespace().scale               = cwrapper.prototype("void     time_t_vector_scale( time_t_vector , time_t )")
TimeVector.cNamespace().div                 = cwrapper.prototype("void     time_t_vector_div( time_t_vector , time_t )")
TimeVector.cNamespace().inplace_add         = cwrapper.prototype("void     time_t_vector_inplace_add( time_t_vector , time_t_vector )")
TimeVector.cNamespace().inplace_mul         = cwrapper.prototype("void     time_t_vector_inplace_mul( time_t_vector , time_t_vector )")
TimeVector.cNamespace().assign              = cwrapper.prototype("void     time_t_vector_set_all( time_t_vector , time_t)")
TimeVector.cNamespace().memcpy              = cwrapper.prototype("void     time_t_vector_memcpy(time_t_vector , time_t_vector )")
TimeVector.cNamespace().set_default         = cwrapper.prototype("void     time_t_vector_set_default( time_t_vector , time_t)")
TimeVector.cNamespace().get_default         = cwrapper.prototype("time_t   time_t_vector_get_default( time_t_vector )")
TimeVector.cNamespace().alloc_data_copy     = cwrapper.prototype("time_t*  time_t_vector_alloc_data_copy( time_t_vector )")
TimeVector.cNamespace().data_ptr            = cwrapper.prototype("time_t*  time_t_vector_get_ptr( time_t_vector )")
TimeVector.cNamespace().element_size        = cwrapper.prototype("int      time_t_vector_element_size( time_t_vector )")

TimeVector.cNamespace().permute          = cwrapper.prototype("void time_t_vector_permute(time_t_vector, permutation_vector)")
TimeVector.cNamespace().sort_perm        = cwrapper.prototype("permutation_vector_obj time_t_vector_alloc_sort_perm(time_t_vector)")
TimeVector.cNamespace().rsort_perm       = cwrapper.prototype("permutation_vector_obj time_t_vector_alloc_rsort_perm(time_t_vector)")
