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

struct well_segment_struct {
  
  int                 link_count;
  int                 segment_id;  
  int                 branch_id; 
  int                 outlet_segment_id;  // This is in the global index space given by the ISEG keyword.
  well_segment_type * outlet_segment;
};



well_segment_type * well_segment_alloc(int segment_id , int outlet_segment_id , int branch_id ) {
  well_segment_type * segment = util_malloc( sizeof * segment );

  segment->link_count = 0;
  segment->segment_id = segment_id;
  segment->outlet_segment_id = outlet_segment_id;
  segment->branch_id = branch_id;
  segment->outlet_segment = NULL;

  return segment;
}


void well_segment_free(well_segment_type * segment ) {
  free( segment );
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
