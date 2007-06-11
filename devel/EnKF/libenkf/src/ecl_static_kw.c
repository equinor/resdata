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



const ecl_kw_type * ecl_static_kw_ecl_kw_ptr(const ecl_static_kw_type * ecl_static) { return ecl_static->ecl_kw; }

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
  kw->ecl_kw = NULL;
}


void ecl_static_kw_free(ecl_static_kw_type * kw) {
  ecl_static_kw_free_data(kw);
  free(kw);
}


void ecl_static_kw_init(ecl_static_kw_type * ecl_static_kw, const ecl_kw_type * ecl_kw) {
  ecl_static_kw->ecl_kw = ecl_kw_alloc_copy(ecl_kw);
}


char * ecl_static_kw_alloc_ensfile(const ecl_static_kw_type * ecl_static_kw , const char * path) {
  return util_alloc_full_path(path , ecl_static_kw_config_get_ensname_ref(ecl_static_kw->config));
}


void ecl_static_kw_fread(ecl_static_kw_type * ecl_static_kw , const char * file) {
  if (ecl_static_kw->ecl_kw != NULL) {
    fprintf(stderr,"%s: Internal  programming error - should be called with ecl_kw == NULL - aborting \n",__func__);
    abort();
  }
  {
    const bool endian_convert = true;  /* SHould be stored in config somewhere ... */ 
    const bool fmt_file       = false;

    fortio_type * fortio = fortio_open(file , "r" , endian_convert );
    ecl_static_kw->ecl_kw = ecl_kw_fread_alloc( fortio , fmt_file , endian_convert);
    fortio_close(fortio);
  }
}

void ecl_static_kw_ens_read(ecl_static_kw_type * ecl_static_kw , const char * path) {
  char * ensfile = ecl_static_kw_alloc_ensfile(ecl_static_kw , path);
  ecl_static_kw_fread(ecl_static_kw , ensfile);
  free( ensfile );
}


void ecl_static_kw_fwrite(const ecl_static_kw_type * ecl_static_kw , const char * file) {
  fortio_type * fortio = fortio_open(file , "w" , ecl_kw_get_endian_convert(ecl_static_kw->ecl_kw));
  ecl_kw_fwrite(ecl_static_kw->ecl_kw , fortio);
  fortio_close(fortio);
}

void ecl_static_kw_ens_write(const ecl_static_kw_type * ecl_static_kw , const char * path) {
  char * ensfile = ecl_static_kw_alloc_ensfile(ecl_static_kw , path);
  ecl_static_kw_fwrite(ecl_static_kw , ensfile);
  free( ensfile );
}


char * ecl_static_kw_swapout(ecl_static_kw_type * ecl_static_kw , const char * path) {
  char * ensfile = ecl_static_kw_alloc_ensfile(ecl_static_kw , path);
  ecl_static_kw_fwrite(ecl_static_kw , ensfile);
  ecl_static_kw_free_data(ecl_static_kw);
  return ensfile;
}



void ecl_static_kw_swapin(ecl_static_kw_type * ecl_static_kw , const char * file) {
  ecl_static_kw_fread(ecl_static_kw , file);
}


VOID_SWAPIN(ecl_static_kw);
VOID_SWAPOUT(ecl_static_kw);
VOID_ALLOC_ENSFILE(ecl_static_kw);
/*****************************************************************/
VOID_ALLOC(ecl_static_kw)
VOID_FREE(ecl_static_kw)
VOID_FREE_DATA(ecl_static_kw)
VOID_ENS_WRITE (ecl_static_kw)
VOID_ENS_READ  (ecl_static_kw)
VOID_COPYC(ecl_static_kw)

