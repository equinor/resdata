#include <string.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <ecl_sum.h>
#include <hash.h>


struct ecl_sum_struct {
  ecl_fstate_type * header;
  ecl_fstate_type * data;
  hash_type       * index_hash;
  int               fmt_mode;
  int               Nwells;
  char            **well_list;
  bool              endian_convert;
};




static void set_well_kw_string(const char *well , const char *kw , char *well_kw) {
  const char space_char = ' ';
  const char null_char  = '\0';
  int i, offset;
  i = 0;
  while (well[i] != space_char && well[i] != null_char) {
    well_kw[i] = well[i];
    i++;
  }

  offset = i;
  i = 0;
  while (kw[i] != space_char && kw[i] != null_char) {
    well_kw[offset + i] = kw[i];
    i++;
  }
  well_kw[offset + i] = null_char;
}  


static ecl_sum_type * ecl_sum_alloc1(const char *header_file , int fmt_mode , bool endian_convert) {
  ecl_sum_type *ecl_sum;
  ecl_sum = malloc(sizeof *ecl_sum);
  ecl_sum->fmt_mode       = fmt_mode;
  ecl_sum->endian_convert = endian_convert;
  ecl_sum->header         = ecl_fstate_load_unified(header_file , ecl_sum->fmt_mode , ecl_sum->endian_convert);
  ecl_sum->index_hash     = hash_alloc(ecl_fstate_get_blocksize(ecl_sum->header));
  {
    ecl_kw_type *wells     = ecl_fstate_get_kw(ecl_sum->header , 0 , "WGNAMES"); 
    ecl_kw_type *keywords  = ecl_fstate_get_kw(ecl_sum->header , 0 , "KEYWORDS"); 
    hash_type   *well_hash = hash_alloc(10);
    int index;
    for (index=0; index < ecl_kw_get_size(wells); index++) {
      char well_kw[17];
      set_well_kw_string(ecl_kw_iget_ptr(wells , index) , ecl_kw_iget_ptr(keywords , index) , well_kw);
      hash_insert_int(ecl_sum->index_hash , well_kw , index);
      hash_insert_int(well_hash , ecl_kw_iget_ptr(wells , index) , 1);
    }
    ecl_sum->Nwells = hash_get_size(well_hash);
    ecl_sum->well_list = hash_alloc_keylist(well_hash);
    hash_free(well_hash);
  }
  
  return ecl_sum;
}
										


ecl_sum_type * ecl_sum_load_unified(const char * header_file , const char * data_file , int fmt_mode , bool endian_convert) {
  ecl_sum_type * ecl_sum = ecl_sum_alloc1(header_file , fmt_mode , endian_convert);
  ecl_sum->data   = ecl_fstate_load_unified(data_file  , ecl_sum->fmt_mode , ecl_sum->endian_convert);
  return ecl_sum;
}



ecl_sum_type * ecl_sum_load_multiple(const char * header_file , int files , const char ** data_files , int fmt_mode , bool endian_convert) {
  ecl_sum_type * ecl_sum = ecl_sum_alloc1(header_file , fmt_mode , endian_convert);
  ecl_sum->data   = ecl_fstate_load_multiple(files , data_files  , ecl_sum->fmt_mode , ecl_sum->endian_convert);
  return ecl_sum;
}


void ecl_sum_iget2(const ecl_sum_type *ecl_sum , int istep , int index, void *value) {
  ecl_kw_type * data_kw = ecl_fstate_get_kw(ecl_sum->data , istep , "PARAMS");
  ecl_kw_iget(data_kw , index , value);
}


int ecl_sum_iget1(const ecl_sum_type *ecl_sum , int istep , const char *well_name , const char *var_name ,  void *value) {
  char well_kw[17];
  int index;
  
  set_well_kw_string(well_name , var_name , well_kw);
  if (!hash_has_key(ecl_sum->index_hash , well_kw)) {
    fprintf(stderr,"%s: could not find data for well/variable %s/%s \n",__func__ , well_name , var_name);
    abort();
  }
  index = hash_get_int(ecl_sum->index_hash , well_kw);
  ecl_sum_iget2(ecl_sum , istep , index , value);
  return index;
}


int ecl_sum_get_size(const ecl_sum_type *ecl_sum) {
  return ecl_fstate_get_blocksize(ecl_sum->data);
}

void ecl_sum_free(ecl_sum_type *ecl_sum) {
  int i;
  ecl_fstate_free(ecl_sum->header);
  ecl_fstate_free(ecl_sum->data);
  hash_free(ecl_sum->index_hash);
  for (i=0; i < ecl_sum->Nwells; i++)
    free(ecl_sum->well_list[i]);
  free(ecl_sum->well_list);
  free(ecl_sum);
}

