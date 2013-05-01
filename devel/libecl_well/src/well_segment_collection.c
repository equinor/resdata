/*
   Copyright (C) 2013  Statoil ASA, Norway. 
   
   The file 'well_segment_collection.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/vector.h>

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_rsthead.h>

#include <ert/ecl_well/well_segment.h>
#include <ert/ecl_well/well_segment_collection.h>

struct well_segment_collection_struct {
  vector_type * segment_list;
};



well_segment_collection_type * well_segment_collection_alloc() {
  well_segment_collection_type * segment_collection = util_malloc( sizeof * segment_collection );

  segment_collection->segment_list = vector_alloc_new();

  return segment_collection;
}



void well_segment_collection_free(well_segment_collection_type * segment_collection ) {
  vector_free( segment_collection->segment_list );
  free( segment_collection );
}



int well_segment_collection_get_size( const well_segment_collection_type * segment_collection ) {
  return vector_get_size( segment_collection->segment_list );
}
