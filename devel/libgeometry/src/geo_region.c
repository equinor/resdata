/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'geo_region.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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


#include <stdlib.h>
#include <stdbool.h>
#include <util.h>
#include <int_vector.h>
#include <bool_vector.h>
#include <geo_util.h>
#include <geo_pointset.h>
#include <geo_region.h>



struct geo_region_struct {
  bool                      preselect;
  bool                      index_valid;
  bool                    * active_mask; 
  int_vector_type         * index_list;
  const geo_pointset_type * pointset;
  int                       pointset_size;   
};



geo_region_type * geo_region_alloc( const geo_pointset_type * pointset , bool preselect) {
  geo_region_type * region = util_malloc( sizeof * region , __func__);

  region->pointset = pointset;
  region->pointset_size = geo_pointset_get_size( pointset );
  region->preselect = preselect;
  region->index_list = int_vector_alloc( 0, 0);
  region->active_mask = util_malloc( region->pointset_size * sizeof * region->active_mask , __func__ );
  geo_region_reset( region );

  return region;
}


static void geo_region_invalidate_index_list( geo_region_type * region ) {
  region->index_valid = false;
}

static void geo_region_assert_index_list( geo_region_type * region ) {
  if (!region->index_valid) {
    int_vector_reset( region->index_list );
    for (int i=0; i < region->pointset_size; i++)
      if (region->active_mask[i])
        int_vector_append( region->index_list , i );

    region->index_valid = true;
  }
}


void geo_region_reset( geo_region_type * region ) {
  for (int i=0; i < region->pointset_size; i++)
    region->active_mask[i] = region->preselect;
  geo_region_invalidate_index_list( region );
}


void geo_region_free( geo_region_type * region ) {
  int_vector_free( region->index_list );
  free( region );
}


/*****************************************************************/

static void geo_region_polygon_select__( geo_region_type * region , 
                                         const double * xlist , const double * ylist , 
                                         int num_points,
                                         bool select_inside , bool select) {
  
  int index;
  for (index = 0; index < region->pointset_size; index++) {

    double x , y;
    bool is_inside;
    geo_pointset_iget_xy( region->pointset , index , &x , &y);

    is_inside = geo_util_inside_polygon( xlist , ylist , num_points , x , y );
    if (is_inside == select_inside) 
      region->active_mask[index] = select;

  }
  geo_region_invalidate_index_list( region );
}


void geo_region_select_inside_polygon( geo_region_type * region , const double * xlist , const double * ylist , int num_points) {
  geo_region_polygon_select__( region , xlist , ylist , num_points , true , true );
}

void geo_region_select_outside_polygon( geo_region_type * region , const double * xlist , const double * ylist , int num_points) {
  geo_region_polygon_select__( region , xlist , ylist , num_points , false , true );
}

void geo_region_deselect_inside_polygon( geo_region_type * region , const double * xlist , const double * ylist , int num_points) {
  geo_region_polygon_select__( region , xlist , ylist , num_points , true , false );
}

void geo_region_deselect_outside_polygon( geo_region_type * region , const double * xlist , const double * ylist , int num_points) {
  geo_region_polygon_select__( region , xlist , ylist , num_points , false , false );
}

/*****************************************************************/




const int_vector_type * geo_region_get_index_list( geo_region_type * region ) {
  geo_region_assert_index_list( region );
  return region->index_list;
}
