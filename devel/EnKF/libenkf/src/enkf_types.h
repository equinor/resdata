#ifndef __ENKF_TYPES_H__
#define __ENKF_TYPES_H__

typedef double enkf_float_type;
typedef enum {MULTZ , MULTFLT , EQUIL} enkf_type;
typedef enum {parameter = 1 , ecl_restart = 2 , ecl_summary = 4 , ecl_static = 8 , all_types = 15} enkf_var_type;



#endif
