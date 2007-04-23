#include <stdlib.h>
#include <enkf_state.h>
#include <ecl_static_kw.h>

struct ecl_static_kw_struct {
  const enkf_state_type * enkf_state;
};


ecl_static_kw_type * ecl_static_kw_alloc(const enkf_state_type * enkf_state) {
  ecl_static_kw_type * static_kw = malloc(sizeof *static_kw);

  static_kw->enkf_state = enkf_state;
  return static_kw;
}



