#ifndef __ECL_REGION_H__
#define __ECL_REGION_H__

#include <ecl_grid.h>
#include <stdbool.h>

typedef struct ecl_region_struct ecl_region_type; 

ecl_region_type * ecl_region_alloc( const ecl_grid_type * ecl_grid , bool preselect);
void              ecl_region_free( ecl_region_type * region );

void          ecl_region_select_in_interval( ecl_region_type * region , const ecl_kw_type * ecl_kw, float min_value , float max_value);
void          ecl_region_deselect_in_interval( ecl_region_type * region , const ecl_kw_type * ecl_kw, float min_value , float max_value);
void          ecl_region_select_equal( ecl_region_type * region , const ecl_kw_type * ecl_kw, int value);
void          ecl_region_deselect_equal( ecl_region_type * region , const ecl_kw_type * ecl_kw, int value);



UTIL_IS_INSTANCE_HEADER( ecl_region );
UTIL_SAFE_CAST_HEADER( ecl_region );


#endif
