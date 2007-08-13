#ifndef __ENKF_TYPES_H__
#define __ENKF_TYPES_H__

typedef double enkf_float_type;

/*
  These correspond to implementation types.
*/
typedef enum {STATIC = 0 , MULTZ , MULTFLT , EQUIL , FIELD , WELL} enkf_impl_type;


/*
  These types are logical types, describing how the parameter behaves in the EnKF
  loop.
*/

/*
  Observe that these are used as bitmask's, i.e. they must be power of 2 series.
*/
typedef enum {parameter = 1 , static_parameter = 2 , ecl_restart = 4 , ecl_summary = 8 , ecl_static = 16 , all_types = 31} enkf_var_type;
/* 
   For instance the pressure is implemented with a field, and behaves as a ecl_restart variable. The
   permeability is also implemented as a field, but this is a parameter.
*/



#endif
