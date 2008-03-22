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


struct ecl_sum_struct {
  ecl_fstate_type  * header;
  ecl_fstate_type  * data;
  hash_type        * well_var_index;
  hash_type        * well_completion_var_index;
  hash_type        * group_var_index;
  hash_type        * field_var_index;
  hash_type        * region_var_index;
  hash_type        * misc_var_index;
  hash_type        * unit_hash;
  ecl_sum_var_type * var_type;
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






static ecl_sum_type * ecl_sum_alloc_empty(int fmt_mode , bool endian_convert) {
  ecl_sum_type *ecl_sum;
  ecl_sum = malloc(sizeof *ecl_sum);
  ecl_sum->fmt_mode           	     = fmt_mode;
  ecl_sum->endian_convert     	     = endian_convert;
  ecl_sum->unified            	     = true;  /* Dummy */
  ecl_sum->well_var_index     	     = hash_alloc(10);
  ecl_sum->well_completion_var_index = hash_alloc(10);
  ecl_sum->group_var_index    	     = hash_alloc(10);
  ecl_sum->field_var_index    	     = hash_alloc(10);
  ecl_sum->region_var_index   	     = hash_alloc(10);
  ecl_sum->misc_var_index     	     = hash_alloc(10);
  ecl_sum->unit_hash          	     = hash_alloc(10);
  ecl_sum->var_type           	     = NULL;
  ecl_sum->header             	     = NULL;
  ecl_sum->data               	     = NULL;
  ecl_sum->well_list          	     = NULL;
  ecl_sum->base_name          	     = NULL;
  ecl_sum->sim_start_time     	     = -1;  
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
    sum->data     = ecl_fstate_fread_alloc(files , data_files   , file_type , report_mode , sum->endian_convert);
    /*
      Check size of PARAMS block...
    */
    {
      const ecl_block_type * data_block  = ecl_fstate_iget_block(sum->data , 0);
      const ecl_kw_type    * params_kw   = ecl_block_get_kw(data_block , "PARAMS");
      sum->params_size                   = ecl_kw_get_size(params_kw);
    }
    sum->var_type = util_malloc( sum->params_size * sizeof * sum->var_type , __func__);
  }
}
  




static void ecl_sum_fread_header(ecl_sum_type * ecl_sum, const char * header_file) {
  ecl_sum->header         = ecl_fstate_fread_alloc(1     , &header_file , ecl_summary_header_file , false , ecl_sum->endian_convert);
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
    ecl_sum->sim_start_time = util_make_time1(date[0] , date[1] , date[2]);
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
      bool misc_var;
      int num = -1;
      for (index=0; index < ecl_kw_get_size(wells); index++) {
	char * well = util_alloc_strip_copy(ecl_kw_iget_ptr(wells    , index));
	char * kw   = util_alloc_strip_copy(ecl_kw_iget_ptr(keywords , index));
	ecl_sum->var_type[index] = ecl_sum_NOT_IMPLEMENTED_var;
	misc_var = false;
	if (nums != NULL) num = ecl_kw_iget_int(nums , index);
	if (strlen(well) > 0) {
	  /* See table 3.4 in the ECLIPSE file format reference manual. */
	  switch(kw[0]) {
	  case('A'):
	    break;
	  case('B'):
	    break;
	  case('C'):
	    /* Three level indexing: well -> string(cell_nr) -> variable */
	    if (!DUMMY_WELL(well) && num >= 0) {
	      char cell_str[16];
	      if (!hash_has_key(ecl_sum->well_completion_var_index , well)) 
		hash_insert_hash_owned_ref(ecl_sum->well_completion_var_index , well , hash_alloc(10) , hash_free__);
	      {
		hash_type * cell_hash = hash_get(ecl_sum->well_completion_var_index , well);
		sprintf(cell_str , "%d" , num);
		if (!hash_has_key(cell_hash , cell_str)) 
		  hash_insert_hash_owned_ref(cell_hash , cell_str , hash_alloc(10) , hash_free__);
		{
		  hash_type * var_hash = hash_get(cell_hash , cell_str);
		  hash_insert_int(var_hash , kw , index);
		}
	      }
	    }
	    break;
	  case('F'):
	    /* 
	       Field variable 
	    */
	    hash_insert_int(ecl_sum->field_var_index , kw , index);
	    ecl_sum->var_type[index] = ecl_sum_field_var;
	    break;
	  case('G'):
	    {
	      const char * group = well;
	      if (!DUMMY_WELL(well)) {
		if (!hash_has_key(ecl_sum->group_var_index , group)) 
		  hash_insert_hash_owned_ref(ecl_sum->group_var_index , group , hash_alloc(10) , hash_free__);
		{
		  hash_type * var_hash = hash_get(ecl_sum->group_var_index , group);
		  hash_insert_int(var_hash , kw , index);
		}
	      }
	      ecl_sum->var_type[index] = ecl_sum_group_var;
	    }
	    break;
	  case('L'):
	    {
	      switch(kw[1]) {
	      case('B'):
		break;
	      case('C'):
		break;
	      case('W'):
		break;
	      default:
		break;
	      }
	    }
	    break;
	  case('N'):
	    break;
	  case('R'):
	    if (kw[2] == 'F') {
	      /*
		 This is a region --> region flow situation 
	      */
	    } else {
	      if (!hash_has_key(ecl_sum->region_var_index , kw)) 
		hash_insert_int(ecl_sum->region_var_index , kw , index);
	      ecl_sum->var_type[index] = ecl_sum_region_var;
	    }
	    ecl_sum->num_regions = util_int_max(ecl_sum->num_regions , num);  
	    break;
	  case('S'):
	    /* Some special cases on Sxxxx not handled */
	    break;
	  case ('W'):
	    {
	      if (!DUMMY_WELL(well)) {
		set_add_key(well_set , well);
		if (!hash_has_key(ecl_sum->well_var_index , well)) {
		  hash_insert_hash_owned_ref(ecl_sum->well_var_index , well , hash_alloc(10) , hash_free__);
		}
		{
		  hash_type * var_hash = hash_get(ecl_sum->well_var_index , well);
		  hash_insert_int(var_hash , kw , index);
		}   
		ecl_sum->var_type[index] = ecl_sum_well_var;
	      }
	    }
	    break;
	  default:
	    misc_var = true;
	    break;
	  }
	} else 
	  misc_var = true;
	
	if (misc_var) {
	  hash_insert_int(ecl_sum->misc_var_index , kw , index);
	  ecl_sum->var_type[index] = ecl_sum_misc_var;
	}
	free(kw);
	free(well);
      }
      ecl_sum->Nwells    = set_get_size(well_set);
      ecl_sum->well_list = set_alloc_keylist(well_set);
      set_free(well_set);
    }
  }


  /*
    This is the only place the misc_var_index field
    is used - maybe a bit overkill?
  */

  if (hash_has_key(ecl_sum->misc_var_index , "DAY")) {
    int iblock;
    int day_index   = hash_get_int(ecl_sum->misc_var_index , "DAY");
    int month_index = hash_get_int(ecl_sum->misc_var_index , "MONTH");
    int year_index  = hash_get_int(ecl_sum->misc_var_index , "YEAR");
    
    for (iblock = 0; iblock < ecl_fstate_get_size(ecl_sum->data); iblock++) {
      ecl_block_type * block = ecl_fstate_iget_block(ecl_sum->data , iblock);
      ecl_block_set_sim_time_summary(block , /*time_index , years_index , */ day_index , month_index , year_index);
    }
  } 
}


ecl_sum_type * ecl_sum_fread_alloc(const char *header_file , int files , const char **data_files , bool report_mode , bool endian_convert) {
  ecl_sum_type *ecl_sum   = ecl_sum_alloc_empty(ECL_FMT_AUTO , endian_convert);
  ecl_sum_fread_alloc_data(ecl_sum , files , data_files , report_mode);
  ecl_sum_fread_header(ecl_sum , header_file);
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
    util_free_string_list(filelist , files);
  }
  
  ecl_fstate_save(ecl_sum->header);
  ecl_fstate_save(ecl_sum->data);
  free(summary_spec);
}



static void ecl_sum_assert_index(const ecl_sum_type * ecl_sum, int index) {
  if (index < 0 || index >= ecl_sum->params_size) {
    fprintf(stderr,"%s: index:%d invalid - aborting \n",__func__ , index);
    abort();
  }
}




/*
  time_index is zero based anyway ..... not nice 
*/
double ecl_sum_get_with_index(const ecl_sum_type *ecl_sum , int time_index , int sum_index) {
  /*
    fprintf(stderr,"%s: ** Warning ** incorrectly using ecl_fstate_iget_block() - should use ecl_fstate_get_block() \n",__func__);
  */
  if (ecl_sum->data == NULL) {
    fprintf(stderr,"%s: data not loaded - aborting \n",__func__);
    abort();
  }
  ecl_sum_assert_index(ecl_sum , sum_index);
  {
    ecl_block_type * block    = ecl_fstate_iget_block(ecl_sum->data , time_index);
    ecl_kw_type    * data_kw  = ecl_block_get_kw(block , "PARAMS");
    
    /* PARAMS underlying data type is float. */
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


static void ecl_sum_list_wells(const ecl_sum_type * ecl_sum) {
  int iw;
  char ** well_list = hash_alloc_keylist(ecl_sum->well_var_index);
  for (iw = 0; iw < hash_get_size(ecl_sum->well_var_index); iw++)
    printf("well[%03d] = %s \n",iw , well_list[iw]);
  hash_free_ext_keylist(ecl_sum->well_var_index , well_list);
}



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



int ecl_sum_get_field_var_index(const ecl_sum_type * ecl_sum , const char *var) {
  int index;

  if (hash_has_key(ecl_sum->field_var_index , var)) 
    index = hash_get_int(ecl_sum->field_var_index , var); 
  else {
    fprintf(stderr,"%s: summary object does not have field variable: %s  \n",__func__ , var);
    abort(); 
  }
  
  return index;
}


int ecl_sum_get_misc_var_index(const ecl_sum_type * ecl_sum , const char *var) {
  int index;

  if (hash_has_key(ecl_sum->misc_var_index , var)) 
    index = hash_get_int(ecl_sum->misc_var_index , var); 
  else {
    fprintf(stderr,"%s: summary object does not have misc variable: %s  \n",__func__ , var);
    abort(); 
  }
  
  return index;
}



/**
   region_nr: 0...num_regions-1 (C-based indexing)
*/

static void ecl_sum_assert_region_nr(const ecl_sum_type * ecl_sum , int region_nr) {
  if (region_nr < 0 || region_nr >= ecl_sum->num_regions) {
    fprintf(stderr,"%s: region_nr:%d not in valid range: [%d,%d) - aborting \n",__func__ , region_nr , 0 , ecl_sum->num_regions);
    abort();
  }
}




int ecl_sum_get_region_var_index(const ecl_sum_type * ecl_sum , int region_nr , const char *var) {
  int index = -1;
  

  ecl_sum_assert_region_nr(ecl_sum , region_nr);
  if (hash_has_key(ecl_sum->region_var_index , var)) 
    index = region_nr + hash_get_int(ecl_sum->region_var_index , var); 
  else {
    fprintf(stderr,"%s: summary object does not have region variable: %s  \n",__func__ , var);
    abort(); 
  }
  
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
    if (hash_has_key(var_hash , var))
      return true;
    else 
      return false;
  } else 
    return false;
}



bool ecl_sum_has_var(const ecl_sum_type * ecl_sum , const char *var) {
  if (hash_has_key(ecl_sum->misc_var_index , var))
    return true;
  else
    return false;
}


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


bool ecl_sum_get_report_mode(const ecl_sum_type * ecl_sum) {
  return ecl_fstate_get_report_mode(ecl_sum->data);
}


time_t ecl_sum_get_start_time(const ecl_sum_type * ecl_sum) {
  return ecl_sum->sim_start_time;
}


time_t ecl_sum_get_sim_time(const ecl_sum_type * ecl_sum , int index) {
  ecl_block_type * block = ecl_fstate_iget_block(ecl_sum->data , index);
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
  util_free_string_list(hvar_list , nvar);
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
  util_free_string_list(ecl_sum->well_list  , ecl_sum->Nwells);
  free(ecl_sum->var_type);

  if (ecl_sum->base_name != NULL)
    free(ecl_sum->base_name);
  free(ecl_sum);
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
    util_free_string_list(unformatted_list , unformatted_files);
    util_free_string_list(formatted_list   , formatted_files);
  }
  
  
  if (files > 0) 
    ecl_sum = ecl_sum_fread_alloc(header_file , files , (const char ** ) file_list , report_mode , endian_convert);
  else 
    ecl_sum = NULL;
  
  util_free_string_list(file_list , files); 
  if (header_file != NULL) free(header_file);
  free(base);

  return ecl_sum;
}
