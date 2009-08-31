#ifndef __ECL_BOX_H__
#define __ECL_BOX_H__
#include <ecl_grid.h>


typedef struct ecl_box_struct ecl_box_type;


void           ecl_box_set_size       (ecl_box_type * , int , int , int , int , int , int );
ecl_box_type * ecl_box_alloc(const ecl_grid_type * ecl_grid , int i1,int i2 , int j1 , int j2 , int k1, int k2);
void 	       ecl_box_free            (ecl_box_type * );
void 	       ecl_box_set_values(const ecl_box_type * , char * , const char * , int );
int  	       ecl_box_get_total_size(const ecl_box_type * );
int            ecl_box_get_active_size( const ecl_box_type * ecl_box );
const int    * ecl_box_get_active_list( const ecl_box_type * ecl_box );


UTIL_IS_INSTANCE_HEADER( ecl_box );
UTIL_SAFE_CAST_HEADER( ecl_box );


#endif
