/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'smspec_node.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#include <hash.h>
#include <util.h>
#include <set.h>
#include <vector.h>
#include <int_vector.h>
#include <stringlist.h>
#include <type_macros.h>

#include <ecl_kw.h>
#include <ecl_util.h>
#include <ecl_smspec.h>
#include <smspec_node.h>
#include <ecl_file.h>
#include <ecl_kw_magic.h>


#define DUMMY_WELL(well) (strcmp((well) , ":+:+:+:+") == 0)


/**
   This struct contains meta-information about one element in the smspec
   file; the content is based on the smspec vectors WGNAMES, KEYWORDS, UNIT
   and NUMS. The index field of this struct points to where the actual data
   can be found in the PARAMS vector of the *.Snnnn / *.UNSMRY files;
   probably the most important field.  
*/

#define SMSPEC_TYPE_ID 61550451


struct smspec_node_struct {
  UTIL_TYPE_ID_DECLARATION;
  char                 * gen_key;            /* The composite key, i.e. WWCT:OP3 for this element. */ 
  ecl_smspec_var_type    var_type;           /* The variable type */
  char                 * wgname;             /* The value of the WGNAMES vector for this element. */
  char                 * keyword;            /* The value of the KEYWORDS vector for this elements. */
  char                 * unit;               /* The value of the UNITS vector for this elements. */
  int                    num;                /* The value of the NUMS vector for this elements - NB this will have the value SMSPEC_NUMS_INVALID if the smspec file does not have a NUMS vector. */
  char                 * lgr_name;           /* The lgr name of the current variable - will be NULL for non-lgr variables. */
  int                  * lgr_ijk;            /* The (i,j,k) coordinate, in the local grid, if this is a LGR variable. WIll be NULL for no-lgr variables. */
  bool                   rate_variable;      /* Is this a rate variable (i.e. WOPR) or a state variable (i.e. BPR). Relevant when doing time interpolation. */
  bool                   total_variable;     /* Is this a total variable like WOPT? */
  int                    params_index;       /* The index of this variable (applies to all the vectors - in particular the PARAMS vectors of the summary files *.Snnnn / *.UNSMRY ). */
  float                  default_value;      /* Default value for this variable. */
};


/*****************************************************************/
/*
  The key formats for the combined keys like e.g. 'WWCT:OP_5' should
  have the keyword, i.e. 'WWCT', as the first part of the string. That
  guarantees that the function ecl_smspec_identify_var_type() can take
  both a pure ECLIPSE variable name, like .e.g 'WWCT' and also an
  ecl_sum combined key like 'WWCT:OPX' as input.
*/

#define ECL_SUM_KEYFMT_BLOCK_IJK              "%s%s%d,%d,%d"
#define ECL_SUM_KEYFMT_BLOCK_NUM              "%s%s%d"
#define ECL_SUM_KEYFMT_LOCAL_BLOCK            "%s%s%s%s%d,%d,%d"
#define ECL_SUM_KEYFMT_COMPLETION_IJK         "%s%s%s%s%d,%d,%d" 
#define ECL_SUM_COMPLETION_NUM                "%s%s%s%s%d"
#define ECL_SUM_KEYFMT_COMPLETION_NUM         "%s%s%s%s%d"
#define ECL_SUM_KEYFMT_LOCAL_COMPLETION       "%s%s%s%s%s%s%d,%d,%d"
#define ECL_SUM_KEYFMT_GROUP                  "%s%s%s"
#define ECL_SUM_KEYFMT_WELL                   "%s%s%s"
#define ECL_SUM_KEYFMT_REGION                 "%s%s%d"
#define ECL_SUM_KEYFMT_SEGMENT                "%s%s%s%s%d"
#define ECL_SUM_KEYFMT_LOCAL_WELL             "%s%s%s%s%s"

UTIL_SAFE_CAST_FUNCTION( smspec_node , SMSPEC_TYPE_ID )


char * smspec_alloc_block_ijk_key( const char * join_string , const char * keyword , int i , int j , int k) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_BLOCK_IJK , 
                            keyword,
                            join_string , 
                            i,j,k);
}


char * smspec_alloc_completion_ijk_key( const char * join_string , const char * keyword, const char * wgname , int i , int j , int k) {
  return util_alloc_sprintf( ECL_SUM_KEYFMT_COMPLETION_IJK , 
                             keyword , 
                             join_string , 
                             wgname , 
                             join_string , 
                             i , j , k );
}

char * smspec_alloc_completion_num_key( const char * join_string , const char * keyword, const char * wgname , int num) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_COMPLETION_NUM,
                            keyword , 
                            join_string , 
                            wgname , 
                            join_string , 
                            num );
}

char * smspec_alloc_group_key( const char * join_string , const char * keyword , const char * wgname) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_GROUP,
                            keyword , 
                            join_string , 
                            wgname );
}


char * smspec_alloc_well_key( const char * join_string , const char * keyword , const char * wgname) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_WELL,
                            keyword , 
                            join_string , 
                            wgname );
}


char * smspec_alloc_region_key( const char * join_string , const char * keyword , int num) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_REGION , 
                            keyword , 
                            join_string , 
                            num );
}

char * smspec_alloc_segment_key( const char * join_string , const char * keyword , const char * wgname , int num) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_SEGMENT , 
                            keyword , 
                            join_string , 
                            wgname , 
                            join_string , 
                            num );
}


char * smspec_alloc_block_num_key( const char * join_string , const char * keyword , int num) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_BLOCK_NUM,
                            keyword , 
                            join_string , 
                            num );
}


char * smspec_alloc_local_well_key( const char * join_string , const char * keyword , const char * lgr_name , const char * wgname) {
  return util_alloc_sprintf( ECL_SUM_KEYFMT_LOCAL_WELL , 
                             keyword , 
                             join_string , 
                             lgr_name , 
                             join_string , 
                             wgname);
}


char * smspec_alloc_local_block_key( const char * join_string , const char * keyword , const char * lgr_name , int i , int j , int k) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_LOCAL_BLOCK , 
                            keyword , 
                            join_string , 
                            lgr_name , 
                            join_string , 
                            i,j,k);
}

char * smspec_alloc_local_completion_key( const char * join_string, const char * keyword , const char * lgr_name , const char * wgname , int i , int j , int k) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_LOCAL_COMPLETION , 
                            keyword  , 
                            join_string , 
                            lgr_name , 
                            join_string ,
                            wgname , 
                            join_string ,
                            i,j,k);
}

/*****************************************************************/


static void smspec_node_set_flags( smspec_node_type * smspec_node) {
  /* 
     Check if this is a rate variabel - that info is used when
     interpolating results to true_time between ministeps. 
  */
  {
    const char *rate_vars[] = {"OPR" , "GPR" , "WPR" , "GOR" , "WCT"};
    int num_rate_vars = sizeof( rate_vars ) / sizeof( rate_vars[0] );
    bool  is_rate           = false;
    int ivar;
    for (ivar = 0; ivar < num_rate_vars; ivar++) {
      const char * var_substring = &smspec_node->keyword[1];
      if (util_string_equal( rate_vars[ivar] , var_substring)) {
        is_rate = true;
        break;
      }
    }
    smspec_node->rate_variable = is_rate;
  }
  
  /*
    This code checks in a predefined list whether a certain WGNAMES
    variable represents a total accumulated quantity. Only the last three
    characters in the variable is considered (i.e. the leading 'W', 'G' or
    'F' is discarded).
    
    The list below is all the keyowrds with 'Total' in the information from
    the tables 2.7 - 2.11 in the ECLIPSE fileformat documentation.  Have
    skipped some of the most exotic keywords (AND ALL THE HISTORICAL).
  */
  {
    bool is_total = false;
    if (smspec_node->var_type == ECL_SMSPEC_WELL_VAR || smspec_node->var_type == ECL_SMSPEC_GROUP_VAR || smspec_node->var_type == ECL_SMSPEC_FIELD_VAR) {
      const char *total_vars[] = {"OPT"  , "GPT"  , "WPT" , "OPTF" , "OPTS" , "OIT"  , "OVPT" , "OVIT" , "MWT" , "WIT" ,
                                  "WVPT" , "WVIT" , "GMT"  , "GPTF" , "GIT"  , "SGT"  , "GST" , "FGT" , "GCT" , "GIMT" , 
                                  "WGPT" , "WGIT" , "EGT"  , "EXGT" , "GVPT" , "GVIT" , "LPT" , "VPT" , "VIT" };

      int num_total_vars = sizeof( total_vars ) / sizeof( total_vars[0] );
      int ivar;
      for (ivar = 0; ivar < num_total_vars; ivar++) {
        const char * var_substring = &smspec_node->keyword[1];
        if (util_string_equal( total_vars[ivar] , var_substring)) {
          is_total = true;
          break;
        }
      }
    }
    smspec_node->total_variable = is_total;
  }
}



static smspec_node_type * smspec_node_alloc_empty(ecl_smspec_var_type var_type, const char * keyword , const char * unit , int params_index, float default_value) {
  smspec_node_type * node = util_malloc( sizeof * node , __func__);
  
  UTIL_TYPE_ID_INIT( node , SMSPEC_TYPE_ID);
  /** These can stay with values NULL / SMSPEC_NUMS_INVALID for variables where those fields are not accessed. */
  node->wgname        = NULL;
  node->num           = SMSPEC_NUMS_INVALID;

  node->gen_key       = NULL;
  node->params_index  = SMSPEC_PARAMS_INDEX_INVALID;

  /** All smspec_node instances should have valid values of these fields. */
  node->var_type      = var_type;
  node->unit          = util_alloc_string_copy( unit );
  node->keyword       = util_alloc_string_copy( keyword );
  node->params_index  = params_index;
  node->lgr_name      = NULL;
  node->lgr_ijk       = NULL;
  node->default_value = default_value;
  
  smspec_node_set_flags( node );
  return node;
}


void smspec_node_set_wgname( smspec_node_type * index , const char * wgname ) {
  if (DUMMY_WELL( wgname ))
    util_abort("%s: trying to set/dereference WGNAME = %s which is invalid \n",__func__);
  
  index->wgname = util_realloc_string_copy(index->wgname , wgname );
}


void smspec_node_set_lgr_name( smspec_node_type * index , const char * lgr_name ) {
  index->lgr_name = util_realloc_string_copy(index->lgr_name , lgr_name);
}


void smspec_node_set_lgr_ijk( smspec_node_type * index , int lgr_i , int lgr_j , int lgr_k) {
  if (index->lgr_ijk == NULL)
    index->lgr_ijk = util_malloc( 3 * sizeof * index->lgr_ijk , __func__);
  
  index->lgr_ijk[0] = lgr_i;
  index->lgr_ijk[1] = lgr_j;
  index->lgr_ijk[2] = lgr_k;
}


void smspec_node_set_num( smspec_node_type * index , int num) {
  if (num == SMSPEC_NUMS_INVALID)
    util_abort("%s: explicitly trying to set nums == SMSPEC_NUMS_INVALID - seems like a bug?!\n",__func__);
  
  index->num = num;
}

/**
   This function will init the gen_key field of the smspec_node
   instance; this is the keyw which is used to install the
   smspec_node instance in the gen_var dictionary. The node related
   to grid locations are installed with both a XXX:num and XXX:i,j,k
   in the gen_var dictionary; this function will initializethe XXX:num
   form.
*/


void smspec_node_set_gen_key( smspec_node_type * smspec_node , const char * key_join_string) {
  switch( smspec_node->var_type) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    // KEYWORD:WGNAME:NUM
    smspec_node->gen_key = smspec_alloc_completion_num_key( key_join_string , smspec_node->keyword , smspec_node->wgname , smspec_node->num);
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    // KEYWORD
    smspec_node->gen_key = util_alloc_string_copy( smspec_node->keyword );
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    // KEYWORD:WGNAME 
    smspec_node->gen_key = smspec_alloc_group_key( key_join_string , smspec_node->keyword , smspec_node->wgname);
    break;
  case(ECL_SMSPEC_WELL_VAR):
    // KEYWORD:WGNAME 
    smspec_node->gen_key = smspec_alloc_well_key( key_join_string , smspec_node->keyword , smspec_node->wgname);
    break;
  case(ECL_SMSPEC_REGION_VAR):
    // KEYWORD:NUM
    smspec_node->gen_key = smspec_alloc_region_key( key_join_string , smspec_node->keyword , smspec_node->num);
    break;
  case(ECL_SMSPEC_SEGMENT_VAR):
    // KEYWORD:WGNAME:NUM 
    smspec_node->gen_key = smspec_alloc_segment_key( key_join_string , smspec_node->keyword , smspec_node->wgname , smspec_node->num);
    break;
  case(ECL_SMSPEC_MISC_VAR):
    // KEYWORD
    /* Misc variable - i.e. date or CPU time ... */
    smspec_node->gen_key = util_alloc_string_copy( smspec_node->keyword );
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    // KEYWORD:NUM
    smspec_node->gen_key = smspec_alloc_block_num_key( key_join_string , smspec_node->keyword , smspec_node->num);
    break;
  case(ECL_SMSPEC_LOCAL_WELL_VAR):
    /** KEYWORD:LGR:WGNAME */
    smspec_node->gen_key = smspec_alloc_local_well_key( key_join_string , smspec_node->keyword , smspec_node->lgr_name , smspec_node->wgname);
    break;
  case(ECL_SMSPEC_LOCAL_BLOCK_VAR):
    /* KEYWORD:LGR:i,j,k */
    smspec_node->gen_key = smspec_alloc_local_block_key( key_join_string , 
                                                         smspec_node->keyword , 
                                                         smspec_node->lgr_name , 
                                                         smspec_node->lgr_ijk[0] , 
                                                         smspec_node->lgr_ijk[1] , 
                                                         smspec_node->lgr_ijk[2] );
    break;
  case(ECL_SMSPEC_LOCAL_COMPLETION_VAR):
    /* KEYWORD:LGR:WELL:i,j,k */
    smspec_node->gen_key = smspec_alloc_local_completion_key( key_join_string , 
                                                              smspec_node->keyword , 
                                                              smspec_node->lgr_name , 
                                                              smspec_node->wgname , 
                                                              smspec_node->lgr_ijk[0],
                                                              smspec_node->lgr_ijk[1],
                                                              smspec_node->lgr_ijk[2]);
    
    break;
  default:
    util_abort("%s: internal error - should not be here? \n");
  }
}


  
/**
   This function will allocate a smspec_node instance, and initialize
   all the elements. Observe that the function can return NULL, in the
   case we do not care to internalize the variable in question,
   i.e. if it is a well_rate from a dummy well or a variable type we
   do not support at all.

   This function initializes a valid smspec_node instance based on
   the supplied var_type, and the input. Observe that when a new
   variable type is supported, the big switch() statement must be
   updated in the functions ecl_smspec_install_gen_key() and
   ecl_smspec_fread_header() functions in addition. UGGGLY
*/


smspec_node_type * smspec_alloc_well_var( const char * wgname , 
                                          const char * keyword , 
                                          const char * unit , 
                                          const char * key_join_string , 
                                          int param_index , 
                                          float default_value) {

  smspec_node_type * smspec_node = smspec_node_alloc_empty( ECL_SMSPEC_WELL_VAR , keyword , unit , param_index , default_value);
  smspec_node_set_wgname( smspec_node , wgname );
  
  return smspec_node;
}



 smspec_node_type * smspec_node_alloc( ecl_smspec_var_type var_type , 
                                       const char * wgname  , 
                                       const char * keyword , 
                                       const char * unit    , 
                                       const char * key_join_string , 
                                       int num , int param_index, float default_value) {
  smspec_node_type * smspec_node = NULL;
  
  switch (var_type) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    /* Completion variable : WGNAME & NUM */
    if (!DUMMY_WELL(wgname)) {
      smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
      smspec_node_set_wgname( smspec_node , wgname );
      smspec_node_set_num( smspec_node , num );
    }
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    /* Field variable : */
    smspec_node = smspec_node_alloc_empty( var_type ,  keyword , unit , param_index , default_value);
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    /* Group variable : WGNAME */
    if (!DUMMY_WELL(wgname)) {
      smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
      smspec_node_set_wgname( smspec_node , wgname );
    }
    break;
  case(ECL_SMSPEC_WELL_VAR):
    /* Well variable : WGNAME */
    if (!DUMMY_WELL(wgname)) {
      smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
      smspec_node_set_wgname( smspec_node , wgname );
    }
    break;
  case(ECL_SMSPEC_SEGMENT_VAR):
    if (!DUMMY_WELL( wgname )) {
      smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
      smspec_node_set_wgname( smspec_node , wgname );
      smspec_node_set_num( smspec_node , num );
    }
    break;
  case(ECL_SMSPEC_REGION_VAR):
    /* Region variable : NUM */
    smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
    smspec_node_set_num( smspec_node , num );
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    /* A block variable : NUM*/
    smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
    smspec_node_set_num( smspec_node , num );
    break;
  case(ECL_SMSPEC_MISC_VAR):
    /* Misc variable : */
    smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
    break;
  default:
    /* Lots of legitimate alternatives which are not handled .. */
    break;
  }
  
  
  if (smspec_node != NULL) 
    smspec_node_set_gen_key( smspec_node , key_join_string );

  return smspec_node;
}


smspec_node_type * smspec_node_alloc_lgr( ecl_smspec_var_type var_type , 
                                          const char * wgname  , 
                                          const char * keyword , 
                                          const char * unit    , 
                                          const char * lgr , 
                                          const char * key_join_string , 
                                          int   lgr_i, int lgr_j , int lgr_k,
                                          int param_index , float default_value) {

  smspec_node_type * smspec_node = NULL;
  switch (var_type) {
  case(ECL_SMSPEC_LOCAL_WELL_VAR):
    if (!DUMMY_WELL(wgname)) {
      smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
      smspec_node_set_wgname( smspec_node , wgname );
      smspec_node_set_lgr_name( smspec_node , lgr );
    }
    break;
  case(ECL_SMSPEC_LOCAL_BLOCK_VAR):
    smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
    smspec_node_set_lgr_name( smspec_node , lgr );
    smspec_node_set_lgr_ijk( smspec_node , lgr_i, lgr_j , lgr_k );
    break;
  case(ECL_SMSPEC_LOCAL_COMPLETION_VAR):
    if (!DUMMY_WELL(wgname)) {
      smspec_node = smspec_node_alloc_empty( var_type , keyword , unit , param_index , default_value);
      smspec_node_set_lgr_name( smspec_node , lgr );
      smspec_node_set_wgname( smspec_node , wgname );
      smspec_node_set_lgr_ijk( smspec_node , lgr_i, lgr_j , lgr_k );
    }
    break;
  default:
    util_abort("%s: internal error:  in LGR function with  non-LGR keyword:%s \n",__func__ , keyword);
  }
  smspec_node_set_gen_key( smspec_node , key_join_string );
  return smspec_node;
}


void smspec_node_free( smspec_node_type * index ) {
  free( index->unit );
  free( index->keyword );
  util_safe_free( index->gen_key );
  util_safe_free( index->wgname );
  util_safe_free( index->lgr_name );
  util_safe_free( index->lgr_ijk );
  free( index );
}

void smspec_node_free__( void * arg ) {
  smspec_node_type * node = smspec_node_safe_cast( arg );
  smspec_node_free( node );
}


/*****************************************************************/

int smspec_node_get_params_index( const smspec_node_type * smspec_node ) {
  return smspec_node->params_index;
}


void smspec_node_set_params_index( smspec_node_type * smspec_node , int params_index) {
  smspec_node->params_index = params_index;
}

const char * smspec_node_get_gen_key( const smspec_node_type * smspec_node) {
  return smspec_node->gen_key;
}

const char * smspec_node_get_wgname( const smspec_node_type * smspec_node) {
  return smspec_node->wgname;
}

const char * smspec_node_get_keyword( const smspec_node_type * smspec_node) {
  return smspec_node->keyword;
}

ecl_smspec_var_type smspec_node_get_var_type( const smspec_node_type * smspec_node) {
  return smspec_node->var_type;
}

int smspec_node_get_num( const smspec_node_type * smspec_node) {
  return smspec_node->num;
}

bool smspec_node_is_rate( const smspec_node_type * smspec_node ) {
  return smspec_node->rate_variable;
}


bool smspec_node_is_total( const smspec_node_type * smspec_node ){ 
  return smspec_node->total_variable;
}


const char  * smspec_node_get_unit( const smspec_node_type * smspec_node) {
  return smspec_node->unit;
}


float smspec_node_get_default_value( const smspec_node_type * smspec_node ) {
  return smspec_node->default_value;
}
