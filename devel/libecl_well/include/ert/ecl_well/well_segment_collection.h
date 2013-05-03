/*
   Copyright (C) 2013  Statoil ASA, Norway. 
   
                   The file 'well_segment_collection.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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


#ifndef __WELL_SEGMENT_COLLECTION_H__
#define __WELL_SEGMENT_COLLECTION_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <ert/ecl_well/well_segment.h>
  
  typedef struct well_segment_collection_struct well_segment_collection_type;

  well_segment_collection_type * well_segment_collection_alloc();
  void                           well_segment_collection_free(well_segment_collection_type * segment_collection );
  int                            well_segment_collection_get_size( const well_segment_collection_type * segment_collection );
  void                           well_segment_collection_add( well_segment_collection_type * segment_collection , well_segment_type * segment);
  well_segment_type            * well_segment_collection_iget( const well_segment_collection_type * segment_collection , int index);

#ifdef __cplusplus
}
#endif
#endif
