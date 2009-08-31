#include <stdlib.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_grid.h>
#include <ecl_box.h>
#include <ecl_util.h>
#include <ecl_region.h>
#include <util.h>

#define ECL_REGION_TYPE_ID 1106377

struct ecl_region_struct {
  UTIL_TYPE_ID_DECLARATION;
  bool                * active_mask;          /* This marks active|inactive in the region, which is unrelated to active in the grid. */
  int                 * global_index_list;    /* This is a list of the cells in the region - irrespective of whether they are active in the grid or not. */
  int                 * active_index_list;    /* This means cells in the region which are also active in the grid */
  int                   global_size;          /* The size of global_index_list. */
  int                   active_size;          /* The size of active_index_list. */
  /******************************************************************/
  /* Grid properties */
  int                   grid_nx,grid_ny,grid_nz,grid_vol,grid_active;
  const ecl_grid_type * parent_grid;
};



UTIL_IS_INSTANCE_FUNCTION( ecl_region , ECL_REGION_TYPE_ID)
UTIL_SAFE_CAST_FUNCTION( ecl_region , ECL_REGION_TYPE_ID)


ecl_region_type * ecl_region_alloc( const ecl_grid_type * ecl_grid , bool preselect) {
  ecl_region_type * region = util_malloc( sizeof * region , __func__);
  UTIL_TYPE_ID_INIT( region , ECL_REGION_TYPE_ID);
  region->parent_grid = ecl_grid;
  ecl_grid_get_dims( ecl_grid , &region->grid_nx , &region->grid_ny , &region->grid_nz , &region->grid_active);
  region->grid_vol          =  region->grid_nx * region->grid_ny * region->grid_nz;
  region->active_mask       = util_malloc(region->grid_vol * sizeof * region->active_mask , __func__);
  region->active_index_list = NULL;
  region->global_index_list = NULL;

  {
    int i;
    for (i=0; i < region->grid_vol; i++) 
      region->active_mask[i] = preselect;
  }

  return region;
}
  


void ecl_region_free( ecl_region_type * region ) {
  free( region->active_mask );
  util_safe_free( region->active_index_list );
  util_safe_free( region->global_index_list );
  free( region );
}

/*****************************************************************/

static void ecl_region_invalidate_index_list( ecl_region_type * region ) {
  region->active_index_list = util_safe_free( region->active_index_list );
  region->global_index_list = util_safe_free( region->global_index_list );
}


static void ecl_region_assert_global_index_list( ecl_region_type * region ) {
  if (region->global_index_list == NULL) {
    int global_index;
    region->global_size = 0;
    region->global_index_list = util_malloc( region->grid_vol * sizeof * region->global_index_list , __func__);
    for (global_index = 0; global_index < region->grid_vol; global_index++) {
      if (region->active_mask[ global_index ]) {
        region->global_index_list[ region->global_size ] = global_index;
        region->global_size++;
      }
    }
    region->global_index_list = util_realloc( region->global_index_list , region->global_size * sizeof * region->global_index_list , __func__);
  }
}


static void ecl_region_assert_active_index_list( ecl_region_type * region ) {
  if (region->active_index_list == NULL) {
    int global_index;
    region->active_size = 0;
    region->active_index_list = util_malloc( region->grid_active * sizeof * region->global_index_list , __func__);
    for (global_index = 0; global_index < region->grid_vol; global_index++) {
      if (region->active_mask[ global_index ]) {
        int active_index = ecl_grid_get_active_index1( region->parent_grid , global_index );
        if (active_index >= 0) {
          region->active_index_list[ region->active_size ] = active_index;
          region->active_size++;
        }
      }
    }
    region->active_index_list = util_realloc( region->active_index_list , region->active_size * sizeof * region->active_index_list , __func__);
  }
}


/*****************************************************************/

int ecl_region_get_active_size( ecl_region_type * region ) {
  ecl_region_assert_active_index_list( region );
  return region->active_size;
}


int ecl_region_get_global_size( ecl_region_type * region ) {
  ecl_region_assert_global_index_list( region );
  return region->global_size;
}


const int * ecl_region_get_active_list( ecl_region_type * region ) {
  ecl_region_assert_active_index_list( region );
  return region->active_index_list;
}


const int * ecl_region_get_global_list( ecl_region_type * region ) {
  ecl_region_assert_global_index_list( region );
  return region->global_index_list;
}

/*****************************************************************/

static void ecl_region_assert_kw( const ecl_region_type * region , const ecl_kw_type * ecl_kw , bool * global_kw) {
  int kw_size = ecl_kw_get_size( ecl_kw );
  if (!(kw_size == region->grid_vol || kw_size == region->grid_active))
    util_abort("%s: size mismatch between ecl_kw instance and region->grid \n",__func__);
  if (kw_size == region->grid_vol)
    *global_kw = true;
  else
    *global_kw = false;
  
}


/*****************************************************************/ 

static void ecl_region_select_cell__( ecl_region_type * region , int i , int j , int k, bool select) {
  int global_index = ecl_grid_get_global_index3( region->parent_grid , i,j,k);
  region->active_mask[global_index] = select;
  ecl_region_invalidate_index_list( region );
}


void ecl_region_select_cell( ecl_region_type * region , int i , int j , int k) {
  ecl_region_select_cell__( region , i,j,k, true);
}

void ecl_region_deselect_cell( ecl_region_type * region , int i , int j , int k) {
  ecl_region_select_cell__( region , i,j,k, false);
}

/*****************************************************************/


static void ecl_region_select_equal__( ecl_region_type * region , const ecl_kw_type * ecl_kw, int value , bool select) {
  bool global_kw;
  ecl_region_assert_kw( region , ecl_kw , &global_kw);
  if (ecl_kw_get_type( ecl_kw ) != ecl_int_type) 
    util_abort("%s: sorry - select by equality is only supported for integer keywords \n",__func__);
  {
    const int * kw_data = ecl_kw_get_int_ptr( ecl_kw );
    if (global_kw) {
      int global_index;
      for (global_index = 0; global_index < region->grid_vol; global_index++) {
        if (kw_data[ global_index ] == value)
          region->active_mask[ global_index ] = select;
      }
    } else {
      int active_index;
      for (active_index = 0; active_index < region->grid_active; active_index++) {
        if (kw_data[active_index] == value) {
          int global_index = ecl_grid_get_global_index1A( region->parent_grid , active_index );
          region->active_mask[ global_index ] = select;
        }
      }
    }
  }
  ecl_region_invalidate_index_list( region );
}


void ecl_region_select_equal( ecl_region_type * region , const ecl_kw_type * ecl_kw, int value ) {
  ecl_region_select_equal__( region , ecl_kw , value , true );
}


void ecl_region_deselect_equal( ecl_region_type * region , const ecl_kw_type * ecl_kw, int value ) {
  ecl_region_select_equal__( region , ecl_kw , value , false );
}


/*****************************************************************/

static void ecl_region_select_in_interval__( ecl_region_type * region , const ecl_kw_type * ecl_kw, float min_value , float max_value , bool select) {
  bool global_kw;
  ecl_region_assert_kw( region , ecl_kw , &global_kw);
  if (ecl_kw_get_type( ecl_kw ) != ecl_float_type) 
    util_abort("%s: sorry - select by in_interval is only supported for float keywords \n",__func__);
  {
    const float * kw_data = ecl_kw_get_float_ptr( ecl_kw );
    if (global_kw) {
      int global_index;
      for (global_index = 0; global_index < region->grid_vol; global_index++) {
        if (kw_data[ global_index ] >= min_value && kw_data[ global_index ] < max_value)
          region->active_mask[ global_index ] = select;
      }
    } else {
      int active_index;
      for (active_index = 0; active_index < region->grid_active; active_index++) {
        if (kw_data[ active_index ] >= min_value && kw_data[ active_index ] < max_value) {
          int global_index = ecl_grid_get_global_index1A( region->parent_grid , active_index );
          region->active_mask[ global_index ] = select;
        }
      }
    }
  }
  ecl_region_invalidate_index_list( region );
}

 
void ecl_region_select_in_interval( ecl_region_type * region , const ecl_kw_type * ecl_kw, float min_value , float max_value) {
  ecl_region_select_in_interval__( region , ecl_kw , min_value , max_value , true );
}


void ecl_region_deselect_in_interval( ecl_region_type * region , const ecl_kw_type * ecl_kw, float min_value , float max_value) {
  ecl_region_select_in_interval__( region , ecl_kw , min_value , max_value , false );
}


/*****************************************************************/

/**
   Will select all the cells in the box. Remember that the box is
   defined as an inclusive geometry.
*/

static void ecl_region_select_from_box__( ecl_region_type * region , const ecl_box_type * ecl_box , bool select) {
  const int   box_size    = ecl_box_get_global_size( ecl_box );
  const int * active_list = ecl_box_get_global_list( ecl_box );
  
  for (int box_index = 0; box_index < box_size; box_index++) 
    region->active_mask[ active_list[box_index] ] = select;
      
  ecl_region_invalidate_index_list( region );
}


void ecl_region_select_from_box( ecl_region_type * region , const ecl_box_type * ecl_box ) {
  ecl_region_select_from_box__( region , ecl_box , true );
}


void ecl_region_deselect_from_box( ecl_region_type * region , const ecl_box_type * ecl_box ) {
  ecl_region_select_from_box__( region , ecl_box , false );
}



/*****************************************************************/

/**
   Observe that i1 and i2 are:
   
     * ZERO offset.
     * An inclusive interval : [i1,i2]
     
   Input values below zero or above the upper limit are truncated.  
*/

static void ecl_region_select_i1i2__( ecl_region_type * region , int i1 , int i2 , bool select) {
  if (i1 > i2) 
    util_abort("%s: i1 > i2 - this is illogical ... \n",__func__);
  i1 = util_int_max(0 , i1);
  i2 = util_int_min(region->grid_nx - 1 , i2);
  {
    for (int k = 0; k < region->grid_nz; k++)
      for (int j = 0; j < region->grid_ny; j++)
        for (int i = i1; i <= i2; i++) {
          int global_index = ecl_grid_get_global_index3( region->parent_grid , i,j,k);
          region->active_mask[global_index] = select;
        }
  }
  ecl_region_invalidate_index_list( region );
}


void ecl_region_select_i1i2( ecl_region_type * region , int i1 , int i2) {
  ecl_region_select_i1i2__( region , i1 , i2 , true );
}


void ecl_region_deselect_i1i2( ecl_region_type * region , int i1 , int i2) {
  ecl_region_select_i1i2__( region , i1 , i2 , false );
}


/*****************************************************************/

/**
   Observe that j1 and j2 are:
   
     * ZERO offset.
     * An inclusive interval : [i1,i2]
     
   Input values below zero or above the upper limit are truncated.  
*/

static void ecl_region_select_j1j2__( ecl_region_type * region , int j1 , int j2 , bool select) {
  if (j1 > j2) 
    util_abort("%s: i1 > i2 - this is illogical ... \n",__func__);
  j1 = util_int_max(0 , j1);
  j2 = util_int_min(region->grid_nx - 1 , j2);
  {
    for (int k = 0; k < region->grid_nz; k++)
      for (int j = j1; j <= j2; j++)
        for (int i = 0; i < region->grid_nx; i++) {
          int global_index = ecl_grid_get_global_index3( region->parent_grid , i,j,k);
          region->active_mask[global_index] = select;
        }
  }
  ecl_region_invalidate_index_list( region );
}


void ecl_region_select_j1j2( ecl_region_type * region , int j1 , int j2) {
  ecl_region_select_j1j2__( region , j1 , j2 , true );
}


void ecl_region_deselect_j1j2( ecl_region_type * region , int j1 , int j2) {
  ecl_region_select_j1j2__( region , j1 , j2 , false );
}


/*****************************************************************/

/**
   Observe that k1 and k2 are:
   
     * ZERO offset.
     * An inclusive interval : [i1,i2]
     
   Input values below zero or above the upper limit are truncated.  
*/

static void ecl_region_select_k1k2__( ecl_region_type * region , int k1 , int k2 , bool select) {
  if (k1 > k2) 
    util_abort("%s: i1 > i2 - this is illogical ... \n",__func__);
  k1 = util_int_max(0 , k1);
  k2 = util_int_min(region->grid_nx - 1 , k2);
  {
    for (int k = k1; k <= k2; k++)
      for (int j = 0; j < region->grid_ny; j++)
        for (int i = 0; i < region->grid_nx; i++) {
          int global_index = ecl_grid_get_global_index3( region->parent_grid , i,j,k);
          region->active_mask[global_index] = select;
        }
  }
  ecl_region_invalidate_index_list( region );
}


void ecl_region_select_k1k2( ecl_region_type * region , int k1 , int k2) {
  ecl_region_select_k1k2__( region , k1 , k2 , true );
}


void ecl_region_deselect_k1k2( ecl_region_type * region , int k1 , int k2) {
  ecl_region_select_k1k2__( region , k1 , k2 , false );
}


/*****************************************************************/

static void ecl_region_select_all__( ecl_region_type * region , bool select) {
  int global_index;
  for (global_index = 0; global_index < region->grid_vol; global_index++) 
    region->active_mask[ global_index ] = select;
  ecl_region_invalidate_index_list( region );
}


void ecl_region_select_all( ecl_region_type * region) {
  ecl_region_select_all__( region , true );
}

void ecl_region_deselect_all( ecl_region_type * region ) {
  ecl_region_select_all__( region , false );
}
