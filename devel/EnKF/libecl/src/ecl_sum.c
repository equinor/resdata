#include <string.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <ecl_sum.h>
#include <hash.h>
#include <util.h>
#include <time.h>
#include <set.h>

#define ECL_DUMMY_WELL ":+:+:+:+"



struct ecl_sum_struct {
  ecl_fstate_type * header;
  ecl_fstate_type * data;
  hash_type       * index_hash;
  hash_type       *_index_hash;
  hash_type       *kw_index_hash;
  int               fmt_mode;
  int               Nwells , Nvars , param_offset;
  char            **well_list;
  char            * base_name;
  bool              endian_convert;
  bool              unified;
  time_t            sim_start_time;
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



static ecl_sum_type * ecl_sum_alloc_empty(int fmt_mode , bool endian_convert) {
  ecl_sum_type *ecl_sum;
  ecl_sum = malloc(sizeof *ecl_sum);
  ecl_sum->fmt_mode       = fmt_mode;
  ecl_sum->endian_convert = endian_convert;
  ecl_sum->unified        = true;  /* Dummy */
  ecl_sum->index_hash     = hash_alloc(10);
  ecl_sum->_index_hash    = hash_alloc(10);
  ecl_sum->kw_index_hash  = hash_alloc(10);
  ecl_sum->header         = NULL;
  ecl_sum->data           = NULL;
  ecl_sum->well_list      = NULL;
  ecl_sum->base_name      = NULL;
  ecl_sum->sim_start_time = -1;
  return ecl_sum;
}



void ecl_sum_fread_alloc_data(ecl_sum_type * sum , int files , const char **data_files , bool report_mode) {
  if (files <= 0) {
    fprintf(stderr,"%s: number of data files = %d - aborting \n",__func__ , files);
    abort();
  }
  {
    ecl_file_type file_type;
    bool fmt_file;
    int report_nr;
    
    ecl_util_get_file_type(data_files[0] , &file_type , &fmt_file , &report_nr);
    sum->data  = ecl_fstate_fread_alloc(files , data_files   , file_type , report_mode , sum->endian_convert);
    /*
      Check size of PARAMS block...
      {
      ecl_block_type * data_block1 = ecl_fstate_get_block();
      }
    */
  }
}
  

ecl_sum_type * ecl_sum_fread_alloc(const char *header_file , int files , const char **data_files , bool report_mode , bool endian_convert) {
  ecl_sum_type *ecl_sum   = ecl_sum_alloc_empty(ECL_FMT_AUTO , endian_convert);
  ecl_sum->header         = ecl_fstate_fread_alloc(1     , &header_file , ecl_summary_header_file , false , ecl_sum->endian_convert);
  ecl_sum_fread_alloc_data(ecl_sum , files , data_files , report_mode);
  {
    char well[9] , kw[9];
    int *date;
    ecl_block_type * block = ecl_fstate_iget_block(ecl_sum->header , 0);
    ecl_kw_type *wells     = ecl_block_get_kw(block , "WGNAMES"); 
    ecl_kw_type *keywords  = ecl_block_get_kw(block , "KEYWORDS"); 
    ecl_kw_type *startdat  = ecl_block_get_kw(block , "STARTDAT");
    hash_type   *well_hash = hash_alloc(10);
    int index;
      
    if (startdat == NULL) {
      fprintf(stderr,"%s could not locate STARTDAT keyword in header - aborting \n",__func__);
      abort();
    }
    date = ecl_kw_get_data_ref(startdat);
    ecl_sum->sim_start_time = util_make_time1(date[0] , date[1] , date[2]);
    {
      set_type * kw_set = set_alloc(0 , NULL);
      for (index=0; index < ecl_kw_get_size(keywords); index++) {
	char *kw_s;
	kw_s = util_alloc_strip_copy(ecl_kw_iget_ptr(keywords , index));
	if (set_add_key(kw_set , kw_s)) 
	  hash_insert_int(ecl_sum->kw_index_hash , kw_s , index);
      }
      set_free(kw_set);
    }
    

    for (index=0; index < ecl_kw_get_size(wells); index++) {
      char well_kw[17];
      char *well_s;
      well_s = util_alloc_strip_copy(ecl_kw_iget_ptr(wells , index));
      set_well_kw_string(ecl_kw_iget_ptr(wells , index) , ecl_kw_iget_ptr(keywords , index) , well_kw);
      hash_insert_int(ecl_sum->index_hash , well_kw , index);
      hash_insert_int(well_hash , ecl_kw_iget_ptr(wells , index) , 1);
      free(well_s);
    }
    for (index=0; index < ecl_kw_get_size(wells); index++) {
      util_set_strip_copy(well , ecl_kw_iget_ptr(wells    , index));
      util_set_strip_copy(kw , ecl_kw_iget_ptr(keywords , index));
	
      if (!hash_has_key(ecl_sum->_index_hash , well)) 
	hash_insert_hash_owned_ref(ecl_sum->_index_hash , well , hash_alloc(10) , hash_free__);
	
      {
	hash_type * var_hash = hash_get(ecl_sum->_index_hash , well);
	hash_insert_int(var_hash , kw , index);
      }
    }
      
    ecl_sum->Nwells    = hash_get_size(well_hash);
    ecl_sum->well_list = hash_alloc_keylist(well_hash);
    hash_free(well_hash);
  }
  {
    /*
      Should probably check that they are in the table first ....
    */
    int iblock;
    /*
      int time_index  = hash_get_int(ecl_sum->kw_index_hash , "TIME");
      int years_index = hash_get_int(ecl_sum->kw_index_hash , "YEARS");
    */
    int day_index   = hash_get_int(ecl_sum->kw_index_hash , "DAY");
    int month_index = hash_get_int(ecl_sum->kw_index_hash , "MONTH");
    int year_index  = hash_get_int(ecl_sum->kw_index_hash , "YEAR");

    for (iblock = 0; iblock < ecl_fstate_get_size(ecl_sum->data); iblock++) {
      ecl_block_type * block = ecl_fstate_get_block(ecl_sum->data , iblock);
      ecl_block_set_sim_time_summary(block , /*time_index , years_index , */ day_index , month_index , year_index);
    }
  }
  

  return ecl_sum;
}
	

									
static void ecl_sum_set_unified(ecl_sum_type *ecl_sum , bool unified) {
  ecl_sum->unified = unified;
  ecl_fstate_set_unified(ecl_sum->data , unified);
}



ecl_sum_type * ecl_sum_alloc_new(const char *base_name , int Nwells, int Nvars, int param_offset , int fmt_mode , bool report_mode , bool endian_convert , bool unified) {
  ecl_sum_type *ecl_sum = ecl_sum_alloc_empty(fmt_mode , endian_convert );
  ecl_sum_set_unified(ecl_sum , unified);
  ecl_sum->header       = ecl_fstate_alloc_empty(fmt_mode , ecl_summary_header_file , false , endian_convert);
  if (unified)
    ecl_sum->data         = ecl_fstate_alloc_empty(fmt_mode , ecl_unified_summary_file , false , endian_convert);
  else
    ecl_sum->data         = ecl_fstate_alloc_empty(fmt_mode , ecl_summary_file , report_mode , endian_convert);

  ecl_sum->base_name    = calloc(strlen(base_name) + 1 , sizeof *ecl_sum->base_name);
  ecl_sum->Nwells       = Nwells;
  ecl_sum->Nvars        = Nvars;
  ecl_sum->param_offset = param_offset;
  strcpy(ecl_sum->base_name , base_name);
  {
    const int size = param_offset + Nwells * Nvars;
    bool FMT_FILE;
    if (ecl_sum->fmt_mode == ECL_FORMATTED) 
      FMT_FILE = true;
    else
      FMT_FILE = false;
    
    ecl_block_type *header_block = ecl_block_alloc(0 , FMT_FILE , ecl_sum->endian_convert);
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
    
    ecl_kw_set_header_alloc(kw       , "KEYWORDS" , size , "CHAR");
    ecl_kw_set_header_alloc(units    , "UNITS"    , size , "CHAR");
    ecl_kw_set_header_alloc(restart  , "RESTART"  , 9    , "CHAR");
    ecl_kw_set_header_alloc(dimens   , "DIMENS"   , 6    , "INTE");
    ecl_kw_set_header_alloc(wells    , "WGNAMES"  , size , "CHAR");
    ecl_kw_set_header_alloc(nums     , "NUMS"     , size , "INTE");
    ecl_kw_set_header_alloc(startdat , "STARTDAT" , 3    , "INTE");
    
    ecl_block_add_kw(header_block , restart, COPY);
    ecl_block_add_kw(header_block , dimens , COPY);
    ecl_block_add_kw(header_block , kw , COPY);
    ecl_block_add_kw(header_block , wells , COPY);
    ecl_block_add_kw(header_block , nums , COPY);
    ecl_block_add_kw(header_block , units, COPY);
    ecl_block_add_kw(header_block , startdat, COPY);
    
    ecl_fstate_add_block(ecl_sum->header , header_block);
  }
  return ecl_sum;
}


void ecl_sum_set_header_data(ecl_sum_type *ecl_sum , const char *kw , void *value_ptr) {
  ecl_block_type *   block = ecl_fstate_iget_block(ecl_sum->header , 0 );
  ecl_kw_type     * ecl_kw = ecl_block_get_kw(block , kw);
  ecl_kw_set_memcpy_data(ecl_kw , value_ptr);
}



/*
  Format

  WGNAMES = 
  Dummy1 Dummy2 Dummy3  Well1 Well2 Well3
  Well4 Well1 Well2 Well3 Well4 Well1 Well2 ....


  KEYWORDS = 
  
*/



void ecl_sum_set_well_header(ecl_sum_type *ecl_sum, const char **_well_list) {
  ecl_block_type * block   = ecl_fstate_iget_block(ecl_sum->header , 0);
  ecl_kw_type     * ecl_kw = ecl_block_get_kw(block , "WGNAMES");
  {
    const char null_char = '\0';
    char *well_list = malloc(ecl_kw_get_size(ecl_kw) * (1 + ecl_str_len));
    char *well;
    int iw , ivar;
    for (iw = 0; iw < ecl_sum->param_offset; iw++) {
      well = &well_list[iw * (1 + ecl_str_len)];
      sprintf(well , ECL_DUMMY_WELL);
    }

    for (ivar = 0; ivar < ecl_sum->Nvars; ivar++) {
      for (iw = 0; iw < ecl_sum->Nwells; iw++) {
	well = &well_list[(ecl_sum->param_offset + ivar*ecl_sum->Nwells + iw) * (1 + ecl_str_len)];
	strcpy(well , _well_list[iw]);
	well[ecl_str_len] = null_char;
      }
    }
    
    ecl_kw_set_memcpy_data(ecl_kw , well_list);
    free(well_list);
  }
}


void ecl_sum_set_kw_header(ecl_sum_type * ecl_sum , const char **_var_list) {
}



void ecl_sum_init_save(ecl_sum_type * ecl_sum , const char * base_name , int fmt_mode , bool unified) {
  ecl_sum->base_name = calloc(strlen(base_name) + 1 , sizeof *ecl_sum->base_name);
  strcpy(ecl_sum->base_name , base_name);

  ecl_sum_set_fmt_mode(ecl_sum , fmt_mode);
  ecl_sum_set_unified(ecl_sum , unified);
}


void ecl_sum_save(const ecl_sum_type * ecl_sum) {
  char *summary_spec , ext[2] , *data_file;
  bool fmt_file;
  if (ecl_sum->base_name == NULL || !(ecl_sum->fmt_mode == ECL_FORMATTED || ecl_sum->fmt_mode == ECL_BINARY)) {
    fprintf(stderr,"%s: must inititialise ecl_sum object prior to saving - aborting \n",__func__);
    abort();
  }
  
  if (ecl_sum->fmt_mode == ECL_FORMATTED) {
    fmt_file = true;
  } else {
    fmt_file = false;
    sprintf(ext , "S");
  }
  summary_spec = ecl_util_alloc_filename(NULL , ecl_sum->base_name ,  ecl_summary_header_file , fmt_file , -1);
  ecl_fstate_set_files(ecl_sum->header , 1 , (const char **) &summary_spec);
  

  if (ecl_sum->unified) {
    data_file = ecl_util_alloc_filename(NULL , ecl_sum->base_name ,  ecl_unified_summary_file , fmt_file , -1);
    ecl_fstate_set_files(ecl_sum->data , 1 , (const char **) &data_file);
    free(data_file);
  } else {
    int files , report_nr1 , report_nr2;
    char **filelist;

    files = ecl_fstate_get_report_size(ecl_sum->data , &report_nr1 , &report_nr2);
    filelist = ecl_util_alloc_filelist(NULL , ecl_sum->base_name , ecl_summary_file , fmt_file , report_nr1 , report_nr2);
    ecl_fstate_set_files(ecl_sum->data , files , (const char **) filelist);
    util_free_string_list(filelist , files);
    
  }
  
  ecl_fstate_save(ecl_sum->header);
  ecl_fstate_save(ecl_sum->data);
  free(summary_spec);
}



static void ecl_sum_assert_index(const ecl_kw_type *params_kw, int index) {
  if (index < 0 || index >= ecl_kw_get_size(params_kw)) {
    fprintf(stderr,"%s index:%d invalid - aborting \n",__func__ , index);
    abort();
  }
}


double ecl_sum_iget2(const ecl_sum_type *ecl_sum , int time_index , int sum_index) {
  if (ecl_sum->data == NULL) {
    fprintf(stderr,"%s: data not loaded - aborting \n",__func__);
    abort();
  }
  {
    float tmp;
    ecl_block_type * block = ecl_fstate_get_block(ecl_sum->data , time_index);
    ecl_kw_type * data_kw  = ecl_block_get_kw(block , "PARAMS");
    ecl_sum_assert_index(data_kw , sum_index);
    ecl_kw_iget(data_kw , sum_index , &tmp); /* PARAMS underlying data type is float. */
    return tmp;
  }
}


int ecl_sum_get_index(const ecl_sum_type * ecl_sum , const char * well_name , const char *var_name) {
  char well_kw[17];
  set_well_kw_string(well_name , var_name , well_kw);
  if (!hash_has_key(ecl_sum->index_hash , well_kw)) 
    return -1;
  else
    return hash_get_int(ecl_sum->index_hash , well_kw);
}


bool ecl_sum_has_kw(const ecl_sum_type * ecl_sum , const char * well_name , const char *var_name) {
  if (ecl_sum_get_index(ecl_sum , well_name , var_name) >= 0)
    return true;
  else
    return false;
}



double ecl_sum_iget1(const ecl_sum_type *ecl_sum , int time_index , const char *well_name , const char *var_name , int *_sum_index) {
  int sum_index;
  double value;
  
  sum_index = ecl_sum_get_index(ecl_sum , well_name , var_name);
  value = ecl_sum_iget2(ecl_sum , time_index , sum_index);
  {
    char well[9] , kw[9];
    int _index;
    util_set_strip_copy(well , well_name);
    util_set_strip_copy(kw   , var_name);

    if (hash_has_key(ecl_sum->_index_hash , well)) {
      hash_type * var_hash = hash_get(ecl_sum->_index_hash , well);
      if (hash_has_key(var_hash , kw))
	_index = hash_get_int(var_hash , kw);
      else
	_index = -1;
    } else
      _index = -1;
    
    if (sum_index != _index) {
      fprintf(stderr,"%s fatal error _index != index - aborting \n",__func__);
      abort();
    }
  }
  
  if (_sum_index != NULL)
    *_sum_index = sum_index;
  return value;
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
  for (iw = 0; iw < ecl_sum->Nwells; iw++) {
    well_list[iw] = malloc(strlen(ecl_sum->well_list[iw]) + 1);
    well_list[iw][0] = '\0';
  }
  ecl_sum_copy_well_names(ecl_sum , well_list);
  return well_list;
}


int ecl_sum_get_size(const ecl_sum_type *ecl_sum) {
  return ecl_fstate_get_size(ecl_sum->data);
}


bool ecl_sum_get_report_mode(const ecl_sum_type * ecl_sum) {
  return ecl_fstate_get_report_mode(ecl_sum->data);
}


time_t ecl_sum_get_start_time(const ecl_sum_type * ecl_sum) {
  return ecl_sum->sim_start_time;
}


time_t ecl_sum_get_sim_time(const ecl_sum_type * ecl_sum , int index) {
  ecl_block_type * block = ecl_fstate_get_block(ecl_sum->data , index);
  return ecl_block_get_sim_time(block);
}


int ecl_sum_get_report_size(const ecl_sum_type * ecl_sum , int * first_report_nr , int * last_report_nr) {
  return ecl_fstate_get_report_size(ecl_sum->data , first_report_nr , last_report_nr);
}


void ecl_sum_free_data(ecl_sum_type * ecl_sum) {
  ecl_fstate_free(ecl_sum->data);
  ecl_sum->data = NULL;
}


void ecl_sum_free(ecl_sum_type *ecl_sum) {
  int i;
  ecl_fstate_free(ecl_sum->header);

  hash_free(ecl_sum->index_hash);
  hash_free(ecl_sum->_index_hash);
  hash_free(ecl_sum->kw_index_hash);

  for (i=0; i < ecl_sum->Nwells; i++)
    free(ecl_sum->well_list[i]);
  free(ecl_sum->well_list);

  if (ecl_sum->base_name != NULL)
    free(ecl_sum->base_name);
  free(ecl_sum);
}

