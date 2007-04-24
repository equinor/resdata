#include <stdlib.h>
#include <enkf_state.h>
#include <ecl_static_kw.h>
#include <ecl_static_kw_config.h>
#include <ecl_kw.h>


struct ecl_static_kw_struct {
  ecl_static_kw_config_type * config;
  ecl_kw_type               * ecl_kw;
  const enkf_state_type     * enkf_state;
};



ecl_static_kw_type * ecl_static_kw_alloc(const enkf_state_type * enkf_state) {
  ecl_static_kw_type * static_kw = malloc(sizeof *static_kw);
  static_kw->ecl_kw     = NULL;
  static_kw->enkf_state = enkf_state;
  return static_kw;
}

void ecl_static_kw_free(ecl_static_kw_type * kw) {
  if (kw->ecl_kw != NULL) ecl_kw_free(kw->ecl_kw);
  /*ecl_static_kw_config_free(kw->config);*/
  free(kw);
}



void ecl_static_kw_init(ecl_static_kw_type * ecl_static_kw, const ecl_kw_type * ecl_kw) {
  
  ecl_static_kw->ecl_kw = ecl_kw_alloc_copy(ecl_kw);

}

char * ecl_static_kw_alloc_ensname(const ecl_static_kw_type *kw) {
  char *ens_file  = enkf_state_alloc_ensname(kw->enkf_state , kw->config->ens_file);
  return ens_file;
}


void ecl_static_ens_write(const ecl_static_kw_type * ecl_static_kw) {
  char * ens_name = ecl_static_kw_alloc_ensname(ecl_static_kw);
  fortio_type * fortio = fortio_open(ens_name , "w" , ecl_kw_get_endian_convert(ecl_static_kw->ecl_kw));
  
  ecl_kw_fwrite(ecl_static_kw->ecl_kw , fortio);
  fortio_close(fortio);

  free(ens_name);
}

