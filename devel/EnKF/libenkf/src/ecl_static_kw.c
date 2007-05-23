#include <stdlib.h>
#include <enkf_state.h>
#include <util.h>
#include <ecl_static_kw.h>
#include <ecl_static_kw_config.h>
#include <ecl_kw.h>


struct ecl_static_kw_struct {
  const ecl_static_kw_config_type * config;
  ecl_kw_type               * ecl_kw;
};



ecl_static_kw_type * ecl_static_kw_alloc(const ecl_static_kw_config_type * config) {
  ecl_static_kw_type * static_kw = malloc(sizeof *static_kw);
  static_kw->ecl_kw = NULL;
  static_kw->config = config;
  return static_kw;
}


ecl_static_kw_type * ecl_static_kw_copyc(const ecl_static_kw_type *src) {
  ecl_static_kw_type * new = ecl_static_kw_alloc(src->config);
  if (src->ecl_kw != NULL)
    new->ecl_kw = ecl_kw_alloc_copy(src->ecl_kw);
  return new;
}


void ecl_static_kw_free_data(ecl_static_kw_type * kw) {
  if (kw->ecl_kw != NULL) ecl_kw_free(kw->ecl_kw);
}


void ecl_static_kw_free(ecl_static_kw_type * kw) {
  ecl_static_kw_free_data(kw);
  free(kw);
}


void ecl_static_kw_init(ecl_static_kw_type * ecl_static_kw, const ecl_kw_type * ecl_kw) {
  ecl_static_kw->ecl_kw = ecl_kw_alloc_copy(ecl_kw);
}

void ecl_static_kw_ens_read(ecl_static_kw_type * ecl_static_kw , const char * path) {
  
}

void ecl_static_kw_ens_write(const ecl_static_kw_type * ecl_static_kw , const char * path) {
  char * ens_name = util_alloc_full_path(path , ecl_static_kw_config_get_ensname_ref(ecl_static_kw->config));
  fortio_type * fortio = fortio_open(ens_name , "w" , ecl_kw_get_endian_convert(ecl_static_kw->ecl_kw));
  
  ecl_kw_fwrite(ecl_static_kw->ecl_kw , fortio);
  fortio_close(fortio);

  free(ens_name);
}



/*****************************************************************/
VOID_ALLOC(ecl_static_kw)
VOID_FREE(ecl_static_kw)
VOID_FREE_DATA(ecl_static_kw)
VOID_ENS_WRITE (ecl_static_kw)
VOID_ENS_READ  (ecl_static_kw)
VOID_COPYC(ecl_static_kw)

