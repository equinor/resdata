#include <string.h>
#include <ecl_kw.h>
#include <ecl_fstate.h>
#include <ecl_sum.h>


static ecl_fstate_type * ECL_FSTATE     = NULL;
static bool              ENDIAN_CONVERT = true;

static ecl_sum_type    * ECL_SUM        = NULL;

/******************************************************************/


void ecl_inter_load_file__(const char *__filename , const int *strlen) {
  const char null_char = '\0';
  char *filename;
  filename = malloc(*strlen + 1);
  strncpy(filename , __filename , *strlen);
  filename[*strlen] = null_char;
  ECL_FSTATE = ecl_fstate_load_unified(filename , ECL_FMT_AUTO , ENDIAN_CONVERT);
  free(filename);
}


void ecl_inter_free__(void) {
  if (ECL_FSTATE != NULL) {
    ecl_fstate_free(ECL_FSTATE);
    ECL_FSTATE = NULL;
  }
  if (ECL_SUM != NULL) {
    ecl_sum_free(ECL_SUM);
    ECL_SUM = NULL;
  }
}



/*
  Fortran index conventions in the call: These functions are called
  with 1-based indexes.
*/

void ecl_inter_kw_iget__(const char *kw , const int *istep , const int *iw , void *value) {
  if (!ecl_fstate_kw_iget(ECL_FSTATE , (*istep) - 1 , kw  , (*iw) - 1 , value)) {
    fprintf(stderr,"%s - failed to load kw:%s  timestep:%d  index:%d - aborting \n",__func__ , kw , *istep , *iw);
    abort();
  }
}


void ecl_inter_get_kw_size__(const char *kw, const int *istep, int *size) {
  *size = ecl_fstate_kw_get_size(ECL_FSTATE , (*istep) - 1 , kw);
}
  


void ecl_inter_kw_get_data__(const char *kw , const int *istep , void *value) {
  if (!ecl_fstate_kw_get_memcpy_data(ECL_FSTATE , (*istep) - 1, kw , value)) {
    fprintf(stderr,"%s: failed to load kw:%s  timestep:%d - aborting.\n",__func__ , kw , *istep);
    abort();
  }
}


void ecl_inter_kw_exists__(const char *kw , const int *istep , int *int_ex) {
  if (ecl_fstate_kw_exists(ECL_FSTATE ,  (*istep) - 1 , kw))
    *int_ex = 1;
  else
    *int_ex =  0;
}


void ecl_inter_get_blocks__(int *blocks) {
  *blocks =  ecl_fstate_get_blocksize(ECL_FSTATE);
}


void ecl_inter_load_summary__(const char *__header_file , const int *header_len , const char *__data_file , const int *data_len) {
  const char null_char = '\0';
  char *header_file , *data_file;
  header_file = malloc(*header_len + 1);
  data_file   = malloc(*data_len   + 1);

  strncpy(header_file , __header_file , *header_len);
  strncpy(data_file , __data_file , *data_len);
  header_file[*header_len] = null_char;
  data_file[*data_len] = null_char;

  ECL_SUM = ecl_sum_load_unified(header_file , data_file , ECL_FMT_AUTO , ENDIAN_CONVERT);
  free(header_file);  
  free(data_file);
}


/* void ecl_inter_new_file(const char * filename , int fmt_mode) { */
/*   ECL_FSTATE = ecl_fstate_alloc(filename , 10 , fmt_mode , ENDIAN_CONVERT); */
/* } */


/* void ecl_inter_get_kw_data(const char *header , char *buffer) { */
/*   ecl_kw_type * ecl_kw = ecl_fstate_get_kw(ECL_FSTATE , header); */
/*   ecl_kw_get_memcpy_data(ecl_kw , buffer); */
/*   ecl_kw_free(ecl_kw); */
/* } */


/* void ecl_inter_add_kwc(const char *header  , const char *ecl_str_type ,  const char *buffer) { */
/*   ecl_kw_type *ecl_kw = ecl_kw_alloc_complete(true , endian_convert , header , size , ecl_str_type ,  buffer); */
/*   ecl_fstate_add_kw_copy(ECL_FSTATE , ecl_kw); */
/*   ecl_kw_free(ecl_kw); */
/* } */



/* void ecl_inter_write_file() { */
/*   ecl_fstate_fwrite(ECL_FSTATE); */
/* } */

