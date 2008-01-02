#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ecl_box.h>


struct ecl_box_struct {
  int __limits[3][2];
  int total_size[3];
  int box_size[3];
  int main_stride[3];
  int box_stride[3];
  int box_offset;
};


/*
  Eclipse indices on the way in - everywhere else the indices have
  zero offset.
*/

void ecl_box_set_size(ecl_box_type * ecl_box , int x1,int x2 , int y1 , int y2 , int z1, int z2) {

  ecl_box->__limits[0][0] = x1 - 1;
  ecl_box->__limits[0][1] = x2 - 1;
  ecl_box->__limits[1][0] = y1 - 1;
  ecl_box->__limits[1][1] = y2 - 1;
  ecl_box->__limits[2][0] = z1 - 1;
  ecl_box->__limits[2][1] = z2 - 1;


  ecl_box->box_size[0] = x2 - x1 + 1;
  ecl_box->box_size[1] = y2 - y1 + 1;
  ecl_box->box_size[2] = z2 - z1 + 1;
  
  ecl_box->box_stride[0] = 1;
  ecl_box->box_stride[1] = ecl_box->box_size[0];
  ecl_box->box_stride[2] = ecl_box->box_size[0] * ecl_box->box_size[1];

  ecl_box->box_offset = (x1 - 1) * ecl_box->main_stride[0] + (y1 - 1) * ecl_box->main_stride[1] + (z1 - 1) * ecl_box->main_stride[2];
}



void ecl_box_set_limits(const ecl_box_type * ecl_box , int *x1, int *x2 , int * y1 , int *y2 , int *z1 , int *z2) {
  *x1 = ecl_box->__limits[0][0];
  *x2 = ecl_box->__limits[0][1];
  *y1 = ecl_box->__limits[1][0];
  *y2 = ecl_box->__limits[1][1];
  *z1 = ecl_box->__limits[2][0];
  *z2 = ecl_box->__limits[2][1];
}


ecl_box_type * ecl_box_alloc(int nx , int ny , int nz , int x1,int x2 , int y1 , int y2 , int z1, int z2) {
  ecl_box_type * ecl_box = malloc(sizeof * ecl_box);
  
  ecl_box->total_size[0] = nx;
  ecl_box->total_size[1] = ny;
  ecl_box->total_size[2] = nz;
  
  ecl_box->main_stride[0]   = 1;
  ecl_box->main_stride[1]   = nx;
  ecl_box->main_stride[2]   = nx*ny;
  
  ecl_box_set_size(ecl_box , x1 , x2 , y1 , y2 , z1 , z2);
  return ecl_box;
}



void ecl_box_free(ecl_box_type * ecl_box) { free(ecl_box); }



void ecl_box_set_values(const ecl_box_type * ecl_box , char * main_field , const char * sub_field , int element_size) {
  int i,j,k;

  for (k=0; k < ecl_box->box_size[2]; k++) 
    for(j=0; j < ecl_box->box_size[1]; j++)
      for (i=0; i < ecl_box->box_size[0]; i++) {
	int main_index = k*ecl_box->main_stride[2]   + j*ecl_box->main_stride[1]   + i*ecl_box->main_stride[0] + ecl_box->box_offset;
	int sub_index  = k*ecl_box->box_stride[2]    + j*ecl_box->box_stride[1]    + i*ecl_box->box_stride[0];
	memcpy(&main_field[main_index * element_size] , &sub_field[sub_index * element_size] , element_size);
      }
}


int ecl_box_get_total_size(const ecl_box_type * ecl_box) { return ecl_box->total_size[0] * ecl_box->total_size[1] * ecl_box->total_size[2]; }

int ecl_box_get_box_size(const ecl_box_type * ecl_box) { return ecl_box->box_size[0] * ecl_box->box_size[1] * ecl_box->box_size[2]; }



