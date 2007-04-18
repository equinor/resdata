#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <util.h>
#include <ecl_kw.h>
#include <enkf_ecl_kw.h>
#include <enkf_ecl_kw_config.h>

struct enkf_ecl_kw_struct {
  ecl_type_enum                  ecl_type;
  const enkf_ecl_kw_config_type *config;
  double                        *data;
};


enkf_ecl_kw_type * enkf_ecl_kw_alloc() { return NULL; }


static void enkf_ecl_kw_get_data(enkf_ecl_kw_type * enkf_kw , const ecl_kw_type *ecl_kw) {
  if (enkf_kw->ecl_type == ecl_double_type) 
    ecl_kw_get_memcpy_data(ecl_kw , enkf_kw->data);
  else if (enkf_kw->ecl_type == ecl_float_type) 
    util_float_to_double(enkf_kw->data , ecl_kw_get_data_ref(ecl_kw) , ecl_kw_get_size(ecl_kw));
  else {
    fprintf(stderr,"%s fatal error ECLIPSE type:%s can not be used in EnKF - aborting \n",__func__  , ecl_kw_get_str_type_ref(ecl_kw));
    abort();
  }
}



MATH_OPS(enkf_ecl_kw);



