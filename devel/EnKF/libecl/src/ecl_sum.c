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
  char            * base_name;
  bool              endian_convert;
  bool              unified;
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



static ecl_sum_type * ecl_sum_alloc_empty(int fmt_mode , bool endian_convert , bool unified) {
  ecl_sum_type *ecl_sum;
  ecl_sum = malloc(sizeof *ecl_sum);
  ecl_sum->fmt_mode       = fmt_mode;
  ecl_sum->endian_convert = endian_convert;
  ecl_sum->unified        = unified;
  ecl_sum->index_hash     = hash_alloc(10);
  ecl_sum->header         = NULL;
  ecl_sum->data           = NULL;
  ecl_sum->well_list      = NULL;
  ecl_sum->base_name      = NULL;
  return ecl_sum;
}



static ecl_sum_type * ecl_sum_alloc_existing(const char *header_file , int fmt_mode , bool endian_convert , bool unified) {
  ecl_sum_type *ecl_sum   = ecl_sum_alloc_empty(fmt_mode , endian_convert , unified);
  ecl_sum->header         = ecl_fstate_load_unified(header_file , ecl_sum->fmt_mode , ecl_sum->endian_convert);
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
    ecl_sum->Nwells    = hash_get_size(well_hash);
    ecl_sum->well_list = hash_alloc_keylist(well_hash);
    hash_free(well_hash);
  }
  
  return ecl_sum;
}
	
									
static void ecl_sum_init_new(ecl_sum_type * ecl_sum , const int *_dimens , const char *kw_list , const char * _units , const char * well_list , 
			     const int * _nums , bool iter_header , int Nwells, int Nvars , int kw_offset) {
  const bool FMT_FILE = true;
  int size = 10; /*...*/

  ecl_block_type *header_block = ecl_block_alloc(0 , 10 , FMT_FILE , ecl_sum->endian_convert , NULL);
  ecl_kw_type *kw       = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  ecl_kw_type *units    = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  ecl_kw_type *restart  = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  ecl_kw_type *dimens   = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  ecl_kw_type *wells    = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  ecl_kw_type *nums     = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  ecl_kw_type *startdat = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  /*
    Might not need these ???
    ecl_kw_type *runtimeI = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
    ecl_kw_type *runtimeD = ecl_kw_alloc_empty(FMT_FILE , ecl_sum->endian_convert);
  */
  ecl_kw_set_header(kw       , "KEYWORDS" , size , "CHAR");
  ecl_kw_set_header(units    , "UNITS"    , size , "CHAR");
  ecl_kw_set_header(restart  , "RESTART"  , 9    , "CHAR");
  ecl_kw_set_header(dimens   , "DIMENS"   , 6    , "INTE");
  ecl_kw_set_header(wells    , "WGNAMES"  , size , "CHAR");
  ecl_kw_set_header(nums     , "NUMS"     , size , "INTE");
  ecl_kw_set_header(startdat , "STARTDAT" , 3    , "INTE");

  ecl_block_add_kw(header_block , restart);
  ecl_block_add_kw(header_block , dimens);
  ecl_block_add_kw(header_block , kw);
  ecl_block_add_kw(header_block , wells);
  ecl_block_add_kw(header_block , nums);
  ecl_block_add_kw(header_block , units);
  ecl_block_add_kw(header_block , startdat);
}

static void ecl_sum_set_unified(ecl_sum_type *ecl_sum , bool unified) {
  ecl_sum->unified = unified;
  ecl_fstate_set_unified(ecl_sum->data , unified);
}


ecl_sum_type * ecl_sum_alloc_new(const char *base_name , int fmt_mode , bool endian_convert , bool unified) {
  ecl_sum_type *ecl_sum = ecl_sum_alloc_empty(fmt_mode , endian_convert , unified);
  ecl_sum->header = ecl_fstate_alloc_empty(fmt_mode , endian_convert , true);
  ecl_sum->data   = ecl_fstate_alloc_empty(fmt_mode , endian_convert , unified);
  ecl_sum->base_name = calloc(strlen(base_name) + 1 , sizeof *ecl_sum->base_name);
  strcpy(ecl_sum->base_name , base_name);
  return ecl_sum;
}


void ecl_sum_init_save(ecl_sum_type * ecl_sum , const char * base_name , int fmt_mode , bool unified) {
  ecl_sum->base_name = calloc(strlen(base_name) + 1 , sizeof *ecl_sum->base_name);
  strcpy(ecl_sum->base_name , base_name);

  ecl_sum_set_fmt_mode(ecl_sum , fmt_mode);
  ecl_sum_set_unified(ecl_sum , unified);
}


void ecl_sum_save(const ecl_sum_type * ecl_sum) {
  char *summary_spec , ext[2] , *data_file;
  if (ecl_sum->base_name == NULL || !(ecl_sum->fmt_mode == ECL_FORMATTED || ecl_sum->fmt_mode == ECL_BINARY)) {
    fprintf(stderr,"%s: must inititialise ecl_sum object prior to saving - aborting \n",__func__);
    abort();
  }
  
  if (ecl_sum->fmt_mode == ECL_FORMATTED) {
    summary_spec = malloc( strlen(ecl_sum->base_name) + 9 );
    sprintf(summary_spec , "%s.FSMSPEC" , ecl_sum->base_name);
  } else {
    summary_spec = malloc( strlen(ecl_sum->base_name) + 8);
    sprintf(summary_spec , "%s.SMSPEC" , ecl_sum->base_name);
    sprintf(ext , "S");
  }
  ecl_fstate_set_unified_file(ecl_sum->header , summary_spec);

  if (ecl_sum->unified) {
    if (ecl_sum->fmt_mode == ECL_FORMATTED) {
      data_file = calloc(strlen(ecl_sum->base_name) + 9 , sizeof(char));
      sprintf(data_file , "%s.FUNSMRY" , ecl_sum->base_name);
    } else {
      data_file = calloc(strlen(ecl_sum->base_name) + 8 , sizeof(char));
      sprintf(data_file , "%s.UNSMRY" , ecl_sum->base_name);
    }
    ecl_fstate_set_unified_file(ecl_sum->data , data_file);
    free(data_file);
  } else {
    if (ecl_sum->fmt_mode == ECL_FORMATTED) 
      sprintf(ext , "A");
    else
      sprintf(ext , "S");
    ecl_fstate_set_multiple_files(ecl_sum->data , ecl_sum->base_name , ext);
  }
  
  ecl_fstate_save(ecl_sum->header);
  ecl_fstate_save(ecl_sum->data);
  free(summary_spec);
}



ecl_sum_type * ecl_sum_load_unified(const char * header_file , const char * data_file , int fmt_mode , bool endian_convert) {
  ecl_sum_type * ecl_sum = ecl_sum_alloc_existing(header_file , fmt_mode , endian_convert , true);
  ecl_sum->data   = ecl_fstate_load_unified(data_file  , ecl_sum->fmt_mode , ecl_sum->endian_convert);
  return ecl_sum;
}



ecl_sum_type * ecl_sum_load_multiple(const char * header_file , int files , const char ** data_files , int fmt_mode , bool endian_convert) {
  ecl_sum_type * ecl_sum = ecl_sum_alloc_existing(header_file , fmt_mode , endian_convert , false);
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

void ecl_sum_set_fmt_mode(ecl_sum_type *ecl_sum , int fmt_mode) {
  if (ecl_sum->fmt_mode != fmt_mode) {
    ecl_sum->fmt_mode = fmt_mode;
    if (ecl_sum->header != NULL) ecl_fstate_set_fmt_mode(ecl_sum->header , fmt_mode);
    if (ecl_sum->data   != NULL) ecl_fstate_set_fmt_mode(ecl_sum->data , fmt_mode);
  }
}



int ecl_sum_get_Nwells(const ecl_sum_type *ecl_sum) {
  return ecl_sum->Nwells;
}

void ecl_sum_copy_well_names(const ecl_sum_type *ecl_sum , char **well_list) {
  int iw;
  for (iw=0; iw < ecl_sum->Nwells; iw++)
    strcpy(well_list[iw] , ecl_sum->well_list[iw]);
}


char ** ecl_sum_alloc_well_names_copy(const ecl_sum_type *ecl_sum) {
  char **well_list;
  int iw;
  well_list = calloc(ecl_sum->Nwells , sizeof *well_list);
  for (iw = 0; iw < ecl_sum->Nwells; iw++)
    well_list[iw] = malloc(strlen(ecl_sum->well_list[iw]) + 1);
  ecl_sum_copy_well_names(ecl_sum , well_list);
  return well_list;
}


int ecl_sum_get_size(const ecl_sum_type *ecl_sum) {
  return ecl_fstate_get_Nstep(ecl_sum->data);
}

void ecl_sum_free(ecl_sum_type *ecl_sum) {
  int i;
  ecl_fstate_free(ecl_sum->header);
  ecl_fstate_free(ecl_sum->data);

  hash_free(ecl_sum->index_hash);

  for (i=0; i < ecl_sum->Nwells; i++)
    free(ecl_sum->well_list[i]);
  free(ecl_sum->well_list);

  if (ecl_sum->base_name != NULL)
    free(ecl_sum->base_name);
  free(ecl_sum);
}

