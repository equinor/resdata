#ifndef __ECL_WELL_VARS_H__
#define __ECL_WELL_VARS_H__
#include <stdbool.h>

typedef enum { well_var_orat = 0 ,  well_var_grat = 1 , well_var_wrat = 2 , well_var_bhp = 3, well_var_thp = 4 , well_var_wct = 5 , well_var_gor = 6} well_var_type;


bool           ecl_well_var_valid   (const char * );
well_var_type  ecl_well_var_get_type(const char * );


#endif
