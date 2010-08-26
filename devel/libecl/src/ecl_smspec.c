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
#define SMSPEC_INDEX_ID        771109
#define NUMS_INVALID          -991199


/**
   This struct is contains meta-information about one element in the
   smspec file; the content is based on the smspec vectors WGNAMES,
   KEYWORDS, UNIT and NUMS. The index field of this struct points to
   where the actual data can be found in the PARAMS vector of the
   *.Snnnn / *.UNSMRY files; probably the most important field.
*/


typedef struct {
  ecl_smspec_var_type    var_type;           /* The variable type */
  char                 * wgname;             /* The value of the WGNAMES vector for this element. */
  char                 * keyword;            /* The value of the KEYWORDS vector for this elements. */
  char                 * unit;               /* The value of the UNITS vector for this elements. */
  int                    num;                /* The value of the NUMS vector for this elements - NB this will have the value NUMS_INVALID if the smspec file does not have a NUMS vector. */
  bool                   rate_variable;      /* Is this a rate variable (i.e. WOPR) or a state variable (i.e. BPR). Relevant when doing time interpolation. */
  bool                   total_variable;     /* Is this a total variable like WOPT? */
  int                    index;              /* The index of this variable (applies to all the vectors - in particular the PARAMS vectors of the summary files *.Snnnn / *.UNSMRY ). */
} smspec_index_type;



struct ecl_smspec_struct {
  UTIL_TYPE_ID_DECLARATION;
  
  hash_type          * well_var_index;             /* Indexes for all well variables: {well1: {var1: index1 , var2: index2} , well2: {var1: index1 , var2: index2}} */
  hash_type          * well_completion_var_index;  /* Indexes for completion indexes .*/
  hash_type          * group_var_index;            /* Indexes for group variables.*/
  hash_type          * field_var_index;               
  hash_type          * region_var_index;           /* The stored index is an offset. */
  hash_type          * misc_var_index;             /* Variables like 'TCPU' and 'NEWTON'. */
  hash_type          * block_var_index;            /* Block variables like BPR */ 
  hash_type          * gen_var_index;              /* This is "everything" - things can either be found as gen_var("WWCT:OP_X") or as well_var("WWCT" , "OP_X") */
  hash_type          * special_types;              /* Table of funky keywords which break ECLIPSE own default naming scheme. */

  smspec_index_type ** smspec_index_list;          /* This is the storage of smspec_index instances. */

  int               grid_nx , grid_ny , grid_nz;   /* Grid dimensions - in DIMENS[1,2,3] */
  int               num_regions;
  int               Nwells , param_offset;
  int               params_size;
  char            * simulation_path;               /* The path to the case - can be NULL for current directory. */
  char            * base_name;                     /* The basename of this simulation. */   
  char            * simulation_case;               /* This should be full path and basename - without any extension. */
  char            * key_join_string;               /* The string used to join keys when building gen_key keys - typically ":" - 
                                                      but arbitrary - NOT necessary to be able to invert the joining. */
  char            * header_file;                   /* FULL path to the currenbtly loaded header_file. */

  bool                formatted;                     /* Has this summary instance been loaded from a formatted (i.e. FSMSPEC file) or unformatted (i.e. SMSPEC) file. */
  time_t              sim_start_time;                /* When did the simulation start - worldtime. */

  int                 time_index;                    /* The fields time_index, day_index, month_index and year_index */
  int                 day_index;                     /* are used by the ecl_sum_data object to locate per. timestep */  
  int                 month_index;                   /* time information. */ 
  int                 year_index;

  stringlist_type   * restart_list;                  /* List of ECLBASE names of restart files this case has been restarted from (if any). */ 
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

/*****************************************************************/
/**
   Implementation of the small smspec_index_type data type.
*/


static void smspec_index_fprintf( const smspec_index_type * index , FILE * stream) {
  fprintf(stream , "var_type........: %d \n", index->var_type );
  fprintf(stream , "wgname..........: %s \n", index->wgname );
  fprintf(stream , "keyword.........: %s \n", index->keyword );
  fprintf(stream , "unit............: %s \n", index->unit );
  fprintf(stream , "index...........: %d \n", index->index );
  fprintf(stream , "num..  .........: %d \n", index->num);
  fprintf(stream , "rate_variable...: %d \n", index->rate_variable);
  fprintf(stream , "total_variable..: %d \n", index->total_variable);
}


static smspec_index_type * smspec_index_alloc_empty(ecl_smspec_var_type var_type, const char * keyword , const char * unit , int param_index) {
  smspec_index_type * index = util_malloc( sizeof * index , __func__);
  /** These can stay with values NULL / NUMS_INVALID for variables where those fields are not accessed. */
  index->wgname      = NULL;
  index->num         = NUMS_INVALID;


  /** All smspec_index instances should have valid values of these fields. */
  index->var_type    = var_type;
  index->unit        = util_alloc_string_copy( unit );
  index->keyword     = util_alloc_string_copy( keyword );
  index->index       = param_index;
  return index;
}


static void smspec_index_set_wgname( smspec_index_type * index , const char * wgname ) {
  if (DUMMY_WELL( wgname ))
    util_abort("%s: trying to set/derefernce WGNAME = %s which is invalid \n",__func__);
  
  index->wgname = util_realloc_string_copy(index->wgname , wgname );
}


static void smspec_index_set_num( smspec_index_type * index , int num) {
  if (num == NUMS_INVALID)
    util_abort("%s: explicitly trying to set nums == NUMS_INVALID - seems like a bug?!\n",__func__);
  
  index->num = num;
}


/**
   This function will allocate a smspec_index instance, and initialize
   all the elements. Observe that the function can return NULL, in the
   case we do not care to internalize the variable in question,
   i.e. if it is a well_rate from a dummy well or a variable type we
   do not support at all.

   This function initializes a valid smspec_index instance based on
   the supplied var_type, and the input. Observe that when a new
   variable type is supported, the big switch() statement must be
   updated in the functions ecl_smspec_install_gen_key() and
   ecl_smspec_fread_header() functions in addition. UGGGLY
*/


static smspec_index_type * smspec_index_alloc( ecl_smspec_var_type var_type , const char * wgname , const char * keyword , const char * unit , int num , int index) {
  smspec_index_type * smspec_index = NULL;
  
  switch (var_type) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    /* Completion variable : WGNAME & NUM */
    if (!DUMMY_WELL(wgname)) {
      smspec_index = smspec_index_alloc_empty( var_type , keyword , unit , index);
      smspec_index_set_wgname( smspec_index , wgname );
      smspec_index_set_num( smspec_index , num );
    }
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    /* Field variable : */
    smspec_index = smspec_index_alloc_empty( var_type ,  keyword , unit , index);
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    /* Group variable : WGNAME */
    if (!DUMMY_WELL(wgname)) {
      smspec_index = smspec_index_alloc_empty( var_type , keyword , unit , index);
      smspec_index_set_wgname( smspec_index , wgname );
    }
    break;
  case(ECL_SMSPEC_WELL_VAR):
    /* Well variable : WGNAME */
    if (!DUMMY_WELL(wgname)) {
      smspec_index = smspec_index_alloc_empty( var_type , keyword , unit , index);
      smspec_index_set_wgname( smspec_index , wgname );
    }
    break;
  case(ECL_SMSPEC_REGION_VAR):
    /* Region variable : NUM */
    smspec_index = smspec_index_alloc_empty( var_type , keyword , unit , index);
    smspec_index_set_num( smspec_index , num );
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    /* A block variable : NUM*/
    smspec_index = smspec_index_alloc_empty( var_type , keyword , unit , index);
    smspec_index_set_num( smspec_index , num );
    break;
  case(ECL_SMSPEC_MISC_VAR):
    /* Misc variable : */
    smspec_index = smspec_index_alloc_empty( var_type , keyword , unit , index);
    break;
  default:
    /* Lots of legitimate alternatives which are not handled .. */
    break;
  }
  

  if (smspec_index != NULL) {
    /* 
       Check if this is a rate variabel - that info is used when
       interpolating results to true_time between ministeps. 
    */
    {
      const char *rate_vars[5] = {"OPR" , "GPR" , "WPR" , "GOR" , "WCT"};
      bool  is_rate            = false;
      int ivar;
      for (ivar = 0; ivar < 5; ivar++) {
        if (util_string_equal( rate_vars[ivar] , &keyword[1])) {
          is_rate = true;
          break;
        }
      }
      smspec_index->rate_variable = is_rate;
    }

    
    
    /*
      This code checks in a predefined list whether a certain WGNAMES
      variable represents a total accumulated quantity. Only the last
      three characters in the variable is considered (i.e. the leading
      'W', 'G' or 'F' is discarded).
      
      The list below is all the keyowrds with 'Total' in the
      information from the tables 2.7 - 2.11 in the ECLIPSE
      documentation.  Have skipped some of the most exotic keywords
      (AND ALL THE HISTORICAL).
    */
    {
      bool is_total = false;
      if (var_type == ECL_SMSPEC_WELL_VAR || var_type == ECL_SMSPEC_GROUP_VAR || var_type == ECL_SMSPEC_FIELD_VAR) {
        const char *total_vars[29] = {"OPT"  , "GPT"  , "WPT" , "OPTF" , "OPTS" , "OIT"  , "OVPT" , "OVIT" , "MWT" , "WIT" ,
                                      "WVPT" , "WVIT" , "GMT"  , "GPTF" , "GIT"  , "SGT"  , "GST" , "FGT" , "GCT" , "GIMT" , 
                                      "WGPT" , "WGIT" , "EGT"  , "EXGT" , "GVPT" , "GVIT" , "LPT" , "VPT" , "VIT" };
        int ivar;
        bool is_total = false;
        for (ivar = 0; ivar < 29; ivar++) {
          if (util_string_equal( total_vars[ivar] , &keyword[1])) {
            is_total = true;
            break;
          }
        }
      }
      smspec_index->total_variable = is_total;
    }
  }
  return smspec_index;
}


static void smspec_index_free( smspec_index_type * index ) {
  if (index != NULL) {
    free( index->unit );
    free( index->keyword );
    util_safe_free( index->wgname );
    free( index );
  }
}


static inline int smspec_index_get_index( const smspec_index_type * smspec_index ) {
  return smspec_index->index;
}


/*****************************************************************/



static ecl_smspec_type * ecl_smspec_alloc_empty(const char * path , const char * base_name, const char * key_join_string) {
  ecl_smspec_type *ecl_smspec;
  ecl_smspec = util_malloc(sizeof *ecl_smspec , __func__);
  UTIL_TYPE_ID_INIT(ecl_smspec , ECL_SMSPEC_ID);

  ecl_smspec->well_var_index     	     = hash_alloc();
  ecl_smspec->well_completion_var_index      = hash_alloc();
  ecl_smspec->group_var_index    	     = hash_alloc();
  ecl_smspec->field_var_index    	     = hash_alloc();
  ecl_smspec->region_var_index   	     = hash_alloc();
  ecl_smspec->misc_var_index     	     = hash_alloc();
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
  

  ecl_smspec->sim_start_time     	     = -1;
  ecl_smspec->simulation_path                = util_alloc_string_copy( path );
  ecl_smspec->base_name                      = util_alloc_string_copy( base_name ); 
  ecl_smspec->simulation_case                = util_alloc_filename(path , base_name , NULL);
  ecl_smspec->key_join_string                = util_alloc_string_copy( key_join_string );
  ecl_smspec->header_file                    = NULL;

  ecl_smspec->time_index  = -1;
  ecl_smspec->day_index   = -1;
  ecl_smspec->year_index  = -1;
  ecl_smspec->month_index = -1;

  ecl_smspec->restart_list = stringlist_alloc_new();

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



char * ecl_smspec_alloc_gen_key( const ecl_smspec_type * smspec , const char * fmt , ...) {
  char * gen_key = NULL;
  va_list arg_list;
  va_start( arg_list , fmt );
  /* ... */
  va_end( arg_list );
  return gen_key;
}




static void ecl_smspec_install_gen_key( ecl_smspec_type * smspec , smspec_index_type * smspec_index ) {
  char * gen_key = NULL;

  switch( smspec_index->var_type) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    gen_key = util_alloc_sprintf("%s%s%s%s%d" , smspec_index->keyword , smspec->key_join_string , smspec_index->wgname , smspec->key_join_string , smspec_index->num );
    hash_insert_ref(smspec->gen_var_index , gen_key , smspec_index);
    /* Inserted with two keys - just like the block variables. */
    {
      int i,j,k;
      ecl_smspec_get_ijk(smspec , smspec_index->num , &i,&j,&k);
      gen_key = util_realloc_sprintf(gen_key , "%s%s%s%s%d,%d,%d" , smspec_index->keyword , smspec->key_join_string , smspec_index->wgname , smspec->key_join_string , i,j,k);
      hash_insert_ref(smspec->gen_var_index , gen_key , smspec_index);
    }
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    hash_insert_ref(smspec->gen_var_index , smspec_index->keyword , smspec_index);  
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    gen_key = util_alloc_sprintf("%s%s%s" , smspec_index->keyword , smspec->key_join_string , smspec_index->wgname );
    hash_insert_ref(smspec->gen_var_index , gen_key , smspec_index );
    break;
  case(ECL_SMSPEC_WELL_VAR):
    gen_key = util_alloc_sprintf("%s%s%s" , smspec_index->keyword , smspec->key_join_string , smspec_index->wgname );
    hash_insert_ref(smspec->gen_var_index , gen_key , smspec_index );
    break;
  case(ECL_SMSPEC_REGION_VAR):
    gen_key = util_alloc_sprintf("%s%s%d" , smspec_index->keyword , smspec->key_join_string , smspec_index->num );
    hash_insert_ref( smspec->gen_var_index , gen_key , smspec_index);
    break;
  case(ECL_SMSPEC_MISC_VAR):
    /* Misc variable - i.e. date or CPU time ... */
    hash_insert_ref(smspec->gen_var_index  , smspec_index->keyword , smspec_index );
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    /* A block variable */
    {

      /* Block variables are installed with two keys:

         VAR:NUM
         VAR:i,j,k
      */
      
      gen_key = util_alloc_sprintf("%s%s%d" , smspec_index->keyword , smspec->key_join_string , smspec_index->num );
      hash_insert_ref(smspec->gen_var_index , gen_key , smspec_index);
      
      {
        int i,j,k;
        ecl_smspec_get_ijk(smspec , smspec_index->num , &i,&j,&k);
        gen_key = util_realloc_sprintf(gen_key , "%s%s%d,%d,%d" , smspec_index->keyword , smspec->key_join_string , i,j,k);
        hash_insert_ref(smspec->gen_var_index , gen_key , smspec_index);
      }
    }
    break;
  default:
    util_abort("%s: internal error - should not be here? \n");
  }
  util_safe_free( gen_key );
}



bool ecl_smspec_needs_wgname( ecl_smspec_var_type var_type ) {
  switch( var_type ) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    return true;
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    return false;
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    return true;
    break;
  case(ECL_SMSPEC_WELL_VAR):
    return true;
    break;
  case(ECL_SMSPEC_REGION_VAR):
    return false;
    break;
  case(ECL_SMSPEC_MISC_VAR):
    return false;
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    return false;
  case(ECL_SMSPEC_AQUIFER_VAR):
    return false;
    break;
  default:
    util_exit("Sorry: support for variables of type:%s is not implemented in %s.\n",ecl_smspec_get_var_type_name( var_type ), __FILE__);
  }
  /* Really should not be here. */
  return false;
}



bool ecl_smspec_needs_num( ecl_smspec_var_type var_type ) {
  switch( var_type ) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    return true;
    break;
  case(ECL_SMSPEC_AQUIFER_VAR):
    return true;
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    return false;
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    return false;
    break;
  case(ECL_SMSPEC_WELL_VAR):
    return false;
    break;
  case(ECL_SMSPEC_REGION_VAR):
    return true;
    break;
  case(ECL_SMSPEC_MISC_VAR):
    return false;
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    return true;
    break;
  default:
    util_exit("Sorry: support for variables of type:%s is not implemented in %s.\n",ecl_smspec_get_var_type_name( var_type ), __FILE__);
  }
  return false;
}


/**
   This will iterate backwards through the RESTART header in the
   SMSPEC files to find names of the case(s) this case has been
   restarted from. 

   The case names are internalized in the restart_list field of the
   ecl_smspec instance. The actual loading of the restart summary data
   is subsequently handled by the ecl_sum_data function.
*/

static void ecl_smspec_load_restart( ecl_smspec_type * ecl_smspec , const ecl_file_type * header ) {
  if (ecl_file_has_kw( header , "RESTART" )) {
    const ecl_kw_type * restart_kw = ecl_file_iget_kw( header , 0 );
    char   tmp_base[73];   /* To accomodate a maximum of 9 items which consist of 8 characters each. */
    char * restart_base;
    int i;
    tmp_base[0] = '\0';
    for (i=0; i < ecl_kw_get_size( restart_kw ); i++) 
      strcat( tmp_base , ecl_kw_iget_ptr( restart_kw , i ));
    
    restart_base = util_alloc_strip_copy( tmp_base );
    if (strlen(restart_base)) {  /* We ignore the empty ones. */
      char * smspec_header = ecl_util_alloc_exfilename( ecl_smspec->simulation_path , restart_base , ECL_SUMMARY_HEADER_FILE , ecl_smspec->formatted , 0);

      if (!util_same_file(smspec_header , ecl_smspec->header_file)) {   /* Restart from the current case is ignored. */
        /* 
         Verify that this smspec_header is not already in the list of restart
         cases. Don't know if this is at all possible, but this test
         nevertheless prevents against a recursive death.
        */
        if (!stringlist_contains( ecl_smspec->restart_list , restart_base)) {
          stringlist_insert_copy( ecl_smspec->restart_list , 0 , restart_base );
          
          {
            if (smspec_header == NULL) 
              fprintf(stderr,"Warning - the file: %s refers to restart from case: %s - which was not found.... \n", ecl_smspec->simulation_case , restart_base);
            else {
              ecl_file_type * restart_header = ecl_file_fread_alloc( smspec_header );
              ecl_smspec_load_restart( ecl_smspec , restart_header);   /* Recursive call */ 
              ecl_file_free( restart_header );
            }
          }
        }
      }
      
      util_safe_free( smspec_header );
    }
    free( restart_base );
  }
}



static void ecl_smspec_fread_header(ecl_smspec_type * ecl_smspec, const char * header_file) {
  ecl_file_type * header = ecl_file_fread_alloc( header_file );
  {
    int *date;
    ecl_kw_type *wells     = ecl_file_iget_named_kw(header, "WGNAMES"  , 0);
    ecl_kw_type *keywords  = ecl_file_iget_named_kw(header, "KEYWORDS" , 0);
    ecl_kw_type *startdat  = ecl_file_iget_named_kw(header, "STARTDAT" , 0);
    ecl_kw_type *units     = ecl_file_iget_named_kw(header, "UNITS"    , 0 );
    ecl_kw_type *dimens    = ecl_file_iget_named_kw(header, "DIMENS"   , 0);
    ecl_kw_type *nums      = NULL;
    int index;
    ecl_smspec->num_regions     = 0;
    if (startdat == NULL) 
      util_abort("%s: could not locate STARTDAT keyword in header - aborting \n",__func__);
    
    if (ecl_file_has_kw(header , "NUMS"))
      nums = ecl_file_iget_named_kw(header , "NUMS" , 0);
    
    date = ecl_kw_get_int_ptr(startdat);
    ecl_smspec->sim_start_time = util_make_date(date[0] , date[1] , date[2]);
    ecl_smspec->grid_nx           = ecl_kw_iget_int(dimens , 1);
    ecl_smspec->grid_ny           = ecl_kw_iget_int(dimens , 2);
    ecl_smspec->grid_nz           = ecl_kw_iget_int(dimens , 3);
    ecl_smspec->params_size       = ecl_kw_get_size(keywords);
    ecl_smspec->smspec_index_list = util_malloc( ecl_smspec->params_size * sizeof * ecl_smspec->smspec_index_list  ,  __func__);
    ecl_util_get_file_type( header_file , &ecl_smspec->formatted , NULL );
    
    {
      for (index=0; index < ecl_kw_get_size(wells); index++) {
        int num                      = NUMS_INVALID;
	char * well                  = util_alloc_strip_copy(ecl_kw_iget_ptr(wells    , index));
	char * kw                    = util_alloc_strip_copy(ecl_kw_iget_ptr(keywords , index));
        char * unit                  = util_alloc_strip_copy(ecl_kw_iget_ptr(units    , index));
        ecl_smspec_var_type var_type = ecl_smspec_identify_var_type(ecl_smspec , kw);
        smspec_index_type * smspec_index;
	if (nums != NULL) num = ecl_kw_iget_int(nums , index);
        
        smspec_index = smspec_index_alloc( var_type , well , kw , unit , num , index );
        ecl_smspec->smspec_index_list[ index ] = smspec_index;
        if (smspec_index != NULL) {
          /** OK - we know this is valid shit. */
          
          ecl_smspec_install_gen_key( ecl_smspec , smspec_index );
          
          switch(var_type) {
          case(ECL_SMSPEC_COMPLETION_VAR):
            /* Three level indexing: variable -> well -> string(cell_nr)*/
	    if (!hash_has_key(ecl_smspec->well_completion_var_index , well))
              hash_insert_hash_owned_ref(ecl_smspec->well_completion_var_index , well , hash_alloc() , hash_free__);
	    {
	      hash_type * cell_hash = hash_get(ecl_smspec->well_completion_var_index , well);
              char cell_str[16];
	      sprintf(cell_str , "%d" , num);
	      if (!hash_has_key(cell_hash , cell_str))
		hash_insert_hash_owned_ref(cell_hash , cell_str , hash_alloc() , hash_free__);
	      {
		hash_type * var_hash = hash_get(cell_hash , cell_str);
                hash_insert_ref(var_hash , kw , smspec_index );
	      }
	    }
            break;
          case(ECL_SMSPEC_FIELD_VAR):
            /*
              Field variable
            */
            hash_insert_ref(ecl_smspec->gen_var_index , kw , smspec_index);
            hash_insert_int(ecl_smspec->field_var_index , kw , index);
            break;
          case(ECL_SMSPEC_GROUP_VAR):
            {
              const char * group = well;
              if (!hash_has_key(ecl_smspec->group_var_index , group))
                hash_insert_hash_owned_ref(ecl_smspec->group_var_index , group, hash_alloc() , hash_free__);
              {
                hash_type * var_hash = hash_get(ecl_smspec->group_var_index , group);
                hash_insert_ref(var_hash , kw , smspec_index );
              }
            }
            break;
          case(ECL_SMSPEC_REGION_VAR):
            if (!hash_has_key(ecl_smspec->region_var_index , kw))
              hash_insert_int(ecl_smspec->region_var_index , kw , index);
            ecl_smspec->num_regions = util_int_max(ecl_smspec->num_regions , num);
            break;
          case (ECL_SMSPEC_WELL_VAR):
            if (!hash_has_key(ecl_smspec->well_var_index , well))
              hash_insert_hash_owned_ref(ecl_smspec->well_var_index , well , hash_alloc() , hash_free__);
            {
              hash_type * var_hash = hash_get(ecl_smspec->well_var_index , well);
              hash_insert_ref(var_hash , kw , smspec_index );
            }
            break;
          case(ECL_SMSPEC_MISC_VAR):
            /* Misc variable - i.e. date or CPU time ... */
            hash_insert_ref(ecl_smspec->misc_var_index , kw , smspec_index );
            break;
          case(ECL_SMSPEC_BLOCK_VAR):
            /* A block variable */
            if (!hash_has_key(ecl_smspec->block_var_index , kw))
              hash_insert_hash_owned_ref(ecl_smspec->block_var_index , kw , hash_alloc() , hash_free__);
            {
              hash_type * block_hash = hash_get(ecl_smspec->block_var_index , kw);
              char * block_nr        = util_alloc_sprintf("%d" , num);
              hash_insert_ref(block_hash , block_nr , smspec_index);
              free(block_nr);
            }
            break;
          default:
            util_abort("%: Internal error - should never be here ?? \n",__func__);
            break;
          }
        }
        free( kw );
        free( well );
        free( unit );
      }
    }
  }
  ecl_smspec_load_restart( ecl_smspec , header );
  ecl_file_free( header );
  util_safe_free( ecl_smspec->header_file );
  ecl_smspec->header_file = util_alloc_realpath( header_file );
}



ecl_smspec_type * ecl_smspec_fread_alloc(const char *header_file, const char * key_join_string) {
  ecl_smspec_type *ecl_smspec;
  
  {
    char * base_name , *path;
    util_alloc_file_components(header_file , &path , &base_name , NULL);
    ecl_smspec = ecl_smspec_alloc_empty(path , base_name , key_join_string);
    util_safe_free(base_name);
    util_safe_free(path);
  }
  
  ecl_smspec_fread_header(ecl_smspec , header_file);
  
  if (hash_has_key(ecl_smspec->misc_var_index , "TIME")) {
    if (hash_has_key( ecl_smspec->misc_var_index , "TIME"))
      ecl_smspec->time_index = smspec_index_get_index( hash_get(ecl_smspec->misc_var_index , "TIME") );

    if (hash_has_key(ecl_smspec->misc_var_index , "DAY")) {
      ecl_smspec->day_index   = smspec_index_get_index( hash_get(ecl_smspec->misc_var_index , "DAY") );
      ecl_smspec->month_index = smspec_index_get_index( hash_get(ecl_smspec->misc_var_index , "MONTH") );
      ecl_smspec->year_index  = smspec_index_get_index( hash_get(ecl_smspec->misc_var_index , "YEAR") );
    } 
  } return ecl_smspec;
}


static void ecl_smspec_assert_index(const ecl_smspec_type * ecl_smspec, int index) {
  if (index < 0 || index >= ecl_smspec->params_size)
    util_abort("%s: index:%d invalid - aborting \n",__func__ , index);
}


static const smspec_index_type * ecl_smspec_iget_index( const ecl_smspec_type * ecl_smspec , int index) {
  ecl_smspec_assert_index(ecl_smspec , index);
  {
    const smspec_index_type * smspec_index = ecl_smspec->smspec_index_list[ index ];
    if (smspec_index != NULL)
      return smspec_index;
    else {
      util_abort("%s: asked for internal index of element:%d - that element is not internalized \n",__func__ , index);
      return NULL;
    }
  }
}
ecl_smspec_var_type ecl_smspec_iget_var_type(const ecl_smspec_type * ecl_smspec , int sum_index) {
  const smspec_index_type * smspec_index = ecl_smspec_iget_index( ecl_smspec , sum_index );
  return smspec_index->var_type;
}


bool ecl_smspec_is_rate(const ecl_smspec_type * smspec , int kw_index) {
  const smspec_index_type * smspec_index = ecl_smspec_iget_index( smspec , kw_index );
  return smspec_index->rate_variable;
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
      index = smspec_index_get_index( hash_get(var_hash , var) );
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
      index = smspec_index_get_index( hash_get(var_hash , var) );
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
    index = smspec_index_get_index( hash_get(ecl_smspec->field_var_index , var) );
  
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
      index = smspec_index_get_index( hash_get(block_hash , block_str) );
  }

  return index;
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
    index = region_nr -1 + smspec_index_get_index(hash_get( ecl_smspec->region_var_index , var) );
  
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
    index = smspec_index_get_index( hash_get(ecl_smspec->misc_var_index , var) );
  
  return index;
}


bool ecl_smspec_has_misc_var(const ecl_smspec_type * ecl_smspec , const char *var) {
  return hash_has_key(ecl_smspec->misc_var_index , var);
}

/*****************************************************************/
/* Well completion - not fully implemented ?? */


int ecl_smspec_get_well_completion_var_index(const ecl_smspec_type * ecl_smspec , const char * well , const char *var, int cell_nr) {
  int index = -1;
  char * cell_str = util_alloc_sprintf("%d" , cell_nr);
  if (hash_has_key(ecl_smspec->well_completion_var_index , well)) {
    hash_type * cell_hash = hash_get(ecl_smspec->well_completion_var_index , well);
    
    if (hash_has_key(cell_hash , cell_str)) {
      hash_type * var_hash = hash_get(cell_hash , cell_str);
      if (hash_has_key(var_hash , var))
	index = smspec_index_get_index( hash_get( var_hash , var) );
    }
  }
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


/* There is a quite wide range of error which are just returned as
   "Not found" (i.e. -1) - but that is OK. */
/* Completions not supported yet. */

int ecl_smspec_get_general_var_index(const ecl_smspec_type * ecl_smspec , const char * lookup_kw) {
  int     index = -1;
  
  if (hash_has_key( ecl_smspec->gen_var_index , lookup_kw )) {
    const smspec_index_type * smspec_index = hash_get( ecl_smspec->gen_var_index , lookup_kw );
    index = smspec_index_get_index( smspec_index );
  }
  
  return index;
}


bool ecl_smspec_has_general_var(const ecl_smspec_type * ecl_smspec , const char * lookup_kw) {
  return hash_has_key( ecl_smspec->gen_var_index , lookup_kw );
} 


/** DIES if the lookup_kw is not present. */
const char * ecl_smspec_get_general_var_unit( const ecl_smspec_type * ecl_smspec , const char * lookup_kw) {
  const smspec_index_type * smspec_index = hash_get( ecl_smspec->gen_var_index , lookup_kw );
  return smspec_index->unit;
}


/*****************************************************************/
/* 
   Pure indexed lookup - these functions can be used after one of the
   ecl_smspec_get_xxx_index() functions has been used first.
*/

const char * ecl_smspec_iget_unit( const ecl_smspec_type * smspec , int index ) {
  const smspec_index_type * smspec_index = ecl_smspec_iget_index( smspec , index );
  return smspec_index->unit;
}



int ecl_smspec_iget_num( const ecl_smspec_type * smspec , int index ) {
  const smspec_index_type * smspec_index = ecl_smspec_iget_index( smspec , index );
  return smspec_index->num;
}

const char * ecl_smspec_iget_wgname( const ecl_smspec_type * smspec , int index ) {
  const smspec_index_type * smspec_index = ecl_smspec_iget_index( smspec , index );
  return smspec_index->wgname;
}

const char * ecl_smspec_iget_keyword( const ecl_smspec_type * smspec , int index ) {
  const smspec_index_type * smspec_index = ecl_smspec_iget_index( smspec , index );
  return smspec_index->keyword;
}


/*****************************************************************/

time_t ecl_smspec_get_start_time(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->sim_start_time;
}

bool ecl_smspec_get_formatted( const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->formatted;
}

const char * ecl_smspec_get_header_file( const ecl_smspec_type * ecl_smspec ) {
  return ecl_smspec->header_file;
}

const char * ecl_smspec_get_simulation_case(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->simulation_case;
}

const char * ecl_smspec_get_simulation_path(const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->simulation_path;
}

const char * ecl_smspec_get_base_name( const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->base_name;
}

const stringlist_type * ecl_smspec_get_restart_list( const ecl_smspec_type * ecl_smspec) {
  return ecl_smspec->restart_list;
}


void ecl_smspec_free(ecl_smspec_type *ecl_smspec) {
  hash_free(ecl_smspec->well_var_index);
  hash_free(ecl_smspec->well_completion_var_index);
  hash_free(ecl_smspec->group_var_index);
  hash_free(ecl_smspec->field_var_index);
  hash_free(ecl_smspec->region_var_index);
  hash_free(ecl_smspec->misc_var_index);
  hash_free(ecl_smspec->block_var_index);
  hash_free(ecl_smspec->gen_var_index);
  hash_free(ecl_smspec->special_types);
  util_safe_free( ecl_smspec->header_file );
  free(ecl_smspec->simulation_case);
  free(ecl_smspec->simulation_path);
  free(ecl_smspec->base_name);
  free(ecl_smspec->key_join_string);
  {
    int index;
    for (index = 0; index < ecl_smspec->params_size; index++) 
      smspec_index_free( ecl_smspec->smspec_index_list[ index ] );
    free( ecl_smspec->smspec_index_list );
  }
  stringlist_free( ecl_smspec->restart_list );
  free( ecl_smspec );
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
  } else if (smspec->day_index > 0) {
    int sec  = 0;
    int min  = 0;
    int hour = 0;
    
    int day   = roundf(param_data[smspec->day_index]);
    int month = roundf(param_data[smspec->month_index]);
    int year  = roundf(param_data[smspec->year_index]);
    
    sim_time = util_make_datetime(sec , min , hour , day , month , year);
    sim_days = util_difftime_days( smspec->sim_start_time , sim_time);
  } else {
    /* Unusable configuration */
    util_abort("%s: Sorry the SMSPEC file seems to lack all time information, need either TIME, or DAY/MONTH/YEAR information. Can not proceee.",__func__);
    
    sim_time = -1;
    sim_days = -1;
  }

  *_sim_days = sim_days;
  *_sim_time= sim_time;
}


/**
   This function checks whether an input general key (i.e. FWPR or
   GGPT:NORTH) represents an accumulated total. If the variable is not
   internalized the function will fail hard.
*/


bool ecl_smspec_general_is_total( const ecl_smspec_type * smspec , const char * gen_key) {
  const  smspec_index_type * smspec_index = hash_get( smspec->gen_var_index , gen_key );
  return smspec_index->total_variable;
}




/*****************************************************************/


/**
   Fills a stringlist instance with all the gen_key string matching
   the supplied pattern. I.e.

     ecl_smspec_alloc_matching_general_var_list( smspec , "WGOR:*");

   will give a list of WGOR for ALL the wells. The function is
   unfortunately not as useful as one might think because ECLIPSE is a
   bit quite stupid; it will for instance happily give ou the WOPR for
   a water injector or WWIR for an oil producer.
*/

void ecl_smspec_select_matching_general_var_list( const ecl_smspec_type * smspec , const char * pattern , stringlist_type * keys) {
  hash_iter_type * iter = hash_iter_alloc( smspec->gen_var_index);
  stringlist_clear( keys );
  while (!hash_iter_is_complete( iter )) {
    const char * key = hash_iter_get_next_key( iter );
    if (fnmatch( pattern , key , 0) == 0)
      stringlist_append_copy( keys , key );
  }
  hash_iter_free( iter );
}


/**
   Allocates a new stringlist and initializes it with the
   ecl_smspec_select_matching_general_var_list() function.
*/

stringlist_type * ecl_smspec_alloc_matching_general_var_list(const ecl_smspec_type * smspec , const char * pattern) {
  stringlist_type * keys = stringlist_alloc_new();
  ecl_smspec_select_matching_general_var_list( smspec , pattern , keys );
  return keys;
}



const char * ecl_smspec_get_join_string( const ecl_smspec_type * smspec) {
  return smspec->key_join_string;
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



int ecl_smspec_get_param_size( const ecl_smspec_type * smspec ) {
  return smspec->params_size;
}


#undef ECL_SMSPEC_ID
