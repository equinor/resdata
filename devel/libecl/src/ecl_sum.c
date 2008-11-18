#include <string.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <math.h>
#include <ecl_fstate.h>
#include <ecl_sum.h>
#include <hash.h>
#include <util.h>
#include <time.h>
#include <set.h>
#include <util.h>



#define DUMMY_WELL(well) (strcmp((well) , ":+:+:+:+") == 0)
#define ECL_SUM_ID 89067


struct ecl_sum_struct {
  int                __id;                      /* Funny integer used for for "safe" run-time casting. */
  ecl_fstate_type  * data;
  ecl_fstate_type  * header;

  hash_type        * well_var_index;             /* Indexes for all well variables. */
  hash_type        * well_completion_var_index;  /* Indexes for completion indexes .*/
  hash_type        * group_var_index;            /* Indexes for group variables.*/ 
  hash_type        * field_var_index;
  hash_type        * region_var_index;           /* The stored index is an offset. */
  hash_type        * misc_var_index; 
  hash_type        * unit_hash;
  hash_type        * block_var_index; 

  ecl_sum_var_type * var_type;
  int               report_offset;
  int               num_regions;
  int               fmt_mode;
  int               Nwells , param_offset;
  int               params_size;
  char            **well_list;
  char            * base_name;
  bool              endian_convert;
  bool              unified;
  time_t            sim_start_time;
};

/**
   About indexing:
   ---------------


*/




static ecl_sum_type * ecl_sum_alloc_empty(int fmt_mode , bool endian_convert) {
  ecl_sum_type *ecl_sum;
  ecl_sum = util_malloc(sizeof *ecl_sum , __func__);
  ecl_sum->fmt_mode           	     = fmt_mode;
  ecl_sum->endian_convert     	     = endian_convert;
  ecl_sum->unified            	     = true;  /* Dummy */
  ecl_sum->well_var_index     	     = hash_alloc();
  ecl_sum->well_completion_var_index = hash_alloc();
  ecl_sum->group_var_index    	     = hash_alloc();
  ecl_sum->field_var_index    	     = hash_alloc();
  ecl_sum->region_var_index   	     = hash_alloc();
  ecl_sum->misc_var_index     	     = hash_alloc();
  ecl_sum->unit_hash          	     = hash_alloc();
  ecl_sum->block_var_index           = hash_alloc();
  ecl_sum->var_type           	     = NULL;
  ecl_sum->header             	     = NULL;
  ecl_sum->data               	     = NULL;
  ecl_sum->well_list          	     = NULL;
  ecl_sum->base_name          	     = NULL;
  ecl_sum->sim_start_time     	     = -1;  
  ecl_sum->report_offset             = 0;
  ecl_sum->__id                      = ECL_SUM_ID;
  return ecl_sum;
}




/**
   This looks up the last PARAMS instance - an EnKF adaption.
*/
static ecl_kw_type * ecl_sum_get_PARAMS_kw(const ecl_block_type * ecl_block) {
  ecl_kw_type    * params_kw   = ecl_block_get_last_kw(ecl_block , "PARAMS");       /* enkf adaption by taking the last */
  return params_kw;
}



ecl_sum_type * ecl_sum_safe_cast(const void * __ecl_sum) {
  ecl_sum_type * ecl_sum = (ecl_sum_type *) __ecl_sum;
  if (ecl_sum->__id != ECL_SUM_ID)
    util_abort("%s: runtime cast failed - aborting. \n",__func__);
  return ecl_sum;
}


void ecl_sum_fread_alloc_data(ecl_sum_type * sum , int files , const char **data_files , bool report_mode) {
  if (files <= 0) {
    fprintf(stderr,"%s: number of data files = %d - aborting \n",__func__ , files);
    abort();
  }
  {
    ecl_file_type file_type;
    bool promise_RPTONLY = false;
    bool fmt_file;
    int report_nr;
    
    ecl_util_get_file_type(data_files[0] , &file_type , &fmt_file , &report_nr);
    /*
      Burde kanskje vaere litt forsiktig med dette loftet
      for unified filer??
    */
    if (report_mode)
      promise_RPTONLY = true;
    sum->data     = ecl_fstate_fread_alloc(files  , data_files   , file_type , sum->endian_convert , promise_RPTONLY);
    /*
      Check size of PARAMS block...
    */
    {
      const ecl_block_type * data_block  = ecl_fstate_iget_block(sum->data , 0);
      sum->params_size = ecl_kw_get_size(ecl_sum_get_PARAMS_kw(data_block));
    }
    sum->var_type    = util_malloc( sum->params_size * sizeof * sum->var_type , __func__);
  }
}
  

/* See table 3.4 in the ECLIPSE file format reference manual. */

static ecl_sum_var_type __ecl_sum_identify_var_type(const char * var) {
  ecl_sum_var_type var_type = ecl_sum_misc_var;
  switch(var[0]) {
  case('A'):
    var_type = ecl_sum_aquifer_var;
    break;
  case('B'):
    var_type = ecl_sum_block_var;
    break;
  case('C'):
    var_type = ecl_sum_completion_var;
    break;
  case('F'):
    var_type = ecl_sum_field_var;
    break;
  case('G'):
    var_type = ecl_sum_group_var;
    break;
  case('L'): 
    switch(var[1]) {
    case('B'):
      var_type = ecl_sum_local_block_var;
      break;
    case('C'):
      var_type = ecl_sum_local_completion_var;
      break;
    case('W'):
      var_type = ecl_sum_local_well_var;
      break;
    default:
      util_abort("%s: not recognized: %s \n",__func__ , var);
    }
    break;
  case('N'):
    var_type = ecl_sum_network_var;
    break;
  case('R'):
    if (var[2] == 'F')
      var_type  = ecl_sum_region_2_region_var;
    else
      var_type  = ecl_sum_region_var;
    break;
  case('S'):
    var_type = ecl_sum_segment_var;
    break;
  case('W'):
    var_type = ecl_sum_well_var;
    break;
  default:
    /*
      It is unfortunately impossible to recognize an error situation -
      the rest just goes in "other" variables.
    */
    var_type = ecl_sum_misc_var;
  }
  return var_type;
}



static void ecl_sum_fread_header(ecl_sum_type * ecl_sum, const char * header_file) {
  ecl_sum->header         = ecl_fstate_fread_alloc(1     , &header_file , ecl_summary_header_file , ecl_sum->endian_convert , false);
  {
    int *date;
    ecl_block_type * block = ecl_fstate_iget_block(ecl_sum->header , 0);
    ecl_kw_type *wells     = ecl_block_get_kw(block , "WGNAMES"); 
    ecl_kw_type *keywords  = ecl_block_get_kw(block , "KEYWORDS"); 
    ecl_kw_type *startdat  = ecl_block_get_kw(block , "STARTDAT");
    ecl_kw_type *units     = ecl_block_get_kw(block , "UNITS");
    ecl_kw_type *nums      = NULL;
    int index;
    ecl_sum->num_regions     = 0;
    
    if (startdat == NULL) {
      fprintf(stderr,"%s: could not locate STARTDAT keyword in header - aborting \n",__func__);
      abort();
    }
    if (ecl_block_has_kw(block , "NUMS"))
      nums = ecl_block_get_kw(block , "NUMS");

    date = ecl_kw_get_int_ptr(startdat);
    ecl_sum->sim_start_time = util_make_date(date[0] , date[1] , date[2]);
    {
      /*
	Fills a unit_hash: unit_hash["WOPR"] =	"Barels/day"...
      */
	
      for (index=0; index < ecl_kw_get_size(keywords); index++) {
	char * kw = util_alloc_strip_copy( ecl_kw_iget_ptr(keywords , index));
	if (!hash_has_key(ecl_sum->unit_hash , kw)) {
	  char * unit = util_alloc_strip_copy(ecl_kw_iget_ptr(units , index));
	  hash_insert_hash_owned_ref(ecl_sum->unit_hash , kw , unit , free);
	}
	free(kw);
      }
    }
    
    {
      set_type *well_set  = set_alloc_empty();
      int num = -1;
      for (index=0; index < ecl_kw_get_size(wells); index++) {
	char * well = util_alloc_strip_copy(ecl_kw_iget_ptr(wells    , index));
	char * kw   = util_alloc_strip_copy(ecl_kw_iget_ptr(keywords , index));
	if (nums != NULL) num = ecl_kw_iget_int(nums , index);
	ecl_sum->var_type[index] = __ecl_sum_identify_var_type(kw);
	/* See table 3.4 in the ECLIPSE file format reference manual. */
	switch(ecl_sum->var_type[index]) {
	case(ecl_sum_completion_var):
	  /* Three level indexing: well -> string(cell_nr) -> variable */
	  if (!DUMMY_WELL(well)) {
	    /* Seems I have to accept nums < 0 to get shit through ??? */
	    char cell_str[16];
	    if (!hash_has_key(ecl_sum->well_completion_var_index , well)) 
		hash_insert_hash_owned_ref(ecl_sum->well_completion_var_index , well , hash_alloc() , hash_free__);
	    {
	      hash_type * cell_hash = hash_get(ecl_sum->well_completion_var_index , well);
	      sprintf(cell_str , "%d" , num);
	      if (!hash_has_key(cell_hash , cell_str)) 
		hash_insert_hash_owned_ref(cell_hash , cell_str , hash_alloc() , hash_free__);
	      {
		hash_type * var_hash = hash_get(cell_hash , cell_str);
		hash_insert_int(var_hash , kw , index);
	      }
	    }
	  } else 
	    util_abort("%s: incorrectly formatted completion var in SMSPEC. num:%d kw:\"%s\"  well:\"%s\" \n",__func__ , num , kw , well);
	  break;
	case(ecl_sum_field_var):
	  /* 
	     Field variable 
	  */
	  hash_insert_int(ecl_sum->field_var_index , kw , index);
	  break;
	case(ecl_sum_group_var):
	  {
	    const char * group = well;
	    if (!DUMMY_WELL(well)) {
	      if (!hash_has_key(ecl_sum->group_var_index , group)) 
		hash_insert_hash_owned_ref(ecl_sum->group_var_index , group , hash_alloc() , hash_free__);
	      {
		hash_type * var_hash = hash_get(ecl_sum->group_var_index , group);
		hash_insert_int(var_hash , kw , index);
	      }
	    }
	  }
	  break;
	case(ecl_sum_region_var):
	  if (!hash_has_key(ecl_sum->region_var_index , kw)) 
	    hash_insert_int(ecl_sum->region_var_index , kw , index);
	  ecl_sum->num_regions = util_int_max(ecl_sum->num_regions , num);  
	  break;
	case (ecl_sum_well_var):
	  if (!DUMMY_WELL(well)) {
	    /* 
	       It seems we can have e.g. WOPR associated with the dummy well, 
	       there is no limit to the stupidity of these programmers.
	    */
	    set_add_key(well_set , well);
	    if (!hash_has_key(ecl_sum->well_var_index , well)) 
	      hash_insert_hash_owned_ref(ecl_sum->well_var_index , well , hash_alloc() , hash_free__);
	    {
	      hash_type * var_hash = hash_get(ecl_sum->well_var_index , well);
	      hash_insert_int(var_hash , kw , index);
	    }   
	  }
	  break;
	case(ecl_sum_misc_var):
	  /* 
	     Possibly we must have the possibility to alter 
	     reclassify - so this last switch must be done
	     in two passes?
	  */
	  hash_insert_int(ecl_sum->misc_var_index , kw , index);	    
	  break;
	case(ecl_sum_block_var):
	  /* A block variable */
	  {
	    char * block_nr  = util_alloc_sprintf("%d" , num);
	    if (!hash_has_key(ecl_sum->block_var_index , kw))
	      hash_insert_hash_owned_ref(ecl_sum->block_var_index , kw , hash_alloc() , hash_free__);
	    {
	      hash_type * block_hash = hash_get(ecl_sum->block_var_index , kw);
	      hash_insert_int(block_hash , block_nr , index);	    
	    }
	    free(block_nr);
	  }
	default:
	  /* Lots of legitimate alternatives which are not handled .. */
	  break;
	}
	free(kw);
	free(well);
      }
      ecl_sum->Nwells    = set_get_size(well_set);
      ecl_sum->well_list = set_alloc_keylist(well_set);
      set_free(well_set);
    }
  }
}


ecl_sum_type * ecl_sum_fread_alloc(const char *header_file , int files , const char **data_files , bool report_mode , bool endian_convert) {
  ecl_sum_type *ecl_sum   = ecl_sum_alloc_empty(ECL_FMT_AUTO , endian_convert);

  ecl_sum_fread_alloc_data(ecl_sum , files , data_files , report_mode);
  ecl_sum_fread_header(ecl_sum , header_file);
  if (hash_has_key(ecl_sum->misc_var_index , "DAY")) {
    int iblock;
    int day_index   = hash_get_int(ecl_sum->misc_var_index , "DAY");
    int month_index = hash_get_int(ecl_sum->misc_var_index , "MONTH");
    int year_index  = hash_get_int(ecl_sum->misc_var_index , "YEAR");
    
    for (iblock = 0; iblock < ecl_fstate_get_size(ecl_sum->data); iblock++) {
      ecl_block_type * block = ecl_fstate_iget_block(ecl_sum->data , iblock);
      ecl_block_set_sim_time_summary(block , /*time_index , years_index , */ day_index , month_index , year_index);
    }
  } else 
    fprintf(stderr,"** Warning: could not locate DAY / MONTH / YEAR in %s - dates will be defaulted. Add 'DATE' to the summary section in the .DATA file to prevent this problem.\n", header_file);
  
  return ecl_sum;
}
	

									
static void ecl_sum_set_unified(ecl_sum_type *ecl_sum , bool unified) {
  ecl_sum->unified = unified;
  ecl_fstate_set_unified(ecl_sum->data , unified);
}




void ecl_sum_set_header_data(ecl_sum_type *ecl_sum , const char *kw , void *value_ptr) {
  ecl_block_type *   block = ecl_fstate_iget_block(ecl_sum->header , 0 );
  ecl_kw_type     * ecl_kw = ecl_block_get_kw(block , kw);
  ecl_kw_set_memcpy_data(ecl_kw , value_ptr);
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
    filelist = ecl_util_alloc_simple_filelist(NULL , ecl_sum->base_name , ecl_summary_file , fmt_file , report_nr1 , report_nr2);
    ecl_fstate_set_files(ecl_sum->data , files , (const char **) filelist);
    util_free_stringlist(filelist , files);
  }
  
  ecl_fstate_save(ecl_sum->header);
  ecl_fstate_save(ecl_sum->data);
  free(summary_spec);
}



static void ecl_sum_assert_index(const ecl_sum_type * ecl_sum, int index) {
  if (index < 0 || index >= ecl_sum->params_size) 
    util_abort("%s: index:%d invalid - aborting \n",__func__ , index);
}





bool ecl_sum_has_report_nr(const ecl_sum_type * ecl_sum, int report_nr) {
  return ecl_fstate_has_report_nr(ecl_sum->data , report_nr);
}

/*
  time_index is zero based anyway ..... not nice 
*/
double ecl_sum_get_with_index(const ecl_sum_type *ecl_sum , int report_nr , int sum_index) {
  if (ecl_sum->data == NULL) 
    util_abort("%s: data not loaded - aborting \n",__func__);
  ecl_sum_assert_index(ecl_sum , sum_index);
  {
    ecl_block_type * block    = ecl_fstate_get_block_by_report_nr(ecl_sum->data , report_nr);
    ecl_kw_type    * data_kw;
    if (block == NULL) 
      util_abort("%s: failed to get report_nr:%d - something broken ?! \n",__func__ , report_nr);
    
    data_kw  = ecl_sum_get_PARAMS_kw(block);
    return (double) ecl_kw_iget_float(data_kw , sum_index);    
  }
}


double ecl_sum_iget_with_index(const ecl_sum_type *ecl_sum , int block_nr , int sum_index) {
  if (ecl_sum->data == NULL) 
    util_abort("%s: data not loaded - aborting \n",__func__);
  ecl_sum_assert_index(ecl_sum , sum_index);
  {
    ecl_block_type * block    = ecl_fstate_iget_block(ecl_sum->data , block_nr);
    ecl_kw_type    * data_kw;
    if (block == NULL) 
      util_abort("%s: failed to get report_nr:%d - something broken ?! \n",__func__ , block_nr);
    
    data_kw  = ecl_sum_get_PARAMS_kw(block);
    return (double) ecl_kw_iget_float(data_kw , sum_index);    
  }
}


ecl_sum_var_type ecl_sum_iget_var_type(const ecl_sum_type * ecl_sum , int sum_index) {
  ecl_sum_assert_index(ecl_sum , sum_index);
  return ecl_sum->var_type[sum_index];
}


int ecl_sum_get_num_groups(const ecl_sum_type * ecl_sum) {
  return hash_get_size(ecl_sum->group_var_index);
}

char ** ecl_sum_alloc_group_names(const ecl_sum_type * ecl_sum) {
  return hash_alloc_keylist(ecl_sum->group_var_index);
}


int ecl_sum_get_num_regions(const ecl_sum_type * ecl_sum) {
  return ecl_sum->num_regions;
}


/*
static void ecl_sum_list_wells(const ecl_sum_type * ecl_sum) {
  int iw;
  char ** well_list = hash_alloc_keylist(ecl_sum->well_var_index);
  for (iw = 0; iw < hash_get_size(ecl_sum->well_var_index); iw++)
    printf("well[%03d] = %s \n",iw , well_list[iw]);
  util_free_stringlist(well_list , hash_get_size(ecl_sum->well_var_index));
}
*/


int ecl_sum_get_well_var_index(const ecl_sum_type * ecl_sum , const char * well , const char *var) {
  int index;

  if (hash_has_key(ecl_sum->well_var_index , well)) {
    hash_type * var_hash = hash_get(ecl_sum->well_var_index , well);
    if (hash_has_key(var_hash , var))
      index = hash_get_int(var_hash , var); 
    else {
      fprintf(stderr,"%s: summary object does not have well/variable combination: %s/%s  \n",__func__ , well , var);
      abort(); 
    }   
  } else {
    fprintf(stderr,"%s: summary object does not contain well: %s \n",__func__ , well);
    abort(); 
  }
  return index;
}




static int ecl_sum_get_well_completion_var_index__(const ecl_sum_type * ecl_sum , const char * well , const char *var, int cell_nr) {
  int index = -1;
  
  if (hash_has_key(ecl_sum->well_completion_var_index , well)) {
    char cell_str[16];
    hash_type * cell_hash = hash_get(ecl_sum->well_completion_var_index , well);
    sprintf(cell_str , "%d" , cell_nr);
    
    if (hash_has_key(cell_hash , cell_str)) {
      hash_type * var_hash = hash_get(cell_hash , cell_str);
      if (hash_has_key(var_hash , var))
	index = hash_get_int(var_hash , var);
    }
  }
  return index;
}



int ecl_sum_get_well_completion_var_index(const ecl_sum_type * ecl_sum , const char * well , const char *var, int cell_nr) {
  int index = ecl_sum_get_well_completion_var_index__(ecl_sum , well , var , cell_nr);
  if (index < 0) {
    fprintf(stderr,"%s: summary object does not have completion data for well:%s  variable:%s  cell_nr:%d \n",__func__ , well , var , cell_nr);
    abort(); 
  }
  return index;
}




int ecl_sum_get_group_var_index(const ecl_sum_type * ecl_sum , const char * group , const char *var) {
  int index;

  if (hash_has_key(ecl_sum->group_var_index , group)) {
    hash_type * var_hash = hash_get(ecl_sum->group_var_index , group);
    if (hash_has_key(var_hash , var))
      index = hash_get_int(var_hash , var); 
    else {
      fprintf(stderr,"%s: summary object does not have group/variable combination: %s/%s  \n",__func__ , group , var);
      abort(); 
    }   
  } else {
    fprintf(stderr,"%s: summary object does not contain group: %s \n",__func__ , group);
    abort(); 
  }
  return index;
}


/**
   Observe that block_nr is represented as char literal,
   i.e. "2345". This is because it will be used as a hash key.
*/

int ecl_sum_get_block_var_index(const ecl_sum_type * ecl_sum , const char * block_var , const char *block_nr) {
  int index = -1;
  
  if (hash_has_key(ecl_sum->block_var_index , block_var)) {
    hash_type * block_hash = hash_get(ecl_sum->block_var_index , block_var);
    if (hash_has_key(block_hash , block_nr))
      index = hash_get_int(block_hash , block_nr); 
    else 
      util_abort("%s: summary object does not have %s:%s combination. \n",__func__ , block_var , block_nr);
  } else 
    util_abort("%s: summary object does not contain group: block variable: \n",__func__ , block_var);

  return index;
}


int ecl_sum_get_field_var_index(const ecl_sum_type * ecl_sum , const char *var) {
  int index = -1;

  if (hash_has_key(ecl_sum->field_var_index , var)) 
    index = hash_get_int(ecl_sum->field_var_index , var); 
  else 
    util_abort("%s: summary object does not have field variable: %s  \n",__func__ , var);
    
  return index;
}


int ecl_sum_get_misc_var_index(const ecl_sum_type * ecl_sum , const char *var) {
  int index = -1;

  if (hash_has_key(ecl_sum->misc_var_index , var)) 
    index = hash_get_int(ecl_sum->misc_var_index , var); 
  else 
    util_abort("%s: summary object does not have misc variable: %s  \n",__func__ , var);

  return index;
}



/**
   region_nr: [1...num_regions] (NOT C-based indexing)
*/

static void ecl_sum_assert_region_nr(const ecl_sum_type * ecl_sum , int region_nr) {
  if (region_nr <= 0 || region_nr > ecl_sum->num_regions) 
    util_abort("%s: region_nr:%d not in valid range: [1,%d] - aborting \n",__func__ , region_nr , ecl_sum->num_regions);
}




int ecl_sum_get_region_var_index(const ecl_sum_type * ecl_sum , int region_nr , const char *var) {
  int index = -1;
  
  ecl_sum_assert_region_nr(ecl_sum , region_nr);
  if (hash_has_key(ecl_sum->region_var_index , var)) 
    index = region_nr + hash_get_int(ecl_sum->region_var_index , var) - 1;
  else 
    util_abort("%s: summary object does not have region variable: %s  \n",__func__ , var);
     
  return index;
}





bool ecl_sum_has_field_var(const ecl_sum_type * ecl_sum , const char *var) {
  return hash_has_key(ecl_sum->field_var_index , var);
}

bool ecl_sum_has_misc_var(const ecl_sum_type * ecl_sum , const char *var) {
  return hash_has_key(ecl_sum->misc_var_index , var);
}


bool ecl_sum_has_region_var(const ecl_sum_type * ecl_sum , int region_nr , const char *var) {
  ecl_sum_assert_region_nr(ecl_sum , region_nr);
  return hash_has_key(ecl_sum->region_var_index , var);
}


bool ecl_sum_has_well_var(const ecl_sum_type * ecl_sum , const char * well , const char *var) {
  if (hash_has_key(ecl_sum->well_var_index , well)) {
    hash_type * var_hash = hash_get(ecl_sum->well_var_index , well);
    if (hash_has_key(var_hash , var))
      return true;
    else 
      return false;
  } else 
    return false;
}

bool ecl_sum_has_group_var(const ecl_sum_type * ecl_sum , const char * group , const char *var) {
  if (hash_has_key(ecl_sum->group_var_index , group)) {
    hash_type * var_hash = hash_get(ecl_sum->group_var_index , group);
    return hash_has_key(var_hash , var);
  } else 
    return false;
}


bool ecl_sum_has_block_var(const ecl_sum_type * ecl_sum , const char * block_var , const char *block_nr) {
  if (hash_has_key(ecl_sum->block_var_index , block_var)) {
    hash_type * var_hash = hash_get(ecl_sum->block_var_index , block_nr);
    return hash_has_key(var_hash , block_nr);
  } else 
    return false;
}



#define __ASSERT_ARGC(argc , target_argc , s , t) if (argc != target_argc) util_abort("%s: string:%s is not recognized for lookup of %s." , __func__ , s , t);
int ecl_sum_get_general_var_index(const ecl_sum_type * ecl_sum , const char * lookup_kw) {
  int     index = -1;
  char ** argv;
  int     argc;
  ecl_sum_var_type var_type;
  util_split_string(lookup_kw , ":" , &argc , &argv);
  var_type = __ecl_sum_identify_var_type(argv[0]);

  switch(var_type) {
  case(ecl_sum_misc_var):
    index = ecl_sum_get_misc_var_index(ecl_sum , argv[0]);
    break;
  case(ecl_sum_well_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Wells");
    index = ecl_sum_get_well_var_index(ecl_sum , argv[1] , argv[0]);
    break;
  case(ecl_sum_region_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Regions");
    {
      int region_nr;
      if (!util_sscanf_int(argv[1] , &region_nr)) 
	util_abort("%s: failed to parse %s to an integer - aborting. \n",__func__ , argv[1]);
      index = ecl_sum_get_region_var_index( ecl_sum , region_nr , argv[0]);
    }
    break;
  case(ecl_sum_field_var):
    index = ecl_sum_get_field_var_index(ecl_sum , argv[0]);
    break;
  case(ecl_sum_group_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Groups");
    index = ecl_sum_get_group_var_index(ecl_sum , argv[1] , argv[0]);
    break;
  case(ecl_sum_block_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Block");
    index = ecl_sum_get_block_var_index(ecl_sum , argv[0] , argv[1]);
    break;
  default:
    util_abort("%s: sorry looking up the type:%d / %s is not (yet) implemented.\n" , __func__ , var_type , lookup_kw);
  }
  util_free_stringlist(argv , argc);
  return index;
}


bool ecl_sum_has_general_var(const ecl_sum_type * ecl_sum , const char * lookup_kw) {
  bool    has_var = false;
  char ** argv;
  int     argc;
  ecl_sum_var_type var_type;
  util_split_string(lookup_kw , ":" , &argc , &argv);
  var_type = __ecl_sum_identify_var_type(argv[0]);

  switch(var_type) {
  case(ecl_sum_misc_var):
    has_var = ecl_sum_has_misc_var(ecl_sum , argv[0]);
    break;
  case(ecl_sum_well_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Wells");
    has_var = ecl_sum_has_well_var(ecl_sum , argv[1] , argv[0]);
    break;
  case(ecl_sum_region_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Regions");
    {
      int region_nr;
      if (!util_sscanf_int(argv[1] , &region_nr)) 
	util_abort("%s: failed to parse %s to an integer - aborting. \n",__func__ , argv[1]);
      has_var = ecl_sum_has_region_var(ecl_sum , region_nr , argv[0]);
    }
    break;
  case(ecl_sum_field_var):
    has_var = ecl_sum_has_field_var(ecl_sum , argv[0]);
    break;
  case(ecl_sum_group_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Groups");
    has_var = ecl_sum_has_group_var(ecl_sum , argv[1] , argv[0]);
    break;
  case(ecl_sum_block_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Block");
    has_var = ecl_sum_has_block_var(ecl_sum , argv[0] , argv[1]);
    break;
  default:
    util_abort("%s: sorry looking up the type:%d / %s is not (yet) implemented.\n" , __func__ , var_type , lookup_kw);
  }
  util_free_stringlist(argv , argc);
  return has_var;
}
#undef __ASSERT_ARGC    



const char * ecl_sum_get_unit_ref(const ecl_sum_type * ecl_sum , const char *var) {
  if (hash_has_key(ecl_sum->unit_hash , var))
    return hash_get(ecl_sum->unit_hash , var);
  else {
    fprintf(stderr,"%s: variable:%s not defined - aborting \n",__func__ , var);
    abort();
  }
}


double ecl_sum_get_well_var(const ecl_sum_type *ecl_sum , int time_index , const char *well_name , const char *var_name) {
  int sum_index;
  double value;
  
  sum_index = ecl_sum_get_well_var_index(ecl_sum , well_name , var_name);
  value     = ecl_sum_get_with_index(ecl_sum , time_index , sum_index);
  
  return value;
}

double ecl_sum_get_well_completion_var(const ecl_sum_type *ecl_sum , int time_index , const char *well_name , const char *var_name , int completion_nr) {
  int sum_index;
  double value;
  
  sum_index = ecl_sum_get_well_completion_var_index(ecl_sum , well_name , var_name , completion_nr);
  value     = ecl_sum_get_with_index(ecl_sum , time_index , sum_index);
  
  return value;
}


double ecl_sum_get_group_var(const ecl_sum_type *ecl_sum , int time_index , const char *group_name , const char *var_name) {
  int sum_index;
  double value;
  
  sum_index = ecl_sum_get_group_var_index(ecl_sum , group_name , var_name);
  value     = ecl_sum_get_with_index(ecl_sum , time_index , sum_index);
  
  return value;
}


double ecl_sum_get_field_var(const ecl_sum_type * ecl_sum , int time_index , const char * var_name) {
  int sum_index;
  double value;

  sum_index = ecl_sum_get_field_var_index(ecl_sum , var_name);
  value     = ecl_sum_get_with_index(ecl_sum , time_index , sum_index);
  return value;
}


double ecl_sum_get_misc_var(const ecl_sum_type * ecl_sum , int time_index , const char * var_name) {
  int sum_index;
  double value;

  sum_index = ecl_sum_get_misc_var_index(ecl_sum , var_name);
  value     = ecl_sum_get_with_index(ecl_sum , time_index , sum_index);
  return value;
}




double ecl_sum_get_region_var(const ecl_sum_type * ecl_sum , int time_index , int region_nr , const char * var_name) {
  int sum_index;
  double value;

  sum_index = ecl_sum_get_region_var_index(ecl_sum , region_nr , var_name);
  value     = ecl_sum_get_with_index(ecl_sum , time_index , sum_index);
  return value;
}


double ecl_sum_get_general_var(const ecl_sum_type * ecl_sum , int time_index , const char * lookup_string) {
  int sum_index;
  double value;
  
  sum_index = ecl_sum_get_general_var_index(ecl_sum , lookup_string);
  value     = ecl_sum_get_with_index(ecl_sum , time_index , sum_index);
  
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


const char ** ecl_sum_get_well_names_ref(const ecl_sum_type * ecl_sum) {
  return (const char **) ecl_sum->well_list;
}


int ecl_sum_get_size(const ecl_sum_type *ecl_sum) {
  return ecl_fstate_get_size(ecl_sum->data);
}



time_t ecl_sum_get_start_time(const ecl_sum_type * ecl_sum) {
  return ecl_sum->sim_start_time;
}


time_t ecl_sum_iget_sim_time(const ecl_sum_type * ecl_sum , int index) {
  ecl_block_type * block = ecl_fstate_iget_block(ecl_sum->data , index);
  return ecl_block_get_sim_time(block);
}


time_t ecl_sum_get_sim_time(const ecl_sum_type * ecl_sum , int report_step) {
  ecl_block_type * block = ecl_fstate_get_block_by_report_nr(ecl_sum->data , report_step);
  return ecl_block_get_sim_time(block);
}



int ecl_sum_get_report_size(const ecl_sum_type * ecl_sum , int * first_report_nr , int * last_report_nr) {
  return ecl_fstate_get_report_size(ecl_sum->data , first_report_nr , last_report_nr);
}


double ecl_sum_eval_well_misfit(const ecl_sum_type * ecl_sum , const char * well , int nvar , const char ** var_list , const double * inv_covar) {
  double  R2;
  double *residual , *tmp;
  char **hvar_list;
  int istep,ivar;

  hvar_list = malloc(nvar * sizeof * hvar_list);
  for (ivar = 0; ivar < nvar; ivar++) {
    hvar_list[ivar] = malloc(strlen(var_list[ivar]) + 2);
    sprintf(hvar_list[ivar] , "%sH" , var_list[ivar]);
  }
  residual = malloc(nvar * sizeof * residual);
  tmp      = malloc(nvar * sizeof * tmp);

  R2 = 0;
  for (istep = 0; istep < ecl_sum_get_size(ecl_sum);  istep++) {
    double history_value , value;
    for (ivar = 0; ivar < nvar; ivar++) {
      if (ecl_sum_has_well_var(ecl_sum , well , hvar_list[ivar])) {
	history_value = ecl_sum_get_well_var(ecl_sum , istep , well , hvar_list[ivar]);
	value         = ecl_sum_get_well_var(ecl_sum , istep , well , var_list[ivar] );
	
	residual[ivar] = (history_value - value);
      }
    }
    
    {
      int i,j;
      for (i = 0; i < nvar; i++) {
	tmp[i] = 0;
	for (j = 0; j < nvar; j++) 
	  tmp[i] += residual[j] * inv_covar[j*nvar + i];
	R2 += tmp[i] * residual[i];
      }
    }
  }
  
  free(tmp);
  free(residual);
  util_free_stringlist(hvar_list , nvar);
  return R2;
}


void ecl_sum_well_max_min(const ecl_sum_type * ecl_sum, const char * well , int nvar , const char ** var_list , double *max , double *min, bool init) {
  int istep,ivar;

  if (init) {
    for (ivar = 0; ivar < nvar; ivar++) {
      max[ivar] = -1e100;
      min[ivar] =  1e100;
    }
  }

  for (istep = 0; istep < ecl_sum_get_size(ecl_sum);  istep++) {
    for (ivar = 0; ivar < nvar; ivar++) {
      if (ecl_sum_has_well_var(ecl_sum , well , var_list[ivar])) {
	double value = ecl_sum_get_well_var(ecl_sum , istep , well , var_list[ivar]);
	min[ivar]  = util_double_min(min[ivar] , value);
	max[ivar]  = util_double_max(max[ivar] , value);
      }
    }
  }
}



void ecl_sum_max_min(const ecl_sum_type * ecl_sum, int nwell , const char ** well_list , int nvar , const char ** var_list , double *max , double *min, bool init_maxmin) {
  int iwell;
  ecl_sum_well_max_min(ecl_sum , well_list[0] , nvar , var_list , max , min , init_maxmin);
  for (iwell = 1; iwell < nwell; iwell++) 
    ecl_sum_well_max_min(ecl_sum , well_list[iwell] , nvar , var_list , max , min , false);
  
}


double ecl_sum_eval_misfit(const ecl_sum_type * ecl_sum , int nwell , const char ** well_list , int nvar , const char ** var_list , const double * inv_covar, double * misfit) {
  int    iwell;
  double total_misfit = 0;
  for (iwell = 0; iwell < nwell; iwell++) {
    misfit[iwell] = ecl_sum_eval_well_misfit(ecl_sum , well_list[iwell] , nvar , var_list , inv_covar);
    total_misfit += misfit[iwell];
  }
  return total_misfit;
}





void ecl_sum_free_data(ecl_sum_type * ecl_sum) {
  ecl_fstate_free(ecl_sum->data);
  ecl_sum->data = NULL;
}


void ecl_sum_free(ecl_sum_type *ecl_sum) {
  ecl_sum_free_data(ecl_sum);
  ecl_fstate_free(ecl_sum->header);
  hash_free(ecl_sum->well_var_index);
  hash_free(ecl_sum->well_completion_var_index);
  hash_free(ecl_sum->group_var_index);
  hash_free(ecl_sum->field_var_index);
  hash_free(ecl_sum->region_var_index);
  hash_free(ecl_sum->misc_var_index);
  hash_free(ecl_sum->unit_hash);
  util_free_stringlist(ecl_sum->well_list  , ecl_sum->Nwells);
  free(ecl_sum->var_type);

  if (ecl_sum->base_name != NULL)
    free(ecl_sum->base_name);
  free(ecl_sum);
}


/**
   This is actually not a proper report_step - but rather an index ...
*/
void ecl_sum_fprintf(const ecl_sum_type * ecl_sum , FILE * stream , int nvars , const char ** var_list) {
  int report_step , first_report_step , last_report_step;
  ecl_sum_get_report_size(ecl_sum , &first_report_step , &last_report_step);

  for (report_step = first_report_step; report_step <= last_report_step; report_step++) {
    if (ecl_sum_has_report_nr(ecl_sum , report_step)) {
      int day,month,year,ivar;
      util_set_date_values(ecl_sum_get_sim_time(ecl_sum , report_step) , &day , &month, &year); 
      fprintf(stream , "%04d   %02d/%02d/%04d   ",report_step , day , month , year);
      
      for (ivar = 0; ivar < nvars; ivar++)
	fprintf(stream , " %12.3f " , ecl_sum_get_general_var(ecl_sum , report_step , var_list[ivar]));
      
      fprintf(stream , "\n");
    }
  }
  
}


/*****************************************************************/

ecl_sum_type * ecl_sum_fread_alloc_interactive(bool endian_convert) {
  char ** file_list   = NULL;
  char *  header_file = NULL;
  int     files;
  char * base;
  char   path[256];
  bool   report_mode = true;
  ecl_sum_type * ecl_sum;

  util_read_path("Directory to load ECLIPSE summary from" , 50 , true , path);
  base = ecl_util_alloc_base_guess(path);
  if (base == NULL) {
    base = util_malloc(9 , __func__);
    util_read_string("Basename for ECLIPSE simulation" , 50 , base);
  } else 
    printf("%-50s: %s\n" , "Using ECLIPSE base",base);
  
  {
    int formatted_files;
    int unformatted_files;
    int possibilities = 0;

    char * unified_formatted   = ecl_util_alloc_filename(path , base , ecl_unified_summary_file , true  , 0 );
    char * unified_unformatted = ecl_util_alloc_filename(path , base , ecl_unified_summary_file , false , 0 );

    char ** formatted_list     = ecl_util_alloc_scandir_filelist(path , base , ecl_summary_file , true  , &formatted_files  );
    char ** unformatted_list   = ecl_util_alloc_scandir_filelist(path , base , ecl_summary_file , false , &unformatted_files);
    
    char * formatted_header    = ecl_util_alloc_filename(path , base , ecl_summary_header_file , true  , 0 );
    char * unformatted_header  = ecl_util_alloc_filename(path , base , ecl_summary_header_file , false , 0 );
    
    if (util_file_exists(unified_formatted))   possibilities++;
    if (util_file_exists(unified_unformatted)) possibilities++;
    if (formatted_files > 0)                   possibilities++;
    if (unformatted_files > 0)                 possibilities++;

    if (possibilities == 0) {
      fprintf(stderr,"** WARNING: Could not find summary data in directory: %s ** \n",path);
      files     = 0;
      file_list = NULL;
      header_file = NULL;
    } else if (possibilities == 1) {
      if (possibilities == 1) {
	if (util_file_exists(unified_formatted)) {
	  files = 1;
	  file_list   = util_alloc_stringlist_copy((const char **) &unified_formatted , 1);
	  header_file = util_alloc_string_copy(formatted_header);
	} else if (util_file_exists(unified_unformatted)) {
	  files = 1;
	  file_list   = util_alloc_stringlist_copy((const char **) &unified_unformatted , 1);
	  header_file = util_alloc_string_copy(unformatted_header);
	} else if (formatted_files > 0) {
	  files = formatted_files;
	  file_list   = util_alloc_stringlist_copy((const char **) formatted_list , formatted_files);
	  header_file = util_alloc_string_copy(formatted_header);
	} else if (unformatted_files > 0) {
	  files = unformatted_files;
	  file_list   = util_alloc_stringlist_copy((const char **) unformatted_list , unformatted_files);
	  header_file = util_alloc_string_copy(unformatted_header);
	}
      }
    } else {
      bool fmt , unified;
      /* Should be read in interactively */  

      unified = false;
      fmt     = false;
      if (unified) {
	files = 1;
	file_list = malloc(sizeof * file_list);
	file_list[0] = ecl_util_alloc_exfilename(path , base , ecl_unified_summary_file , fmt  , 0 );
      } else 
	file_list = ecl_util_alloc_scandir_filelist(path , base , ecl_summary_file , fmt , &files);

      if (fmt)
	header_file = util_alloc_string_copy(formatted_header);
      else
	header_file = util_alloc_string_copy(unformatted_header);
      
    }
    free(formatted_header);
    free(unformatted_header);
    free(unified_formatted);
    free(unified_unformatted);
    util_free_stringlist(unformatted_list , unformatted_files);
    util_free_stringlist(formatted_list   , formatted_files);
  }
  
  
  if (files > 0) 
    ecl_sum = ecl_sum_fread_alloc(header_file , files , (const char ** ) file_list , report_mode , endian_convert);
  else 
    ecl_sum = NULL;
  
  util_free_stringlist(file_list , files); 
  if (header_file != NULL) free(header_file);
  free(base);

  return ecl_sum;
}


#undef ECL_SUM_ID
