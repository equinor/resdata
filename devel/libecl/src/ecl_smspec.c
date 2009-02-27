#include <string.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <math.h>
#include <ecl_fstate.h>
#include <ecl_util.h>
#include <hash.h>
#include <util.h>
#include <time.h>
#include <set.h>
#include <util.h>
#include <vector.h>
#include <int_vector.h>
#include <ecl_smspec.h>


/**
   This file implements the indexing into the ECLIPSE summary files. 
*/
   


#define DUMMY_WELL(well) (strcmp((well) , ":+:+:+:+") == 0)
#define ECL_SMSPEC_ID          806647


struct ecl_smspec_struct {
  int                __id;                      /* Funny integer used for for "safe" run-time casting. */
  ecl_fstate_type  * header;

  hash_type        * well_var_index;             /* Indexes for all well variables. */
  hash_type        * well_completion_var_index;  /* Indexes for completion indexes .*/
  hash_type        * group_var_index;            /* Indexes for group variables.*/
  hash_type        * field_var_index;
  hash_type        * region_var_index;           /* The stored index is an offset. */
  hash_type        * misc_var_index;
  hash_type        * unit_hash;
  hash_type        * block_var_index;

  ecl_smspec_var_type * var_type;
  int               report_offset;
  int               num_regions;
  int               fmt_mode;
  int               Nwells , param_offset;
  int               params_size;
  char            **well_list;
  char            * simulation_case;        /* This should be full path and basename - without any extension. */
  bool              endian_convert;
  
  time_t            sim_start_time;
  int               time_index;
  int               day_index;
  int               month_index;
  int               year_index;
};

/**
About indexing:
---------------

The ECLISPE summary files are organised (roughly) like this:

 1. A header-file called xxx.SMPSEC is written, which is common to
    every timestep.

 2. For each timestep the summary vector is written in the form of a
    vector 'PARAMS' of length N with floats. In the PARAMS vector all
    types of data are stacked togeheter, and one must use the header
    info in the SMSPEC file to disentangle the summary data.

Here I will try to describe how the header in SMSPEC is organised, and
how that support is imlemented here. The SMSPEC header is organized
around three character vectors, of length N. To find the position in
the PARAMS vector of a certain quantity, you must consult one, two or
all three of these vectors. The most important vecor - which must
always be consulted is the KEYWORDS vector, then it is the WGNAMES and
NUMS (integer) vectors whcih must be consulted for some variable
types.


Let us a consider a system consisting of:

  * Two wells: P1 and P2 - for each well we have variables WOPR, WWCT
    and WGOR.

  * Three regions: For each region we have variables RPR and RXX(??)

  * We have stored field properties FOPT and FWPT


KEYWORDS = ['TIME','FOPR','FPR','FWCT','WOPR','WOPR,'WWCT','WWCT]
       ....




*/




static ecl_smspec_type * ecl_smspec_alloc_empty(int fmt_mode , bool endian_convert , const char * path , const char * base_name) {
  ecl_smspec_type *ecl_smspec;
  ecl_smspec = util_malloc(sizeof *ecl_smspec , __func__);
  ecl_smspec->fmt_mode           	     = fmt_mode;
  ecl_smspec->endian_convert     	     = endian_convert;

  ecl_smspec->well_var_index     	     = hash_alloc();
  ecl_smspec->well_completion_var_index = hash_alloc();
  ecl_smspec->group_var_index    	     = hash_alloc();
  ecl_smspec->field_var_index    	     = hash_alloc();
  ecl_smspec->region_var_index   	     = hash_alloc();
  ecl_smspec->misc_var_index     	     = hash_alloc();
  ecl_smspec->unit_hash          	     = hash_alloc();
  ecl_smspec->block_var_index                = hash_alloc();

  ecl_smspec->var_type           	     = NULL;
  ecl_smspec->header             	     = NULL;
  ecl_smspec->well_list          	     = NULL;
  ecl_smspec->sim_start_time     	     = -1;
  ecl_smspec->report_offset             = 0;
  ecl_smspec->__id                      = ECL_SMSPEC_ID;
  ecl_smspec->simulation_case           = util_alloc_filename(path , base_name , NULL);

  ecl_smspec->time_index  = -1;
  ecl_smspec->day_index   = -1;
  ecl_smspec->year_index  = -1;
  ecl_smspec->month_index = -1;

  return ecl_smspec;
}






ecl_smspec_type * ecl_smspec_safe_cast(const void * __ecl_smspec) {
  ecl_smspec_type * ecl_smspec = (ecl_smspec_type *) __ecl_smspec;
  if (ecl_smspec->__id != ECL_SMSPEC_ID)
    util_abort("%s: runtime cast failed - aborting. \n",__func__);
  return ecl_smspec;
}




/* See table 3.4 in the ECLIPSE file format reference manual. */

ecl_smspec_var_type ecl_smspec_identify_var_type(const char * var) {
  ecl_smspec_var_type var_type = ecl_smspec_misc_var;
  switch(var[0]) {
  case('A'):
    var_type = ecl_smspec_aquifer_var;
    break;
  case('B'):
    var_type = ecl_smspec_block_var;
    break;
  case('C'):
    var_type = ecl_smspec_completion_var;
    break;
  case('F'):
    var_type = ecl_smspec_field_var;
    break;
  case('G'):
    var_type = ecl_smspec_group_var;
    break;
  case('L'):
    switch(var[1]) {
    case('B'):
      var_type = ecl_smspec_local_block_var;
      break;
    case('C'):
      var_type = ecl_smspec_local_completion_var;
      break;
    case('W'):
      var_type = ecl_smspec_local_well_var;
      break;
    default:
      util_abort("%s: not recognized: %s \n",__func__ , var);
    }
    break;
  case('N'):
    var_type = ecl_smspec_network_var;
    break;
  case('R'):
    if (var[2] == 'F')
      var_type  = ecl_smspec_region_2_region_var;
    else
      var_type  = ecl_smspec_region_var;
    break;
  case('S'):
    var_type = ecl_smspec_segment_var;
    break;
  case('W'):
    var_type = ecl_smspec_well_var;
    break;
  default:
    /*
      It is unfortunately impossible to recognize an error situation -
      the rest just goes in "other" variables.
    */
    var_type = ecl_smspec_misc_var;
  }
  return var_type;
}



static void ecl_smspec_fread_header(ecl_smspec_type * ecl_smspec, const char * header_file) {
  ecl_smspec->header = ecl_fstate_fread_alloc(1     , &header_file , ecl_summary_header_file , ecl_smspec->endian_convert , false);
  {
    int *date;
    ecl_block_type * block = ecl_fstate_iget_block(ecl_smspec->header , 0);
    ecl_kw_type *wells     = ecl_block_get_kw(block , "WGNAMES");
    ecl_kw_type *keywords  = ecl_block_get_kw(block , "KEYWORDS");
    ecl_kw_type *startdat  = ecl_block_get_kw(block , "STARTDAT");
    ecl_kw_type *units     = ecl_block_get_kw(block , "UNITS");
    ecl_kw_type *nums      = NULL;
    int index;
    ecl_smspec->num_regions     = 0;
    if (startdat == NULL) {
      fprintf(stderr,"%s: could not locate STARTDAT keyword in header - aborting \n",__func__);
      abort();
    }
    if (ecl_block_has_kw(block , "NUMS"))
      nums = ecl_block_get_kw(block , "NUMS");

    date = ecl_kw_get_int_ptr(startdat);
    ecl_smspec->sim_start_time = util_make_date(date[0] , date[1] , date[2]);
    {
      /*
	Fills a unit_hash: unit_hash["WOPR"] =	"Barrels/day"...
      */

      for (index=0; index < ecl_kw_get_size(keywords); index++) {
	char * kw = util_alloc_strip_copy( ecl_kw_iget_ptr(keywords , index));
	if (!hash_has_key(ecl_smspec->unit_hash , kw)) {
	  char * unit = util_alloc_strip_copy(ecl_kw_iget_ptr(units , index));
	  hash_insert_hash_owned_ref(ecl_smspec->unit_hash , kw , unit , free);
	}
	free(kw);
      }
    }
    
    ecl_smspec->params_size = ecl_kw_get_size(keywords);
    ecl_smspec->var_type    = util_malloc( ecl_smspec->params_size * sizeof * ecl_smspec->var_type , __func__);

    {
      set_type *well_set  = set_alloc_empty();
      int num = -1;
      for (index=0; index < ecl_kw_get_size(wells); index++) {
	char * well = util_alloc_strip_copy(ecl_kw_iget_ptr(wells    , index));
	char * kw   = util_alloc_strip_copy(ecl_kw_iget_ptr(keywords , index));
	if (nums != NULL) num = ecl_kw_iget_int(nums , index);
	ecl_smspec->var_type[index] = ecl_smspec_identify_var_type(kw);
	/* See table 3.4 in the ECLIPSE file format reference manual. */
	switch(ecl_smspec->var_type[index]) {
	case(ecl_smspec_completion_var):
	  /* Three level indexing: well -> string(cell_nr) -> variable */
	  if (!DUMMY_WELL(well)) {
	    /* Seems I have to accept nums < 0 to get shit through ??? */
	    char cell_str[16];
	    if (!hash_has_key(ecl_smspec->well_completion_var_index , well))
		hash_insert_hash_owned_ref(ecl_smspec->well_completion_var_index , well , hash_alloc() , hash_free__);
	    {
	      hash_type * cell_hash = hash_get(ecl_smspec->well_completion_var_index , well);
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
	case(ecl_smspec_field_var):
	  /*
	     Field variable
	  */
	  hash_insert_int(ecl_smspec->field_var_index , kw , index);
	  break;
	case(ecl_smspec_group_var):
	  {
	    const char * group = well;
	    if (!DUMMY_WELL(well)) {
	      if (!hash_has_key(ecl_smspec->group_var_index , group))
		hash_insert_hash_owned_ref(ecl_smspec->group_var_index , group , hash_alloc() , hash_free__);
	      {
		hash_type * var_hash = hash_get(ecl_smspec->group_var_index , group);
		hash_insert_int(var_hash , kw , index);
	      }
	    }
	  }
	  break;
	case(ecl_smspec_region_var):
	  if (!hash_has_key(ecl_smspec->region_var_index , kw))
	    hash_insert_int(ecl_smspec->region_var_index , kw , index);
	  ecl_smspec->num_regions = util_int_max(ecl_smspec->num_regions , num);
	  break;
	case (ecl_smspec_well_var):
	  if (!DUMMY_WELL(well)) {
	    /*
	       It seems we can have e.g. WOPR associated with the
	       dummy well, there is no limit to the stupidity of these
	       programmers.
	    */
	    set_add_key(well_set , well);
	    if (!hash_has_key(ecl_smspec->well_var_index , well))
	      hash_insert_hash_owned_ref(ecl_smspec->well_var_index , well , hash_alloc() , hash_free__);
	    {
	      hash_type * var_hash = hash_get(ecl_smspec->well_var_index , well);
	      hash_insert_int(var_hash , kw , index);
	    }
	  }
	  break;
	case(ecl_smspec_misc_var):
	  /*
	     Possibly we must have the possibility to alter
	     reclassify - so this last switch must be done
	     in two passes?
	  */
	  hash_insert_int(ecl_smspec->misc_var_index , kw , index);
	  break;
	case(ecl_smspec_block_var):
	  /* A block variable */
	  {
	    char * block_nr  = util_alloc_sprintf("%d" , num);
	    if (!hash_has_key(ecl_smspec->block_var_index , kw))
	      hash_insert_hash_owned_ref(ecl_smspec->block_var_index , kw , hash_alloc() , hash_free__);
	    {
	      hash_type * block_hash = hash_get(ecl_smspec->block_var_index , kw);
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
      ecl_smspec->Nwells    = set_get_size(well_set);
      ecl_smspec->well_list = set_alloc_keylist(well_set);
      set_free(well_set);
    }
  }
}



ecl_smspec_type * ecl_smspec_fread_alloc(const char *header_file , bool endian_convert) {
  ecl_smspec_type *ecl_smspec;
  
  {
    char * base_name , *path;
    util_alloc_file_components(header_file , &path , &base_name , NULL);
    ecl_smspec = ecl_smspec_alloc_empty(ECL_FMT_AUTO , endian_convert , path , base_name);
    util_safe_free(base_name);
    util_safe_free(path);
  }
  
  ecl_smspec_fread_header(ecl_smspec , header_file);
  if (hash_has_key(ecl_smspec->misc_var_index , "TIME")) {
    if (hash_has_key( ecl_smspec->misc_var_index , "TIME"))
      ecl_smspec->time_index = hash_get_int(ecl_smspec->misc_var_index , "TIME");

    if (hash_has_key(ecl_smspec->misc_var_index , "DAY")) {
      ecl_smspec->day_index   = hash_get_int(ecl_smspec->misc_var_index , "DAY");
      ecl_smspec->month_index = hash_get_int(ecl_smspec->misc_var_index , "MONTH");
      ecl_smspec->year_index  = hash_get_int(ecl_smspec->misc_var_index , "YEAR");
    } 
  } return ecl_smspec;
}


static void ecl_smspec_assert_index(const ecl_smspec_type * ecl_smspec, int index) {
  if (index < 0 || index >= ecl_smspec->params_size)
    util_abort("%s: index:%d invalid - aborting \n",__func__ , index);
}


ecl_smspec_var_type ecl_smspec_iget_var_type(const ecl_smspec_type * ecl_smspec , int sum_index) {
  ecl_smspec_assert_index(ecl_smspec , sum_index);
  return ecl_smspec->var_type[sum_index];
}


int ecl_smspec_get_num_groups(const ecl_smspec_type * ecl_smspec) {
  return hash_get_size(ecl_smspec->group_var_index);
}

char ** ecl_smspec_alloc_group_names(const ecl_smspec_type * ecl_smspec) {
  return hash_alloc_keylist(ecl_smspec->group_var_index);
}


int ecl_smspec_get_num_regions(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->num_regions;
}


int ecl_smspec_get_well_var_index(const ecl_smspec_type * ecl_smspec , const char * well , const char *var) {
  int index = -1; /* Compiler shut up. */

  if (hash_has_key(ecl_smspec->well_var_index , well)) {
    hash_type * var_hash = hash_get(ecl_smspec->well_var_index , well);
    if (hash_has_key(var_hash , var))
      index = hash_get_int(var_hash , var);
    else
      util_abort("%s: summary object does not have well/variable combination: %s/%s  \n",__func__ , well , var);
  } else
    util_abort("%s: summary object does not contain well: %s \n",__func__ , well);

  return index;
}




static int ecl_smspec_get_well_completion_var_index__(const ecl_smspec_type * ecl_smspec , const char * well , const char *var, int cell_nr) {
  int index = -1;

  if (hash_has_key(ecl_smspec->well_completion_var_index , well)) {
    char cell_str[16];
    hash_type * cell_hash = hash_get(ecl_smspec->well_completion_var_index , well);
    sprintf(cell_str , "%d" , cell_nr);

    if (hash_has_key(cell_hash , cell_str)) {
      hash_type * var_hash = hash_get(cell_hash , cell_str);
      if (hash_has_key(var_hash , var))
	index = hash_get_int(var_hash , var);
    }
  }
  return index;
}



int ecl_smspec_get_well_completion_var_index(const ecl_smspec_type * ecl_smspec , const char * well , const char *var, int cell_nr) {
  int index = ecl_smspec_get_well_completion_var_index__(ecl_smspec , well , var , cell_nr);
  if (index < 0) {
    fprintf(stderr,"%s: summary object does not have completion data for well:%s  variable:%s  cell_nr:%d \n",__func__ , well , var , cell_nr);
    abort();
  }
  return index;
}




int ecl_smspec_get_group_var_index(const ecl_smspec_type * ecl_smspec , const char * group , const char *var) {
  int index;

  if (hash_has_key(ecl_smspec->group_var_index , group)) {
    hash_type * var_hash = hash_get(ecl_smspec->group_var_index , group);
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

int ecl_smspec_get_block_var_index(const ecl_smspec_type * ecl_smspec , const char * block_var , const char *block_nr) {
  int index = -1;

  if (hash_has_key(ecl_smspec->block_var_index , block_var)) {
    hash_type * block_hash = hash_get(ecl_smspec->block_var_index , block_var);
    if (hash_has_key(block_hash , block_nr))
      index = hash_get_int(block_hash , block_nr);
    else
      util_abort("%s: summary object does not have %s:%s combination. \n",__func__ , block_var , block_nr);
  } else
    util_abort("%s: summary object does not contain group: block variable: \n",__func__ , block_var);

  return index;
}


int ecl_smspec_get_field_var_index(const ecl_smspec_type * ecl_smspec , const char *var) {
  int index = -1;

  if (hash_has_key(ecl_smspec->field_var_index , var))
    index = hash_get_int(ecl_smspec->field_var_index , var);
  else
    util_abort("%s: summary object does not have field variable: %s  \n",__func__ , var);

  return index;
}


int ecl_smspec_get_misc_var_index(const ecl_smspec_type * ecl_smspec , const char *var) {
  int index = -1;

  if (hash_has_key(ecl_smspec->misc_var_index , var))
    index = hash_get_int(ecl_smspec->misc_var_index , var);
  else
    util_abort("%s: summary object does not have misc variable: %s  \n",__func__ , var);

  return index;
}



/**
   region_nr: [1...num_regions] (NOT C-based indexing)
*/

static void ecl_smspec_assert_region_nr(const ecl_smspec_type * ecl_smspec , int region_nr) {
  if (region_nr <= 0 || region_nr > ecl_smspec->num_regions)
    util_abort("%s: region_nr:%d not in valid range: [1,%d] - aborting \n",__func__ , region_nr , ecl_smspec->num_regions);
}




int ecl_smspec_get_region_var_index(const ecl_smspec_type * ecl_smspec , int region_nr , const char *var) {
  int index = -1;

  ecl_smspec_assert_region_nr(ecl_smspec , region_nr);
  if (hash_has_key(ecl_smspec->region_var_index , var))
    index = region_nr + hash_get_int(ecl_smspec->region_var_index , var) - 1;
  else
    util_abort("%s: summary object does not have region variable: %s  \n",__func__ , var);

  return index;
}





bool ecl_smspec_has_field_var(const ecl_smspec_type * ecl_smspec , const char *var) {
  return hash_has_key(ecl_smspec->field_var_index , var);
}

bool ecl_smspec_has_misc_var(const ecl_smspec_type * ecl_smspec , const char *var) {
  return hash_has_key(ecl_smspec->misc_var_index , var);
}


bool ecl_smspec_has_region_var(const ecl_smspec_type * ecl_smspec , int region_nr , const char *var) {
  ecl_smspec_assert_region_nr(ecl_smspec , region_nr);
  return hash_has_key(ecl_smspec->region_var_index , var);
}


bool ecl_smspec_has_well_var(const ecl_smspec_type * ecl_smspec , const char * well , const char *var) {
  if (hash_has_key(ecl_smspec->well_var_index , well)) {
    hash_type * var_hash = hash_get(ecl_smspec->well_var_index , well);
    if (hash_has_key(var_hash , var))
      return true;
    else
      return false;
  } else
    return false;
}

bool ecl_smspec_has_group_var(const ecl_smspec_type * ecl_smspec , const char * group , const char *var) {
  if (hash_has_key(ecl_smspec->group_var_index , group)) {
    hash_type * var_hash = hash_get(ecl_smspec->group_var_index , group);
    return hash_has_key(var_hash , var);
  } else
    return false;
}


bool ecl_smspec_has_block_var(const ecl_smspec_type * ecl_smspec , const char * block_var , const char *block_nr) {
  if (hash_has_key(ecl_smspec->block_var_index , block_var)) {
    hash_type * var_hash = hash_get(ecl_smspec->block_var_index , block_nr);
    return hash_has_key(var_hash , block_nr);
  } else
    return false;
}



#define __ASSERT_ARGC(argc , target_argc , s , t) if (argc != target_argc) util_abort("%s: string:%s is not recognized for lookup of %s." , __func__ , s , t);
int ecl_smspec_get_general_var_index(const ecl_smspec_type * ecl_smspec , const char * lookup_kw) {
  int     index = -1;
  char ** argv;
  int     argc;
  ecl_smspec_var_type var_type;
  util_split_string(lookup_kw , ":" , &argc , &argv);
  var_type = ecl_smspec_identify_var_type(argv[0]);

  switch(var_type) {
  case(ecl_smspec_misc_var):
    index = ecl_smspec_get_misc_var_index(ecl_smspec , argv[0]);
    break;
  case(ecl_smspec_well_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Wells");
    index = ecl_smspec_get_well_var_index(ecl_smspec , argv[1] , argv[0]);
    break;
  case(ecl_smspec_region_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Regions");
    {
      int region_nr;
      if (!util_sscanf_int(argv[1] , &region_nr))
	util_abort("%s: failed to parse %s to an integer - aborting. \n",__func__ , argv[1]);
      index = ecl_smspec_get_region_var_index( ecl_smspec , region_nr , argv[0]);
    }
    break;
  case(ecl_smspec_field_var):
    index = ecl_smspec_get_field_var_index(ecl_smspec , argv[0]);
    break;
  case(ecl_smspec_group_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Groups");
    index = ecl_smspec_get_group_var_index(ecl_smspec , argv[1] , argv[0]);
    break;
  case(ecl_smspec_block_var):
    __ASSERT_ARGC(argc , 2 , lookup_kw , "Block");
    index = ecl_smspec_get_block_var_index(ecl_smspec , argv[0] , argv[1]);
    break;
  default:
    util_abort("%s: sorry looking up the type:%d / %s is not (yet) implemented.\n" , __func__ , var_type , lookup_kw);
  }
  util_free_stringlist(argv , argc);
  return index;
}
#undef __ASSERT_ARGC



bool ecl_smspec_has_general_var(const ecl_smspec_type * ecl_smspec , const char * lookup_kw) {
  bool    has_var = false;
  char ** argv;
  int     argc;
  ecl_smspec_var_type var_type;
  util_split_string(lookup_kw , ":" , &argc , &argv);
  var_type = ecl_smspec_identify_var_type(argv[0]);

  switch(var_type) {
  case(ecl_smspec_misc_var):
    has_var = ecl_smspec_has_misc_var(ecl_smspec , argv[0]);
    break;
  case(ecl_smspec_well_var):
    if (argc == 2)
      has_var = ecl_smspec_has_well_var(ecl_smspec , argv[1] , argv[0]);
    break;
  case(ecl_smspec_region_var):
    if (argc == 2)
    {
      int region_nr;
      if (!util_sscanf_int(argv[1] , &region_nr))
	util_abort("%s: failed to parse %s to an integer - aborting. \n",__func__ , argv[1]);
      has_var = ecl_smspec_has_region_var(ecl_smspec , region_nr , argv[0]);
    }
    break;
  case(ecl_smspec_field_var):
    has_var = ecl_smspec_has_field_var(ecl_smspec , argv[0]);
    break;
  case(ecl_smspec_group_var):
    if (argc == 2)
      has_var = ecl_smspec_has_group_var(ecl_smspec , argv[1] , argv[0]);
    break;
  case(ecl_smspec_block_var):
    if (argc == 2)
      has_var = ecl_smspec_has_block_var(ecl_smspec , argv[0] , argv[1]);
    break;
  default:
    util_abort("%s: sorry looking up the type:%d / %s is not (yet) implemented.\n" , __func__ , var_type , lookup_kw);
  }
  util_free_stringlist(argv , argc);
  return has_var;
}



/**
   The var variable can either be just "WOPR", "RPR" or the like. Or
   it can be a ":" variable like "WOPR:OP_3". If the unit_hash does
   not have the variable, in either format, NULL is returned.
*/

const char * ecl_smspec_get_unit(const ecl_smspec_type * ecl_smspec , const char *var) {
  const char * unit = NULL;
  if (hash_has_key(ecl_smspec->unit_hash , var))
    unit = hash_get(ecl_smspec->unit_hash , var);
  else {
    int var_length = strcspn(var , ":");
    if (var_length < strlen(var)) {
      const char * unit;
      char * tmp_var = util_alloc_substring_copy(var , var_length);
      if (hash_has_key(ecl_smspec->unit_hash , tmp_var))
	unit = hash_get(ecl_smspec->unit_hash , tmp_var);
      free(tmp_var);
    }
  }
  return unit;
}



int ecl_smspec_get_Nwells(const ecl_smspec_type *ecl_smspec) {
  return ecl_smspec->Nwells;
}


void ecl_smspec_copy_well_names(const ecl_smspec_type *ecl_smspec , char **well_list) {
  int iw;

  for (iw=0; iw < ecl_smspec->Nwells; iw++)
    strcpy(well_list[iw] , ecl_smspec->well_list[iw]);

}


char ** ecl_smspec_alloc_well_names_copy(const ecl_smspec_type *ecl_smspec) {
  char **well_list;
  int iw;
  well_list = calloc(ecl_smspec->Nwells , sizeof *well_list);
  for (iw = 0; iw < ecl_smspec->Nwells; iw++) {
    well_list[iw] = malloc(strlen(ecl_smspec->well_list[iw]) + 1);
    well_list[iw][0] = '\0';
  }
  ecl_smspec_copy_well_names(ecl_smspec , well_list);
  return well_list;
}


const char ** ecl_smspec_get_well_names_ref(const ecl_smspec_type * ecl_smspec) {
  return (const char **) ecl_smspec->well_list;
}



time_t ecl_smspec_get_start_time(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->sim_start_time;
}



const char * ecl_smspec_get_simulation_case(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->simulation_case;
}



void ecl_smspec_free(ecl_smspec_type *ecl_smspec) {
  ecl_fstate_free(ecl_smspec->header);
  hash_free(ecl_smspec->well_var_index);
  hash_free(ecl_smspec->well_completion_var_index);
  hash_free(ecl_smspec->group_var_index);
  hash_free(ecl_smspec->field_var_index);
  hash_free(ecl_smspec->region_var_index);
  hash_free(ecl_smspec->misc_var_index);
  hash_free(ecl_smspec->unit_hash);
  hash_free(ecl_smspec->block_var_index);
  util_free_stringlist(ecl_smspec->well_list  , ecl_smspec->Nwells);
  free(ecl_smspec->var_type);
  free(ecl_smspec->simulation_case);
  free(ecl_smspec);
}


void ecl_smspec_free__(void * __ecl_smspec) {
  ecl_smspec_type * ecl_smspec = ecl_smspec_safe_cast( __ecl_smspec);
  ecl_smspec_free( ecl_smspec );
}



void ecl_smspec_set_time_info( const ecl_smspec_type * smspec , const float * param_data , double * _sim_days , time_t * _sim_time) {
  double sim_days;
  time_t sim_time;

  if (smspec->time_index >= 0) {
    sim_days = param_data[smspec->time_index];
    sim_time = smspec->sim_start_time;
    util_inplace_forward_days( &sim_time , sim_days);
  } else {
    int sec  = 0;
    int min  = 0;
    int hour = 0;
    
    int day   = roundf(param_data[smspec->day_index]);
    int month = roundf(param_data[smspec->month_index]);
    int year  = roundf(param_data[smspec->year_index]);

    sim_time = util_make_datetime(sec , min , hour , day , month , year);
    sim_days = util_difftime_days( smspec->sim_start_time , sim_time);
  }
  *_sim_days = sim_days;
  *_sim_time= sim_time;
}



#undef ECL_SMSPEC_ID
