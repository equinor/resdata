#ifndef __MEAS_DATA_H__
#define __MEAS_DATA_H__
#include <stdio.h>

typedef struct meas_data_struct meas_data_type;

meas_data_type * meas_data_alloc();
void           meas_data_free(meas_data_type *);
void           meas_data_add(meas_data_type * , double );
void           meas_data_reset(meas_data_type * );
void           meas_data_fprintf(const meas_data_type *  , FILE *);
#endif
