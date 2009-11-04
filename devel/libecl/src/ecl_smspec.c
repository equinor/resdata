#include <string.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <math.h>
#include <ecl_util.h>
#include <hash.h>
#include <util.h>
#include <time.h>
#include <set.h>
#include <util.h>
#include <vector.h>
#include <int_vector.h>
#include <ecl_smspec.h>
#include <ecl_file.h>
#include <stringlist.h>
#include <fnmatch.h>

/**
   This file implements the indexing into the ECLIPSE summary files. 
*/
   


#define DUMMY_WELL(well) (strcmp((well) , ":+:+:+:+") == 0)
#define ECL_SMSPEC_ID          806647


struct ecl_smspec_struct {
  UTIL_TYPE_ID_DECLARATION;
  
  hash_type        * well_var_index;             /* Indexes for all well variables: {well1: {var1: index1 , var2: index2} , well2: {var1: index1 , var2: index2}} */
  hash_type        * well_completion_var_index;  /* Indexes for completion indexes .*/
  hash_type        * group_var_index;            /* Indexes for group variables.*/
  hash_type        * field_var_index;
  hash_type        * region_var_index;           /* The stored index is an offset. */
  hash_type        * misc_var_index;
  hash_type        * unit_hash;
  hash_type        * block_var_index;
  hash_type        * gen_var_index;              /* This is "everything" - thing can either be found as gen_var("WWCT:OP_X") or as well_var("WWCT" , "OP_X") */
  hash_type        * special_types;

  ecl_smspec_var_type * var_type;                
  bool                * rate_variable; 
  int               grid_nx , grid_ny , grid_nz; /* Grid dimensions - in DIMENS[1,2,3] */
  int               num_regions;
  int               Nwells , param_offset;
  int               params_size;
  char            * simulation_case;        /* This should be full path and basename - without any extension. */
  
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



general_var:
------------
VAR_TYPE:(WELL_NAME|GROUP_NAME|NUMBER):NUMBER

Field var:         VAR_TYPE
Misc var:          VAR_TYPE  
Well var:          VAR_TYPE:WELL_NAME
Group var:         VAR_TYPE:GROUP_NAME
Block var:         VAR_TYPE:i,j,k  (where i,j,k is calculated form NUM)
Region var         VAR_TYPE:index  (where index is NOT from the nums vector, it it is just an offset).
Completion var:    VAR_TYPE:WELL_NAME:NUM
.... 


*/




static ecl_smspec_type * ecl_smspec_alloc_empty(const char * path , const char * base_name) {
  ecl_smspec_type *ecl_smspec;
  ecl_smspec = util_malloc(sizeof *ecl_smspec , __func__);
  UTIL_TYPE_ID_INIT(ecl_smspec , ECL_SMSPEC_ID);

  ecl_smspec->well_var_index     	     = hash_alloc();
  ecl_smspec->well_completion_var_index      = hash_alloc();
  ecl_smspec->group_var_index    	     = hash_alloc();
  ecl_smspec->field_var_index    	     = hash_alloc();
  ecl_smspec->region_var_index   	     = hash_alloc();
  ecl_smspec->misc_var_index     	     = hash_alloc();
  ecl_smspec->unit_hash          	     = hash_alloc();
  ecl_smspec->block_var_index                = hash_alloc();
  ecl_smspec->gen_var_index                  = hash_alloc();
  ecl_smspec->special_types                  = hash_alloc();
  /**
     The special_types hash table is used to associate keywords with
     special types, when the kewyord name is in conflict with the
     default vector naming convention.
     
     For instance the keyword 'NEWTON' starts with 'N' and is
     classified as a NETWORK type variable. However it should rather
     be classified as a MISC type variable. (What a fucking mess).
  */
  hash_insert_int(ecl_smspec->special_types , "NEWTON"    , ECL_SMSPEC_MISC_VAR );
  hash_insert_int(ecl_smspec->special_types , "NLINEARS"  , ECL_SMSPEC_MISC_VAR );
  hash_insert_int(ecl_smspec->special_types , "ELAPSED"   , ECL_SMSPEC_MISC_VAR );
  hash_insert_int(ecl_smspec->special_types , "MAXDPR"    , ECL_SMSPEC_MISC_VAR );
  hash_insert_int(ecl_smspec->special_types , "MAXDSO"    , ECL_SMSPEC_MISC_VAR );
  hash_insert_int(ecl_smspec->special_types , "MAXDSG"    , ECL_SMSPEC_MISC_VAR );
  hash_insert_int(ecl_smspec->special_types , "MAXDSW"    , ECL_SMSPEC_MISC_VAR );
  hash_insert_int(ecl_smspec->special_types , "STEPTYPE"  , ECL_SMSPEC_MISC_VAR );
  

  ecl_smspec->rate_variable                  = NULL;
  ecl_smspec->var_type           	     = NULL;
  ecl_smspec->sim_start_time     	     = -1;
  ecl_smspec->simulation_case                = util_alloc_filename(path , base_name , NULL);

  ecl_smspec->time_index  = -1;
  ecl_smspec->day_index   = -1;
  ecl_smspec->year_index  = -1;
  ecl_smspec->month_index = -1;

  return ecl_smspec;
}




UTIL_SAFE_CAST_FUNCTION( ecl_smspec , ECL_SMSPEC_ID )


/* 
   See table 3.4 in the ECLIPSE file format reference manual. 
   
   This function does not consider the variables internalized in the
   smspec instance, only the string 'var'.
*/

ecl_smspec_var_type ecl_smspec_identify_var_type(const ecl_smspec_type * smspec , const char * var) {
  ecl_smspec_var_type var_type = ECL_SMSPEC_MISC_VAR;
  
  if (hash_has_key( smspec->special_types , var ))
    return hash_get_int( smspec->special_types , var);
  else {
    switch(var[0]) {
    case('A'):
      var_type = ECL_SMSPEC_AQUIFER_VAR;
      break;
    case('B'):
      var_type = ECL_SMSPEC_BLOCK_VAR;
      break;
    case('C'):
      var_type = ECL_SMSPEC_COMPLETION_VAR;
      break;
    case('F'):
      var_type = ECL_SMSPEC_FIELD_VAR;
      break;
    case('G'):
      var_type = ECL_SMSPEC_GROUP_VAR;
      break;
    case('L'):
      switch(var[1]) {
      case('B'):
        var_type = ECL_SMSPEC_LOCAL_BLOCK_VAR;
        break;
      case('C'):
        var_type = ECL_SMSPEC_LOCAL_COMPLETION_VAR;
        break;
      case('W'):
        var_type = ECL_SMSPEC_LOCAL_WELL_VAR;
        break;
      default:
        util_abort("%s: not recognized: %s \n",__func__ , var);
      }
      break;
    case('N'):
      var_type = ECL_SMSPEC_NETWORK_VAR;
      break;
    case('R'):
      if (var[2] == 'F')
        var_type  = ECL_SMSPEC_REGION_2_REGION_VAR;
      else
        var_type  = ECL_SMSPEC_REGION_VAR;
      break;
    case('S'):
      var_type = ECL_SMSPEC_SEGMENT_VAR;
      break;
    case('W'):
      var_type = ECL_SMSPEC_WELL_VAR;
      break;
    default:
      /*
        It is unfortunately impossible to recognize an error situation -
        the rest just goes in "other" variables.
      */
      var_type = ECL_SMSPEC_MISC_VAR;
    }
    return var_type;
  }
}


static ecl_smspec_var_type ecl_smspec_split_general(const ecl_smspec_type * smspec , const char * gen_key , int * argc , char *** argv) {
  ecl_smspec_var_type var_type;
  util_split_string(gen_key , ":" , argc , argv);
  var_type = ecl_smspec_identify_var_type( smspec , (*argv)[0] );
  return var_type;
}




/**
   Takes a ecl_smspec_var_type variable as input, and return a string
   representation of this var_type. Suitable for debug messages +++
*/

const char * ecl_smspec_get_var_type_name( ecl_smspec_var_type var_type ) {
  switch(var_type) {
  case(ECL_SMSPEC_INVALID_VAR):
    return "INVALID_VAR";
    break;
  case(ECL_SMSPEC_AQUIFER_VAR):
    return "AQUIFER_VAR";
    break;
  case(ECL_SMSPEC_WELL_VAR):
    return "WELL_VAR";
    break;
  case(ECL_SMSPEC_REGION_VAR):
    return "REGION_VAR";
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    return "FIELD_VAR";
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    return "GROUP_VAR";
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    return "BLOCK_VAR";
    break;
  case(ECL_SMSPEC_COMPLETION_VAR):
    return "COMPLETION_VAR";
    break;
  case(ECL_SMSPEC_LOCAL_BLOCK_VAR):
    return "LOCAL_BLOCK_VAR";
    break;
  case(ECL_SMSPEC_LOCAL_COMPLETION_VAR):
    return "LOCAL_COMPLETION_VAR";
    break;
  case(ECL_SMSPEC_LOCAL_WELL_VAR):
    return "LOCAL_WELL_VAR";
    break;
  case(ECL_SMSPEC_NETWORK_VAR):
    return "NETWORK_VAR";
    break;
  case(ECL_SMSPEC_REGION_2_REGION_VAR):
    return "REGION_2_REGION_VAR";
    break;
  case(ECL_SMSPEC_SEGMENT_VAR):
    return "SEGMENT_VAR";
    break;
  case(ECL_SMSPEC_MISC_VAR):
    return "MISC_VAR";
    break;
  default:
    util_abort("%s: Unrecognized variable type:%d \n",__func__ , var_type);
    return NULL;
  }
}




/**
  Input i,j,k are assumed to be in the interval [1..nx] , [1..ny],
  [1..nz], return value is a global index which can be used in the
  xxx_block_xxx routines.
*/


static int ecl_smspec_get_global_grid_index(const ecl_smspec_type * smspec , int i , int j , int k) {
  return i + (j - 1) * smspec->grid_nx + (k - 1) * smspec->grid_nx * smspec->grid_ny;
}

/**
   Takes as input a global index [1...nx*ny*nz] and returns by
   reference i [1..nx] , j:[1..ny], k:[1..nz].
*/

static void ecl_smspec_get_ijk( const ecl_smspec_type * smspec , int global_index , int * i , int * j , int * k) {
  global_index--;
  *k = global_index / (smspec->grid_nx * smspec->grid_ny); global_index -= (*k) * (smspec->grid_nx * smspec->grid_ny);
  *j = global_index / smspec->grid_nx;                     global_index -= (*j) *  smspec->grid_nx;
  *i = global_index;

  /* Need offset one. */
  (*i) += 1;
  (*j) += 1;
  (*k) += 1;
}


static void ecl_smspec_set_rate_variable(ecl_smspec_type * smspec , const char * kw , int index);

static void ecl_smspec_fread_header(ecl_smspec_type * ecl_smspec, const char * header_file) {
  ecl_file_type * header = ecl_file_fread_alloc( header_file );
  {
    int *date;
    ecl_kw_type *wells     = ecl_file_iget_named_kw(header, "WGNAMES" , 0);
    ecl_kw_type *keywords  = ecl_file_iget_named_kw(header, "KEYWORDS", 0);
    ecl_kw_type *startdat  = ecl_file_iget_named_kw(header, "STARTDAT" , 0);
    ecl_kw_type *units     = ecl_file_iget_named_kw(header, "UNITS" , 0 );
    ecl_kw_type *dimens    = ecl_file_iget_named_kw(header, "DIMENS", 0);
    ecl_kw_type *nums      = NULL;
    int index;
    ecl_smspec->num_regions     = 0;
    if (startdat == NULL) 
      util_abort("%s: could not locate STARTDAT keyword in header - aborting \n",__func__);
    
    if (ecl_file_has_kw(header , "NUMS"))
      nums = ecl_file_iget_named_kw(header , "NUMS" , 0);
    
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
    
    ecl_smspec->grid_nx       = ecl_kw_iget_int(dimens , 1);
    ecl_smspec->grid_ny       = ecl_kw_iget_int(dimens , 2);
    ecl_smspec->grid_nz       = ecl_kw_iget_int(dimens , 3);
    ecl_smspec->params_size   = ecl_kw_get_size(keywords);
    ecl_smspec->var_type      = util_malloc( ecl_smspec->params_size * sizeof * ecl_smspec->var_type , __func__);
    ecl_smspec->rate_variable = util_malloc( ecl_smspec->params_size * sizeof * ecl_smspec->rate_variable , __func__);
    {
      int num = -1;
      for (index=0; index < ecl_kw_get_size(wells); index++) {
	char * well = util_alloc_strip_copy(ecl_kw_iget_ptr(wells    , index));
	char * kw   = util_alloc_strip_copy(ecl_kw_iget_ptr(keywords , index));
	if (nums != NULL) num = ecl_kw_iget_int(nums , index);
	ecl_smspec->var_type[index] = ecl_smspec_identify_var_type(ecl_smspec , kw);
	/* See table 3.4 in the ECLIPSE file format reference manual. */
	switch(ecl_smspec->var_type[index]) {
	case(ECL_SMSPEC_COMPLETION_VAR):
	  /* Three level indexing: variable -> well -> string(cell_nr)*/
	  if (!DUMMY_WELL(well)) {
	    /* Seems we have to accept nums < 0 to get shit through ??? */
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
                char * gen_key = util_alloc_sprintf("%s:%d:%s" , kw , num, well);
                hash_insert_int(var_hash , kw , index);
                hash_insert_int(ecl_smspec->gen_var_index , gen_key , index );
                free(gen_key);
	      }
	    }
	  } else
	    util_abort("%s: incorrectly formatted completion var in SMSPEC. num:%d kw:\"%s\"  well:\"%s\" \n",__func__ , num , kw , well);
	  break;
	case(ECL_SMSPEC_FIELD_VAR):
	  /*
	     Field variable
	  */
	  hash_insert_int(ecl_smspec->field_var_index , kw , index);
          hash_insert_int(ecl_smspec->gen_var_index , kw , index );
	  break;
	case(ECL_SMSPEC_GROUP_VAR):
	  {
	    const char * group = well;
	    if (!DUMMY_WELL(well)) {
	      if (!hash_has_key(ecl_smspec->group_var_index , group))
		hash_insert_hash_owned_ref(ecl_smspec->group_var_index , group , hash_alloc() , hash_free__);
	      {
		hash_type * var_hash = hash_get(ecl_smspec->group_var_index , group);
                char * gen_key = util_alloc_sprintf("%s:%s" , kw , well);
		hash_insert_int(var_hash , kw , index);
                hash_insert_int(ecl_smspec->gen_var_index , gen_key , index );
                free(gen_key);
	      }
	    }
	  }
	  break;
	case(ECL_SMSPEC_REGION_VAR):
	  if (!hash_has_key(ecl_smspec->region_var_index , kw))
	    hash_insert_int(ecl_smspec->region_var_index , kw , index);
	  ecl_smspec->num_regions = util_int_max(ecl_smspec->num_regions , num);
          {
            char * gen_key = util_alloc_sprintf("%s:%d" , kw , num);
            hash_insert_int( ecl_smspec->gen_var_index , gen_key , index);
            free( gen_key );
          }
	  break;
	case (ECL_SMSPEC_WELL_VAR):
	  if (!DUMMY_WELL(well)) {
	    /*
	       It seems we can have e.g. WOPR associated with a dummy
	       well, there is no limit to the stupidity of the
	       programmers at Schlum.
	    */
	    if (!hash_has_key(ecl_smspec->well_var_index , well))
	      hash_insert_hash_owned_ref(ecl_smspec->well_var_index , well , hash_alloc() , hash_free__);
	    {
	      hash_type * var_hash = hash_get(ecl_smspec->well_var_index , well);
              char * gen_key = util_alloc_sprintf("%s:%s" , kw , well);
              hash_insert_int(var_hash , kw , index);
              hash_insert_int(ecl_smspec->gen_var_index , gen_key , index );
              free(gen_key);
	    }
	  }
	  break;
	case(ECL_SMSPEC_MISC_VAR):
          /* Misc variable - i.e. date or CPU time ... */
	  hash_insert_int(ecl_smspec->misc_var_index , kw , index);
          hash_insert_int(ecl_smspec->gen_var_index , kw ,index);
	  break;
	case(ECL_SMSPEC_BLOCK_VAR):
	  /* A block variable */
	  {
	    char * block_nr  = util_alloc_sprintf("%d" , num);
	    if (!hash_has_key(ecl_smspec->block_var_index , kw))
	      hash_insert_hash_owned_ref(ecl_smspec->block_var_index , kw , hash_alloc() , hash_free__);
	    {
	      hash_type * block_hash = hash_get(ecl_smspec->block_var_index , kw);
              int i,j,k;
              char * gen_key;
              ecl_smspec_get_ijk(ecl_smspec , num , &i,&j,&k);
              /* Using (ONLY) the VAR:i,j,k form - could in addition also insert the VAR:num form?? */
              gen_key = util_alloc_sprintf("%s:%d,%d,%d" , kw , i,j,k);
              hash_insert_int(ecl_smspec->gen_var_index , gen_key , index);
	      hash_insert_int(block_hash , block_nr , index);
              free(gen_key);
	    }
	    free(block_nr);
	  }
	default:
	  /* Lots of legitimate alternatives which are not handled .. */
	  break;
	}
        ecl_smspec_set_rate_variable( ecl_smspec , kw ,index );
	free(kw);
	free(well);
      }
    }
  }
  ecl_file_free( header );
}



ecl_smspec_type * ecl_smspec_fread_alloc(const char *header_file) {
  ecl_smspec_type *ecl_smspec;
  
  {
    char * base_name , *path;
    util_alloc_file_components(header_file , &path , &base_name , NULL);
    ecl_smspec = ecl_smspec_alloc_empty(path , base_name);
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



/******************************************************************/
/* 
   For each type of summary data (according to the types in
   ecl_smcspec_var_type there are a set accessor functions:

    xx_get_xx: This function will take the apropriate input, and
       return a double value. The function will fail with util_abort()
       if the ecl_smspec object can not recognize the input. THis
       function is not here.

    xxx_has_xx: Ths will return true / false depending on whether the
       ecl_smspec object the variable we ask for.

    xxx_get_xxx_index: This function will rerturn an (internal)
       integer index of where the variable in question is stored, this
       index can then be subsequently used for faster lookup. If the
       variable can not be found, the function will return -1.

    In general the index function is the real function, the others are
    only wrappers around this. In addition there are specialized
    functions, like get_well_names() and so on.
*/


/******************************************************************/
/* Well variables */

int ecl_smspec_get_well_var_index(const ecl_smspec_type * ecl_smspec , const char * well , const char *var) {
  int index = -1; /* This is returned if we can not find it. */

  if (hash_has_key(ecl_smspec->well_var_index , well)) {
    hash_type * var_hash = hash_get(ecl_smspec->well_var_index , well);
    if (hash_has_key(var_hash , var))
      index = hash_get_int(var_hash , var);
  }
  return index;
}



bool ecl_smspec_has_well_var(const ecl_smspec_type * ecl_smspec , const char * well , const char *var) {
  int index = ecl_smspec_get_well_var_index(ecl_smspec , well ,var);
  if (index >= 0)
    return true;
  else
    return false;
}


/*****************************************************************/
/* Group variables */

int ecl_smspec_get_group_var_index(const ecl_smspec_type * ecl_smspec , const char * group , const char *var) {
  int index = -1;

  if (hash_has_key(ecl_smspec->group_var_index , group)) {
    hash_type * var_hash = hash_get(ecl_smspec->group_var_index , group);
    if (hash_has_key(var_hash , var))
      index = hash_get_int(var_hash , var);
  }
  return index;
}


bool ecl_smspec_has_group_var(const ecl_smspec_type * ecl_smspec , const char * group , const char *var) {
  if (ecl_smspec_get_group_var_index(ecl_smspec , group , var) >= 0)
    return true;
  else
    return false;
}


/*****************************************************************/
/* Field variables */
int ecl_smspec_get_field_var_index(const ecl_smspec_type * ecl_smspec , const char *var) {
  int index = -1;

  if (hash_has_key(ecl_smspec->field_var_index , var))
    index = hash_get_int(ecl_smspec->field_var_index , var);
  
  return index;
}



bool ecl_smspec_has_field_var(const ecl_smspec_type * ecl_smspec , const char *var) {
  return hash_has_key(ecl_smspec->field_var_index , var);
}

/*****************************************************************/
/* Block variables */

/**
   Observe that block_nr is represented as char literal,
   i.e. "2345". This is because it will be used as a hash key.
   
   This is the final low level function which actually consults the
   hash tables.
*/

static int ecl_smspec_get_block_var_index_string(const ecl_smspec_type * ecl_smspec , const char * block_var , const char * block_str) {
  int index = -1;
  if (hash_has_key(ecl_smspec->block_var_index , block_var)) {
    hash_type * block_hash = hash_get(ecl_smspec->block_var_index , block_var);
    if (hash_has_key(block_hash , block_str))
      index = hash_get_int(block_hash , block_str);
  }

  return index;
}


/*
  Here the block_str can either be "i,j,k" or "6362".
*/

static int ecl_smspec_get_block_var_index_gen_string(const ecl_smspec_type * ecl_smspec , const char * block_var , const char * block_str) {
  int i,j,k,global_index;
  
  if (sscanf(block_str , "%d,%d,%d" , &i,&j,&k) == 3) {
    /* We read three comma separated integers - this is ijk*/
    global_index = ecl_smspec_get_global_grid_index( ecl_smspec , i,j,k);
    return ecl_smspec_get_block_var_index( ecl_smspec , block_var , global_index );
  } else
    return ecl_smspec_get_block_var_index_string( ecl_smspec , block_var , block_str);

}


int ecl_smspec_get_block_var_index_ijk(const ecl_smspec_type * ecl_smspec , const char * block_var , int i , int j , int k) {
  return ecl_smspec_get_block_var_index( ecl_smspec , block_var , ecl_smspec_get_global_grid_index( ecl_smspec , i,j,k) );
}


int ecl_smspec_get_block_var_index(const ecl_smspec_type * ecl_smspec , const char * block_var , int block_nr) {
  int index;
  char * block_str = util_alloc_sprintf("%d" , block_nr);
  index = ecl_smspec_get_block_var_index_string(ecl_smspec , block_var , block_str);
  free( block_str );
  return index;
}



bool ecl_smspec_has_block_var(const ecl_smspec_type * ecl_smspec , const char * block_var , int block_nr) {
  if (ecl_smspec_get_block_var_index( ecl_smspec , block_var , block_nr) >= 0)
    return true;
  else
    return false;
}  


bool ecl_smspec_has_block_var_ijk(const ecl_smspec_type * ecl_smspec , const char * block_var , int i , int j , int k) {
  return ecl_smspec_has_block_var( ecl_smspec , block_var , ecl_smspec_get_global_grid_index( ecl_smspec , i,j,k) );
}



/*****************************************************************/
/* Region variables */
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
  
  return index;
}

bool ecl_smspec_has_region_var(const ecl_smspec_type * ecl_smspec , int region_nr , const char *var) {
  if (ecl_smspec_get_region_var_index( ecl_smspec , region_nr , var) >= 0)
    return true;
  else
    return false;
}

/*****************************************************************/
/* Misc variables */

int ecl_smspec_get_misc_var_index(const ecl_smspec_type * ecl_smspec , const char *var) {
  int index = -1;

  if (hash_has_key(ecl_smspec->misc_var_index , var))
    index = hash_get_int(ecl_smspec->misc_var_index , var);
  
  return index;
}


bool ecl_smspec_has_misc_var(const ecl_smspec_type * ecl_smspec , const char *var) {
  return hash_has_key(ecl_smspec->misc_var_index , var);
}

/*****************************************************************/
/* Well completion - not fully implemented ?? */

static int ecl_smspec_get_well_completion_var_index_string(const ecl_smspec_type * ecl_smspec , const char * well , const char *var, const char * cell_str) {
  int index = -1;
  if (hash_has_key(ecl_smspec->well_completion_var_index , well)) {
    hash_type * cell_hash = hash_get(ecl_smspec->well_completion_var_index , well);

    if (hash_has_key(cell_hash , cell_str)) {
      hash_type * var_hash = hash_get(cell_hash , cell_str);
      if (hash_has_key(var_hash , var))
	index = hash_get_int(var_hash , var);
    }
  }
  return index;
}


int ecl_smspec_get_well_completion_var_index(const ecl_smspec_type * ecl_smspec , const char * well , const char *var, int cell_nr) {
  int index;
  char * cell_str = util_alloc_sprintf("%d" , cell_nr);
  index = ecl_smspec_get_well_completion_var_index_string( ecl_smspec , well , var , cell_str);
  free(cell_str);
  return index;
}


bool  ecl_smspec_has_well_completion_var(const ecl_smspec_type * ecl_smspec , const char * well , const char *var, int cell_nr) {
  if (ecl_smspec_get_well_completion_var_index( ecl_smspec , well , var , cell_nr) >= 0)
    return true;
  else
    return false;
}


/*****************************************************************/
/* General variables ... */


/* There is a quite wide range of error which are just returned as "Not found" - but that is OK. */
/* Completions not supported yet. */
int ecl_smspec_get_general_var_index(const ecl_smspec_type * ecl_smspec , const char * lookup_kw) {
  int     index = -1;

  if (hash_has_key( ecl_smspec->gen_var_index , lookup_kw ))
    index = hash_get_int( ecl_smspec->gen_var_index , lookup_kw );

  return index;
  

  //ecl_smspec_var_type var_type;
  //var_type = ecl_smspec_split_general( lookup_kw , &argc , &argv );
  //
  //switch(var_type) {
  //case(ECL_SMSPEC_MISC_VAR):
  //  index = ecl_smspec_get_misc_var_index(ecl_smspec , argv[0]);
  //  break;
  //case(ECL_SMSPEC_WELL_VAR):
  //  if (argc == 2)
  //    index = ecl_smspec_get_well_var_index(ecl_smspec , argv[1] , argv[0]);
  //  break;
  //case(ECL_SMSPEC_REGION_VAR):
  //  if ( argc ==2 ) {
  //    int region_nr;
  //    if (util_sscanf_int(argv[1] , &region_nr))
  //      index = ecl_smspec_get_region_var_index( ecl_smspec , region_nr , argv[0]);
  //  }
  //  break;
  //case(ECL_SMSPEC_FIELD_VAR):
  //  if (argc == 1)
  //    index = ecl_smspec_get_field_var_index(ecl_smspec , argv[0]);
  //  break;
  //case(ECL_SMSPEC_GROUP_VAR):
  //  if (argc == 2)
  //    index = ecl_smspec_get_group_var_index(ecl_smspec , argv[1] , argv[0]);
  //  break;
  //case(ECL_SMSPEC_BLOCK_VAR):
  //  if (argc ==2 )
  //    index = ecl_smspec_get_block_var_index_gen_string(ecl_smspec , argv[0] , argv[1]);
  //  break;
  //default:
  //  util_abort("%s: sorry looking up the type:%d / %s is not (yet) implemented.\n" , __func__ , var_type , lookup_kw);
  //}
  //util_free_stringlist(argv , argc);
  ////if (index != hash_get_int( ecl_smspec->gen_var_index , lookup_kw))
  ////  util_abort("Internal error \n",__func__);

  return index;
}


bool ecl_smspec_has_general_var(const ecl_smspec_type * ecl_smspec , const char * lookup_kw) {
  if (ecl_smspec_get_general_var_index( ecl_smspec , lookup_kw ) >= 0)
    return true;
  else
    return false;
} 

/*****************************************************************/





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




time_t ecl_smspec_get_start_time(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->sim_start_time;
}



const char * ecl_smspec_get_simulation_case(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->simulation_case;
}



void ecl_smspec_free(ecl_smspec_type *ecl_smspec) {
  hash_free(ecl_smspec->well_var_index);
  hash_free(ecl_smspec->well_completion_var_index);
  hash_free(ecl_smspec->group_var_index);
  hash_free(ecl_smspec->field_var_index);
  hash_free(ecl_smspec->region_var_index);
  hash_free(ecl_smspec->misc_var_index);
  hash_free(ecl_smspec->unit_hash);
  hash_free(ecl_smspec->block_var_index);
  hash_free(ecl_smspec->gen_var_index);
  hash_free(ecl_smspec->special_types);
  free(ecl_smspec->var_type);
  free(ecl_smspec->rate_variable);
  free(ecl_smspec->simulation_case);
  free(ecl_smspec);
}


void ecl_smspec_free__(void * __ecl_smspec) {
  ecl_smspec_type * ecl_smspec = ecl_smspec_safe_cast( __ecl_smspec);
  ecl_smspec_free( ecl_smspec );
}



/*
  This function just 'exports functionality', the point is that the
  ecl_smspec object has all the information about indices, whereas the
  data object owns the final (pr. timestep) time information.
*/

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


/*****************************************************************/
/*
  This function checks in a predefined list whether a certain WGNAMES
  variable represents a total accumulated quantity. Only the last
  three characters in the variable is considered (i.e. the leading
  'W', 'G' or 'F' is discarded).
*/

bool ecl_smspec_var_is_total( const ecl_smspec_type * smspec , const char * var ) {
  /** 
      The list below is all the keyowrds with 'Total' in the
      information from the tables 2.7 - 2.11.  Have skipped some of
      the most exotic keywords (AND ALL THE HISTORICAL).
  */
  
  const char *total_vars[29] = {"OPT"  , "GPT"  , "WPT" , "OPTF" , "OPTS" , "OIT"  , "OVPT" , "OVIT" , "MWT" , "WIT" ,
                                "WVPT" , "WVIT" , "GMT"  , "GPTF" , "GIT"  , "SGT"  , "GST" , "FGT" , "GCT" , "GIMT" , 
                                "WGPT" , "WGIT" , "EGT"  , "EXGT" , "GVPT" , "GVIT" , "LPT" , "VPT" , "VIT" };
  int ivar;
  bool is_total = false;
  for (ivar = 0; ivar < 29; ivar++) {
    if (util_string_equal( total_vars[ivar] , &var[1])) {
      is_total = true;
      break;
    }
  }
  return is_total;
}





/**
   This function checks whether an input general key (i.e. FWPR or
   GGPT:NORTH) represents an accumulated total: 
   
     * If the variabel is one of [ECL_SMSPEC_WELL_VAR ,
       ECL_SMSPEC_GROUP_VAR , ECL_SMSPEC_FIELD_VAR] the
       ecl_smspec_var_is_total() is consulted. 

     * Otherwise the function will just return false.
*/


bool ecl_smspec_general_is_total( const ecl_smspec_type * smspec , const char * gen_key) {
  char ** argv;
  int     argc;
  bool is_total;
  ecl_smspec_var_type var_type;
  var_type = ecl_smspec_split_general( smspec , gen_key , &argc , &argv );
  if (var_type == ECL_SMSPEC_WELL_VAR || var_type == ECL_SMSPEC_GROUP_VAR || var_type == ECL_SMSPEC_FIELD_VAR) 
    is_total = ecl_smspec_var_is_total( smspec , argv[0] );
  else
    is_total = false;
  
  util_free_stringlist( argv , argc );
  return is_total;
}



        
        
static void ecl_smspec_set_rate_variable(ecl_smspec_type * smspec , const char * kw , int index) {
  const char *rate_vars[5] = {"OPR" , "GPR" , "WPR" , "GOR" , "WCT"};
  bool  is_rate            = false;
  int ivar;
  for (ivar = 0; ivar < 5; ivar++) {
    if (util_string_equal( rate_vars[ivar] , &kw[1])) {
      is_rate = true;
      break;
    }
  }
  smspec->rate_variable[ index ] = is_rate;
}



bool ecl_smspec_is_rate(const ecl_smspec_type * smspec , int kw_index) {
  return smspec->rate_variable[kw_index];
}





/*****************************************************************/
/**
   Allocates a stringlist instance with all the gen_key string
   matching the supplied pattern. I.e.

     ecl_smspec_alloc_matching_general_var_list( smspec , "WGOR:*");

   will give a list of WGOR for ALL the wells. The function is
   unfortunately not as useful as one might think because ECLIPSE is a
   bit stupid; it will for instance happily give ou the WOPR for a
   water injector or WWIR for an oil producer.
*/

stringlist_type * ecl_smspec_alloc_matching_general_var_list(const ecl_smspec_type * smspec , const char * pattern) {
  stringlist_type * keys = stringlist_alloc_new();
  hash_iter_type * iter = hash_iter_alloc( smspec->gen_var_index);
  while (!hash_iter_is_complete( iter )) {
    const char * key = hash_iter_get_next_key( iter );
    if (fnmatch( pattern , key , 0) == 0)
      stringlist_append_copy( keys , key );
  }
  hash_iter_free( iter );
  return keys;
}


/** 
    Returns a stringlist instance with all the (valid) well names. It
    is the responsability of the calling scope to free the stringlist
    with stringlist_free();
*/

stringlist_type * ecl_smspec_alloc_well_list( const ecl_smspec_type * smspec ) {
  return hash_alloc_stringlist( smspec->well_var_index );
}



/** 
    Returns a stringlist instance with all the well variables.  It is
    the responsability of the calling scope to free the stringlist
    with stringlist_free();
*/

stringlist_type * ecl_smspec_alloc_well_var_list( const ecl_smspec_type * smspec ) {
  hash_iter_type * well_iter = hash_iter_alloc( smspec->well_var_index );
  hash_type      * var_hash = hash_iter_get_next_value( well_iter );
  hash_iter_free( well_iter );
  return hash_alloc_stringlist( var_hash );
}




#undef ECL_SMSPEC_ID
