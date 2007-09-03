#include <stdlib.h>
#include <stdio.h>
#include <enkf_util.h>
#include <meas_data.h>

struct meas_data_struct {
  int     size;
  int     alloc_size;
  double *data;
}; 


static void meas_data_realloc_data(meas_data_type * meas_data, int new_alloc_size) {
  meas_data->alloc_size = new_alloc_size;
  meas_data->data       = enkf_util_realloc(meas_data->data , new_alloc_size * sizeof * meas_data->data , __func__);
}


void meas_data_reset(meas_data_type * meas_data) {
  meas_data->size = 0;
}


meas_data_type * meas_data_alloc() {
  meas_data_type * meas_data = malloc(sizeof * meas_data);
  meas_data->size = 0;
  meas_data->data = NULL;
  meas_data->alloc_size = 0;
  meas_data_realloc_data(meas_data , 10);
  return meas_data;
}



void meas_data_add(meas_data_type * meas_data, double value) {
  if (meas_data->size == meas_data->alloc_size)
    meas_data_realloc_data(meas_data , 2*meas_data->alloc_size + 2);
  meas_data->data[meas_data->size] = value;
  meas_data->size++;
}



void meas_data_fprintf(const meas_data_type * meas_data , FILE *stream) {
  int i;
  for (i = 0; i < meas_data->size; i++)
    fprintf(stream , "%-3d : %12.3f\n", i+1 , meas_data->data[i]);
}


void meas_data_free(meas_data_type * meas_data) {
  free(meas_data->data);
  free(meas_data);
}


