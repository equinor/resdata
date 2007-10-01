#ifndef __THIST_H__
#define __THIST_H__
#include <stdbool.h>

typedef struct thist_struct thist_type;
typedef enum { thist_forecast = 1 , thist_analyzed = 2 , thist_both = 3} thist_data_type;


thist_type * thist_alloc(int , int , thist_data_type);
void         thist_update_scalar_forecast(thist_type *  , int , int , double );
void         thist_update_vector_forecast(thist_type *  , int , const double *);
void         thist_update_scalar_analyzed(thist_type *  , int , int , double );
void         thist_update_vector_analyzed(thist_type *  , int , const double *);
void         thist_update_scalar 	    (thist_type *  , int , int , double , double );
void         thist_update_vector	    (thist_type *  , int , const double * , const double *);
void         thist_matlab_dump(const thist_type * , const char * , const char *);
void         thist_clear(thist_type *);
void         thist_free(thist_type *);

#endif
