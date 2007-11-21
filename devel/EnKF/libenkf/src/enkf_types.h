#ifndef __ENKF_TYPES_H__
#define __ENKF_TYPES_H__
#include <void_arg.h>
typedef double (transform_ftype)                  (double , const void_arg_type *);


/*
  Observe that seemingly random numbers are used in these enum definitions, 
  that is to be able to catch it if a wrong constant is used.
*/

typedef double enkf_float_type;

/*
  Observe that these are used as bitmask's, i.e. they must be power of 2 series.
*/
typedef enum {parameter = 1 , static_parameter = 2 , ecl_restart = 4 , ecl_summary = 8 , ecl_static = 16 , all_types = 31} enkf_var_type;
/* 
   For instance the pressure is implemented with a field, and behaves as a ecl_restart variable. The
   permeability is also implemented as a field, but this is a parameter.
*/


/*
  These correspond to implementation types.
*/
typedef enum {STATIC = 100 , MULTZ , MULTFLT , EQUIL , FIELD , WELL} enkf_impl_type;


/*
  These types are logical types, describing how the parameter behaves in the EnKF
  loop.
*/


typedef enum {active_off = 200 , active_on , active_at , active_after , active_before} enkf_active_type;


typedef enum {abs_err = 300  , rel_err  , relmin_err } enkf_obs_err_type;

/*
  typedef enum {WELL_OBS = 0 , POINT_OBS = 1} enkf_obs_type;
*/

typedef enum {nolog = 0 , log_input_mask = 1 , log_enkf_mask = 2 , log_output_mask = 4 , log_all = 7} enkf_logmode_enum;


#endif
