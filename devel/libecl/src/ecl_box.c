#include <util.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ecl_box.h>
#include <ecl_grid.h>


#define ECL_BOX_TYPE_ID 6610643

struct ecl_box_struct {
  UTIL_TYPE_ID_DECLARATION;
  int     grid_nx  , grid_ny  , grid_nz;
  int     grid_sx  , grid_sy  , grid_sz;   /* xxx_sx : x stride */

  int     box_nx  , box_ny  , box_nz;
  int     box_sx  , box_sy  , box_sz;   
  int     box_offset;
  int     active_size;
  int    *active_list;  /* This is a list with active_size elements containing the index active index of the elemtents in the box. Will be NULL if there are no active elements. */
  int    *global_list;  /* This is a list of global indices which are present in the box. */
  const ecl_grid_type * parent_grid;
};


static bool verify_pair( int a1, int a2 ) {
  bool OK = true;
  if ( a1 > a2 ) OK = false;
  if ( a1 <  0 ) OK = false;
  return OK;
}


UTIL_IS_INSTANCE_FUNCTION( ecl_box , ECL_BOX_TYPE_ID)
UTIL_SAFE_CAST_FUNCTION( ecl_box , ECL_BOX_TYPE_ID)


/**
   Observe that:

    1. The coordinates i1,i2...k2 are assumed to be ZERO offset.
    2. The corrdinates are  _INCLUSIVE_, i.e. the box is [i1..i2] x [j1..j2] x [k1..k2]

*/

ecl_box_type * ecl_box_alloc(const ecl_grid_type * ecl_grid , int i1,int i2 , int j1 , int j2 , int k1, int k2) {
  ecl_box_type * ecl_box = util_malloc(sizeof * ecl_box , __func__);
  UTIL_TYPE_ID_INIT( ecl_box , ECL_BOX_TYPE_ID);
  ecl_box->parent_grid = ecl_grid;
  /* Properties of the parent grid. */
  ecl_grid_get_dims( ecl_grid , &ecl_box->grid_nx , &ecl_box->grid_ny , &ecl_box->grid_nz , NULL);
  ecl_box->grid_sx   = 1;
  ecl_box->grid_sy   = ecl_box->grid_nx;
  ecl_box->grid_sz   = ecl_box->grid_nx * ecl_box->grid_ny;
  
  {
    bool OK = true;
    if (!verify_pair(i1,i2)) OK = false;
    if (!verify_pair(j1,j2)) OK = false;
    if (!verify_pair(k1,k2)) OK = false;
    if (!OK)
      util_abort("%s: invalid input ... \n",__func__);
  }
  if (i2 >= ecl_box->grid_nx) {
    fprintf(stderr,"** Warning box defined with i2 = %d - truncated to maximum value %d \n",i2 , ecl_box->grid_nx - 1);
    i2 = ecl_box->grid_nx - 1;
  }
  if (j2 >= ecl_box->grid_ny) {
    fprintf(stderr,"** Warning box defined with j2 = %d - truncated to maximum value %d \n",j2 , ecl_box->grid_ny - 1);
    j2 = ecl_box->grid_ny - 1;
  }
  if (k2 >= ecl_box->grid_nz) {
    fprintf(stderr,"** Warning box defined with k2 = %d - truncated to maximum value %d \n",k2 , ecl_box->grid_nz - 1);
    k2 = ecl_box->grid_nz -1;
  }
  
  
  /*Properties of the box: */
  ecl_box->box_nx = i2 - i1 + 1;
  ecl_box->box_ny = j2 - j1 + 1;
  ecl_box->box_nz = k2 - k1 + 1;

  ecl_box->box_sx = 1;
  ecl_box->box_sy = ecl_box->box_nx;
  ecl_box->box_sz = ecl_box->box_nx * ecl_box->box_ny;
  ecl_box->box_offset = i1 * ecl_box->box_sx + j1 * ecl_box->box_sy + k1 * ecl_box->box_sz;
  /* Counting the number of active elements in the box */
  {
    int global_counter = 0;
    int i,j,k;
    ecl_box->active_size = 0;
    ecl_box->active_list = util_malloc( ecl_box->box_nx * ecl_box->box_ny * ecl_box->box_nz * sizeof * ecl_box->active_list , __func__);
    ecl_box->global_list = util_malloc( ecl_box->box_nx * ecl_box->box_ny * ecl_box->box_nz * sizeof * ecl_box->global_list , __func__);
    for (k=k1; k <= k2; k++) 
      for (j=j1; j <= j2; j++)
        for (i=i1; i <= i2; i++) {
          {
            int global_index = ecl_grid_get_global_index3( ecl_box->parent_grid , i , j , k);
            ecl_box->global_list[global_counter] = global_index;
            global_counter++;
          }
          {
            int active_index = ecl_grid_get_active_index3( ecl_box->parent_grid , i,j,k);
            if (active_index >= 0) {
              ecl_box->active_list[ecl_box->active_size] = active_index;
              ecl_box->active_size++;
            }
          }
        }
    
    ecl_box->active_list = util_realloc( ecl_box->active_list , ecl_box->active_size * sizeof * ecl_box->active_list , __func__);
  }
  
  return ecl_box;
}



void ecl_box_free(ecl_box_type * ecl_box) { 
  util_safe_free(ecl_box->active_list );
  util_safe_free(ecl_box->global_list );
  free(ecl_box); 
}



/*
void ecl_kw_merge(ecl_kw_type * main_kw , const ecl_kw_type * sub_kw , const ecl_box_type * ecl_box) {
  if (main_kw->sizeof_ctype != sub_kw->sizeof_ctype) 
    util_abort("%s: trying to combine two different underlying datatypes - aborting \n",__func__);

  if (ecl_kw_get_size(main_kw) != ecl_box_get_total_size(ecl_box)) 
    util_abort("%s box size and total_kw mismatch - aborting \n",__func__);

  if (ecl_kw_get_size(sub_kw)   != ecl_box_get_box_size(ecl_box)) 
    util_abort("%s box size and total_kw mismatch - aborting \n",__func__);

  ecl_box_set_values(ecl_box , ecl_kw_get_data_ref(main_kw) , ecl_kw_get_data_ref(sub_kw) , main_kw->sizeof_ctype);
}
*/

void ecl_box_set_values(const ecl_box_type * ecl_box , char * main_field , const char * sub_field , int element_size) {
  int i,j,k;

  for (k=0; k < ecl_box->box_nz; k++) 
    for(j=0; j < ecl_box->box_ny; j++)
      for (i=0; i < ecl_box->box_nx; i++) {
	int grid_index = k*ecl_box->grid_sz   + j*ecl_box->grid_sy   + i*ecl_box->grid_sx + ecl_box->box_offset;
	int box_index  = k*ecl_box->box_sz    + j*ecl_box->box_sy    + i*ecl_box->box_sx;
	memcpy(&main_field[grid_index * element_size] , &sub_field[box_index * element_size] , element_size);
      }
}


/*
  Return the number of active element in the box. 
*/
int ecl_box_get_active_size( const ecl_box_type * ecl_box ) {
  return ecl_box->active_size;
}


const int * ecl_box_get_active_list( const ecl_box_type * ecl_box ) {
  return ecl_box->active_list;
}


/*
  Return the number of global element in the box. 
*/
int ecl_box_get_global_size( const ecl_box_type * ecl_box ) {
  return ecl_box->box_nx * ecl_box->box_ny * ecl_box->box_nz;
}


const int * ecl_box_get_global_list( const ecl_box_type * ecl_box ) {
  return ecl_box->global_list;
}
