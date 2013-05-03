/*
   Copyright (C) 2013  Statoil ASA, Norway. 
   
   The file 'well_segment.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_rsthead.h>

#include <ert/ecl_well/well_const.h>
#include <ert/ecl_well/well_conn.h>
#include <ert/ecl_well/well_segment.h>

#define WELL_SEGMENT_TYPE_ID  2209166 

struct well_segment_struct {
  UTIL_TYPE_ID_DECLARATION;
  int                 link_count;
  int                 segment_id;  
  int                 branch_id; 
  int                 outlet_segment_id;  // This is in the global index space given by the ISEG keyword.
  well_segment_type * outlet_segment;
};


UTIL_IS_INSTANCE_FUNCTION( well_segment , WELL_SEGMENT_TYPE_ID )
static UTIL_SAFE_CAST_FUNCTION( well_segment , WELL_SEGMENT_TYPE_ID )


well_segment_type * well_segment_alloc(int segment_id , int outlet_segment_id , int branch_id ) {
  well_segment_type * segment = util_malloc( sizeof * segment );
  UTIL_TYPE_ID_INIT( segment , WELL_SEGMENT_TYPE_ID );
  
  segment->link_count = 0;
  segment->segment_id = segment_id;
  segment->outlet_segment_id = outlet_segment_id;
  segment->branch_id = branch_id;
  segment->outlet_segment = NULL;

  return segment;
}


well_segment_type * well_segment_alloc_from_kw( const ecl_kw_type * iseg_kw , const ecl_rsthead_type * header , int well_nr, int segment_id) {
  const int iseg_offset = header->nisegz * ( header->nsegmx * well_nr + segment_id);
  int outlet_segment_id = ecl_kw_iget_int( iseg_kw , iseg_offset + ISEG_OUTLET_ITEM );   
  int branch_id         = ecl_kw_iget_int( iseg_kw , iseg_offset + ISEG_BRANCH_ITEM );  
  
  well_segment_type * segment = well_segment_alloc( segment_id , outlet_segment_id , branch_id );
  return segment;
}


/*
    if (iseg_kw != NULL) {
      if (conn->segment != WELL_CONN_NORMAL_WELL_SEGMENT_ID) {
  
      } else {
        conn->branch = 0;
        conn->outlet_segment = 0;
      }
    } else {
      conn->branch = 0;
      conn->outlet_segment = 0;
    }
    */


void well_segment_free(well_segment_type * segment ) {
  free( segment );
}

void well_segment_free__(void * arg) {
  well_segment_type * segment = well_segment_safe_cast( arg );
  well_segment_free( segment );
}


bool well_segment_active( const well_segment_type * segment ) {
  if (segment->branch_id == ECLIPSE_WELL_SEGMENT_BRANCH_INACTIVE_VALUE)
    return false;
  else
    return true;
}


bool well_segment_main_stem( const well_segment_type * segment ) {
  if (segment->branch_id == ECLIPSE_WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE)
    return true;
  else
    return false;
}


bool well_segment_nearest_wellhead( const well_segment_type * segment ) {
  if (segment->outlet_segment_id == ECLIPSE_WELL_SEGMENT_OUTLET_END_VALUE)
    return true;
  else
    return false;
}
  

int well_segment_get_link_count( const well_segment_type * segment ) {
  return segment->link_count;
}

int well_segment_get_branch_id( const well_segment_type * segment ) {
  return segment->branch_id;
}

int well_segment_get_outlet_id( const well_segment_type * segment ) {
  return segment->outlet_segment_id;
}

int well_segment_get_id( const well_segment_type * segment ) {
  return segment->segment_id;
}


well_segment_type * well_segment_get_outlet( const well_segment_type * segment ) {
  return segment->outlet_segment;
}
  

bool well_segment_link( well_segment_type * segment , well_segment_type * outlet_segment ) {
  if (segment->outlet_segment_id == outlet_segment->segment_id) {
    segment->outlet_segment = outlet_segment;
    outlet_segment->link_count++;
    return true;
  } else 
    /* 
       This is a quite fatal topological error - and aborting is probaly the wisest
       thing to do. I.e.  the function well_segment_link_strict() is recommended.
    */
    return false;
}


void well_segment_link_strict( well_segment_type * segment , well_segment_type * outlet_segment ) {
  if (!well_segment_link( segment , outlet_segment))
    util_abort("%s: tried to create invalid link between segments %d and %d \n",segment->segment_id , outlet_segment->segment_id);
}
