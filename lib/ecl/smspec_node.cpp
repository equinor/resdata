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
#include <stddef.h>
#include <math.h>
#include <time.h>

#include <string>
#include <set>
#include <array>
#include <stdexcept>

#include <ert/util/hash.hpp>
#include <ert/util/util.h>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/type_macros.hpp>

#include <ert/ecl/ecl_kw.hpp>
#include <ert/ecl/ecl_util.hpp>
#include <ert/ecl/ecl_smspec.hpp>
#include <ert/ecl/smspec_node.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_kw_magic.hpp>

#include "detail/util/string_util.hpp"


/**
   This struct contains meta-information about one element in the smspec
   file; the content is based on the smspec vectors WGNAMES, KEYWORDS, UNIT
   and NUMS. The index field of this struct points to where the actual data
   can be found in the PARAMS vector of the *.Snnnn / *.UNSMRY files;
   probably the most important field.
*/



static bool smspec_node_need_wgname(ecl_smspec_var_type var_type) {
  if (var_type == ECL_SMSPEC_COMPLETION_VAR ||
      var_type == ECL_SMSPEC_GROUP_VAR      ||
      var_type == ECL_SMSPEC_WELL_VAR       ||
      var_type == ECL_SMSPEC_SEGMENT_VAR)
    return true;
  else
    return false;
}

static bool smspec_node_type_supported(ecl_smspec_var_type var_type) {
  if (var_type == ECL_SMSPEC_NETWORK_VAR)
    return false;

  return true;
}


/*****************************************************************/
/*
  The key formats for the combined keys like e.g. 'WWCT:OP_5' should
  have the keyword, i.e. 'WWCT', as the first part of the string. That
  guarantees that the function ecl_smspec_identify_var_type() can take
  both a pure ECLIPSE variable name, like .e.g 'WWCT' and also an
  ecl_sum combined key like 'WWCT:OPX' as input.
*/

#define ECL_SUM_KEYFMT_AQUIFER                "%s%s%d"
#define ECL_SUM_KEYFMT_BLOCK_IJK              "%s%s%d,%d,%d"
#define ECL_SUM_KEYFMT_BLOCK_NUM              "%s%s%d"
#define ECL_SUM_KEYFMT_LOCAL_BLOCK            "%s%s%s%s%d,%d,%d"
#define ECL_SUM_KEYFMT_COMPLETION_IJK         "%s%s%s%s%d,%d,%d"
#define ECL_SUM_KEYFMT_COMPLETION_NUM         "%s%s%s%s%d"
#define ECL_SUM_KEYFMT_LOCAL_COMPLETION       "%s%s%s%s%s%s%d,%d,%d"
#define ECL_SUM_KEYFMT_GROUP                  "%s%s%s"
#define ECL_SUM_KEYFMT_WELL                   "%s%s%s"
#define ECL_SUM_KEYFMT_REGION                 "%s%s%d"
#define ECL_SUM_KEYFMT_REGION_2_REGION_R1R2   "%s%s%d-%d"
#define ECL_SUM_KEYFMT_REGION_2_REGION_NUM    "%s%s%d"
#define ECL_SUM_KEYFMT_SEGMENT                "%s%s%s%s%d"
#define ECL_SUM_KEYFMT_LOCAL_WELL             "%s%s%s%s%s"


std::string smspec_alloc_block_num_key( const char * join_string , const std::string& keyword , int num) {
  return ecl::util::string_format(ECL_SUM_KEYFMT_BLOCK_NUM,
                            keyword.c_str() ,
                            join_string ,
                            num );
}

std::string smspec_alloc_aquifer_key( const char * join_string , const std::string& keyword , int num) {
  return ecl::util::string_format(ECL_SUM_KEYFMT_AQUIFER,
                            keyword.c_str(),
                            join_string ,
                            num );
}


std::string smspec_alloc_local_block_key( const char * join_string , const std::string& keyword , const std::string& lgr_name , int i , int j , int k) {
  return ecl::util::string_format(ECL_SUM_KEYFMT_LOCAL_BLOCK ,
                            keyword.c_str() ,
                            join_string ,
                            lgr_name.c_str() ,
                            join_string ,
                            i,j,k);
}


std::string smspec_alloc_region_key( const char * join_string , const std::string& keyword , int num) {
  return ecl::util::string_format(ECL_SUM_KEYFMT_REGION ,
                            keyword.c_str(),
                            join_string ,
                            num );
}

std::string smspec_alloc_region_2_region_r1r2_key( const char * join_string , const std::string& keyword , int r1, int r2) {
  return ecl::util::string_format(ECL_SUM_KEYFMT_REGION_2_REGION_R1R2,
                            keyword.c_str(),
                            join_string,
                            r1,
                            r2);
}

std::string smspec_alloc_region_2_region_num_key( const char * join_string , const std::string& keyword , int num) {
  return ecl::util::string_format(ECL_SUM_KEYFMT_REGION_2_REGION_NUM,
                            keyword.c_str() ,
                            join_string ,
                            num);
}






std::string smspec_alloc_completion_ijk_key( const char * join_string , const std::string& keyword, const std::string& wgname , int i , int j , int k) {
  if (wgname.size() > 0)
    return ecl::util::string_format( ECL_SUM_KEYFMT_COMPLETION_IJK ,
                               keyword.c_str() ,
                               join_string ,
                               wgname.c_str(),
                               join_string ,
                               i , j , k );
  else
    return NULL;
}



std::string smspec_alloc_completion_num_key( const char * join_string , const std::string& keyword, const std::string& wgname , int num) {
  if (wgname.size() > 0)
    return ecl::util::string_format(ECL_SUM_KEYFMT_COMPLETION_NUM,
                              keyword.c_str() ,
                              join_string ,
                              wgname.c_str() ,
                              join_string ,
                              num );
  else
    return NULL;
}

/*
  To support ECLIPSE behaviour where new wells/groups can be created
  during the simulation it must be possible to create a smspec node
  with an initially unknown well/group name; all gen_key formats which
  use the wgname value must therefore accept a NULL value for wgname. HMM: Uncertain about this?
*/

static std::string smspec_alloc_wgname_key( const char * join_string , const std::string& keyword , const std::string& wgname) {
  if (wgname.size() > 0)
    return ecl::util::string_format(ECL_SUM_KEYFMT_WELL,
                                    keyword.c_str() ,
                                    join_string ,
                                    wgname.c_str() );
  else
    return "";
}

std::string smspec_alloc_group_key( const char * join_string , const std::string& keyword , const std::string& wgname) {
  return smspec_alloc_wgname_key( join_string , keyword , wgname );
}

std::string smspec_alloc_well_key( const char * join_string , const std::string& keyword , const std::string& wgname) {
  return smspec_alloc_wgname_key( join_string , keyword , wgname );
}


/*
  The smspec_alloc_well_key and smspec_alloc_block_ijk_key() require C linkage due to external use.
*/

char * smspec_alloc_well_key( const char * join_string , const char * keyword , const char * wgname) {
  return util_alloc_sprintf( ECL_SUM_KEYFMT_WELL,
                             keyword,
                             join_string,
                             wgname);
}

char * smspec_alloc_block_ijk_key( const char * join_string , const char * keyword , int i , int j , int k) {
  return util_alloc_sprintf(ECL_SUM_KEYFMT_BLOCK_IJK,
                            keyword,
                            join_string,
                            i,j,k);
}

std::string smspec_alloc_block_ijk_key( const char * join_string , const std::string& keyword , int i , int j , int k) {
  return ecl::util::string_format(ECL_SUM_KEYFMT_BLOCK_IJK ,
                                  keyword.c_str(),
                                  join_string ,
                                  i,j,k);
}

std::string smspec_alloc_segment_key( const char * join_string , const std::string& keyword , const std::string& wgname , int num) {
  if (wgname.size() > 0)
    return ecl::util::string_format(ECL_SUM_KEYFMT_SEGMENT ,
                              keyword.c_str() ,
                              join_string ,
                              wgname.c_str(),
                              join_string ,
                              num );
  else
    return NULL;
}


std::string smspec_alloc_local_well_key( const char * join_string , const std::string& keyword , const std::string& lgr_name , const std::string& wgname) {
  if (wgname.size() > 0)
    return ecl::util::string_format( ECL_SUM_KEYFMT_LOCAL_WELL ,
                               keyword.c_str() ,
                               join_string ,
                               lgr_name.c_str() ,
                               join_string ,
                               wgname.c_str());
  else
    return NULL;
}

std::string smspec_alloc_local_completion_key( const char * join_string, const std::string& keyword , const std::string& lgr_name , const std::string& wgname , int i , int j , int k) {
  if (wgname.size() > 0)
    return ecl::util::string_format(ECL_SUM_KEYFMT_LOCAL_COMPLETION ,
                              keyword.c_str(),
                              join_string ,
                              lgr_name.c_str() ,
                              join_string ,
                              wgname.c_str(),
                              join_string ,
                              i,j,k);
  else
    return NULL;
}

/*****************************************************************/


bool smspec_node_identify_rate(const char * keyword) {
  const char *rate_vars[] = {"OPR" , "GPR" , "WPR" , "LPR", "OIR", "GIR", "WIR", "LIR", "GOR" , "WCT",
                             "OFR" , "GFR" , "WFR"};
  int num_rate_vars = sizeof( rate_vars ) / sizeof( rate_vars[0] );
  bool  is_rate           = false;
  int ivar;
  for (ivar = 0; ivar < num_rate_vars; ivar++) {
    const char * var_substring = &keyword[1];
    if (strncmp( rate_vars[ivar] , var_substring , strlen( rate_vars[ivar] )) == 0) {
      is_rate = true;
      break;
    }
  }
  return is_rate;
}


bool smspec_node_identify_total(const char * keyword, ecl_smspec_var_type var_type) {
 /*
    This code checks in a predefined list whether a certain WGNAMES
    variable represents a total accumulated quantity. Only the last three
    characters in the variable is considered (i.e. the leading 'W', 'G' or
    'F' is discarded).

    The list below is all the keyowrds with 'Total' in the information from
    the tables 2.7 - 2.11 in the ECLIPSE fileformat documentation.  Have
    skipped some of the most exotic keywords.
  */
  bool is_total = false;
  if (var_type == ECL_SMSPEC_WELL_VAR ||
      var_type == ECL_SMSPEC_GROUP_VAR ||
      var_type == ECL_SMSPEC_FIELD_VAR ||
      var_type == ECL_SMSPEC_REGION_VAR ||
      var_type == ECL_SMSPEC_COMPLETION_VAR ) {
    const char *total_vars[] = {"OPT"  , "GPT"  , "WPT" , "GIT", "WIT", "OPTF" , "OPTS" , "OIT"  , "OVPT" , "OVIT" , "MWT" ,
                                "WVPT" , "WVIT" , "GMT"  , "GPTF" , "SGT"  , "GST" , "FGT" , "GCT" , "GIMT" ,
                                "WGPT" , "WGIT" , "EGT"  , "EXGT" , "GVPT" , "GVIT" , "LPT" , "VPT" , "VIT" , "NPT" , "NIT",
                                "CPT", "CIT"};

    int num_total_vars = sizeof( total_vars ) / sizeof( total_vars[0] );
    int ivar;
    for (ivar = 0; ivar < num_total_vars; ivar++) {
      const char * var_substring = &keyword[1];
      /*
        We want to mark both FOPT and FOPTH as total variables;
        we use strncmp() to make certain that the trailing 'H' is
        not included in the comparison.
      */
      if (strncmp( total_vars[ivar] , var_substring , strlen( total_vars[ivar] )) == 0) {
        is_total = true;
        break;
      }
    }
  }
  else if (var_type == ECL_SMSPEC_SEGMENT_VAR) {
    const char *total_vars[] = {"OFT", "GFT", "WFT"};
    const char *var_substring = &keyword[1];
    const size_t num_total_vars = sizeof(total_vars) / sizeof(total_vars[0]);
    for (size_t ivar = 0; ivar < num_total_vars; ivar++)
      if (strncmp(total_vars[ivar], var_substring, strlen(total_vars[ivar])) == 0) {
        is_total = true;
        break;
      }
  }
  return is_total;
}


namespace ecl {


void smspec_node_type::set_keyword( const std::string& keyword_ ) {
  // ECLIPSE Standard: Max eight characters - everything beyond is silently dropped
  // This function can __ONLY__ be called on time; run-time chaning of keyword is not
  // allowed.
  if (keyword.size() == 0)
    this->keyword = keyword_;
  else
    util_abort("%s: fatal error - attempt to change keyword runtime detected - aborting\n",__func__);
}


void smspec_node_type::set_invalid_flags() {
  this->rate_variable  = false;
  this->total_variable = false;
  this->historical     = false;
}


void smspec_node_type::set_flags() {
  /*
     Check if this is a rate variabel - that info is used when
     interpolating results to true_time between ministeps.
  */
  rate_variable = smspec_node_identify_rate(keyword.c_str());
  if (keyword.back() == 'H')
    historical = true;
  total_variable = smspec_node_identify_total(keyword.c_str(), var_type);
}

float smspec_node_type::get_default() const {
  return this->default_value;
}


void smspec_node_type::set_lgr_ijk( int lgr_i , int lgr_j , int lgr_k) {
  lgr_ijk[0] = lgr_i;
  lgr_ijk[1] = lgr_j;
  lgr_ijk[2] = lgr_k;
}


void smspec_node_type::init_num( ecl_smspec_var_type var_type_) {
  switch( var_type_ ) {
  case(ECL_SMSPEC_WELL_VAR):
    num = SMSPEC_NUMS_WELL;
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    num = SMSPEC_NUMS_GROUP;
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    num = SMSPEC_NUMS_FIELD;
    break;
  default:
    num = SMSPEC_NUMS_INVALID;
  }
}

void smspec_node_type::set_num( const int grid_dims[3] , int num_) {
  if (num_ == SMSPEC_NUMS_INVALID)
    util_abort("%s: explicitly trying to set nums == SMSPEC_NUMS_INVALID - seems like a bug?!\n",__func__);

  num = num_;
  if ((var_type == ECL_SMSPEC_COMPLETION_VAR) || (var_type == ECL_SMSPEC_BLOCK_VAR)) {
    int global_index = num - 1;
    ijk[2] = global_index / ( grid_dims[0] * grid_dims[1] );   global_index -= ijk[2] * (grid_dims[0] * grid_dims[1]);
    ijk[1] = global_index /  grid_dims[0] ;                    global_index -= ijk[1] * grid_dims[0];
    ijk[0] = global_index;

    ijk[0] += 1;
    ijk[1] += 1;
    ijk[2] += 1;
  }
}

void smspec_node_type::decode_R1R2(int * r1 , int * r2) const {
  if (var_type == ECL_SMSPEC_REGION_2_REGION_VAR) {
    *r1 = num % 32768;
    *r2 = ((num - (*r1)) / 32768)-10;
  } else {
    *r1 = -1;
    *r2 = -1;
  }
}



/**
   This function will init the gen_key field of the smspec_node
   instance; this is the keyw which is used to install the
   smspec_node instance in the gen_var dictionary. The node related
   to grid locations are installed with both a XXX:num and XXX:i,j,k
   in the gen_var dictionary; this function will initializethe XXX:num
   form.
*/


void smspec_node_type::set_gen_keys( const char * key_join_string_) {
  switch( var_type) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    // KEYWORD:WGNAME:NUM
    gen_key1 = smspec_alloc_completion_ijk_key( key_join_string_ , keyword , wgname , ijk[0], ijk[1], ijk[2]);
    gen_key2 = smspec_alloc_completion_num_key( key_join_string_ , keyword , wgname , num);
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    // KEYWORD
    gen_key1 = keyword;
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    // KEYWORD:WGNAME
    gen_key1 = smspec_alloc_group_key( key_join_string_ , keyword , wgname);
    break;
  case(ECL_SMSPEC_WELL_VAR):
    // KEYWORD:WGNAME
    gen_key1 = smspec_alloc_well_key( key_join_string_ , keyword , wgname);
    break;
  case(ECL_SMSPEC_REGION_VAR):
    // KEYWORD:NUM
    gen_key1 = smspec_alloc_region_key( key_join_string_ , keyword , num);
    break;
  case (ECL_SMSPEC_SEGMENT_VAR):
    // KEYWORD:WGNAME:NUM
    gen_key1 = smspec_alloc_segment_key( key_join_string_ , keyword , wgname , num);
    break;
  case(ECL_SMSPEC_REGION_2_REGION_VAR):
    // KEYWORDS:RXF:NUM and RXF:R1-R2
    {
      int r1,r2;
      decode_R1R2( &r1 , &r2);
      gen_key1 = smspec_alloc_region_2_region_r1r2_key( key_join_string_ , keyword , r1, r2);
    }
    gen_key2 = smspec_alloc_region_2_region_num_key( key_join_string_ , keyword , num);
    break;
  case(ECL_SMSPEC_MISC_VAR):
    // KEYWORD
    /* Misc variable - i.e. date or CPU time ... */
    gen_key1 = keyword;
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    // KEYWORD:NUM
    gen_key1 = smspec_alloc_block_ijk_key( key_join_string_ , keyword , ijk[0], ijk[1], ijk[2]);
    gen_key2 = smspec_alloc_block_num_key( key_join_string_ , keyword , num);
    break;
  case(ECL_SMSPEC_LOCAL_WELL_VAR):
    /** KEYWORD:LGR:WGNAME */
    gen_key1 = smspec_alloc_local_well_key( key_join_string_ , keyword , lgr_name , wgname);
    break;
  case(ECL_SMSPEC_LOCAL_BLOCK_VAR):
    /* KEYWORD:LGR:i,j,k */
    gen_key1 = smspec_alloc_local_block_key( key_join_string_ , keyword , lgr_name , lgr_ijk[0] , lgr_ijk[1] , lgr_ijk[2] );
    break;
  case(ECL_SMSPEC_LOCAL_COMPLETION_VAR):
    /* KEYWORD:LGR:WELL:i,j,k */
    gen_key1 = smspec_alloc_local_completion_key( key_join_string_ , keyword , lgr_name , wgname , lgr_ijk[0], lgr_ijk[1], lgr_ijk[2]);

    break;
  case(ECL_SMSPEC_AQUIFER_VAR):
    gen_key1 = smspec_alloc_aquifer_key( key_join_string_ , keyword , num);
    break;
  default:
    util_abort("%s: internal error - should not be here? \n" , __func__);
  }
}



void smspec_node_type::common_init( ecl_smspec_var_type var_type_ , const char * keyword_ , const std::string& unit_ ) {
  if (var_type == ECL_SMSPEC_INVALID_VAR) {
    this->unit = unit_;
    this->keyword = keyword_;
    this->var_type = var_type_;
    set_flags();
    init_num( var_type_ );
  } else
    util_abort("%s: trying to re-init smspec node with keyword:%s - invalid \n",__func__ , keyword_ );
}


bool smspec_node_type::init__( ecl_smspec_var_type var_type ,
                                           const char * wgname_  ,
                                           const char * keyword_ ,
                                           const char * unit    ,
                                           const char * key_join_string ,
                                           const int grid_dims[3] ,
                                           int num) {

  bool initOK    = true;

  common_init( var_type , keyword_ , unit );
  switch (var_type) {
  case(ECL_SMSPEC_COMPLETION_VAR):
    /* Completion variable : WGNAME & NUM */
    set_num( grid_dims , num );
    wgname = wgname_;
    if (num < 0)
      initOK = false;
    break;
  case(ECL_SMSPEC_GROUP_VAR):
    /* Group variable : WGNAME */
    wgname = wgname_;
    break;
  case(ECL_SMSPEC_WELL_VAR):
    /* Well variable : WGNAME */
    wgname = wgname_;
    break;
  case(ECL_SMSPEC_SEGMENT_VAR):
    wgname = wgname_;
    set_num( grid_dims , num );
    if (num < 0)
      initOK = false;
    break;
  case(ECL_SMSPEC_FIELD_VAR):
    /* Field variable : */
    /* Fully initialized with the smspec_common_init() function */
    break;
  case(ECL_SMSPEC_REGION_VAR):
    /* Region variable : NUM */
    set_num( grid_dims , num );
    break;
  case(ECL_SMSPEC_REGION_2_REGION_VAR):
    /* Region 2 region variable : NUM */
    set_num( grid_dims , num );
    break;
  case(ECL_SMSPEC_BLOCK_VAR):
    /* A block variable : NUM*/
    set_num( grid_dims , num );
    break;
  case(ECL_SMSPEC_MISC_VAR):
    /* Misc variable : */

    /*
       For some keywords the SMSPEC files generated by Eclipse have a
       non zero NUMS value although; it seems that value is required
       for the generatd summaryfiles to display nicely in
       e.g. S3GRAF.
    */

    if (util_string_equal( keyword_ ,SMSPEC_TIME_KEYWORD))
      set_num( grid_dims , SMSPEC_TIME_NUMS_VALUE );

    if (util_string_equal( keyword_ ,SMSPEC_YEARS_KEYWORD))
      set_num( grid_dims , SMSPEC_YEARS_NUMS_VALUE );

    break;
  case(ECL_SMSPEC_AQUIFER_VAR):
    set_num( grid_dims , num );
    break;
  default:
    /* Lots of legitimate alternatives which are not internalized. */
    initOK = false;
    break;
  }

  if (initOK)
    set_gen_keys( key_join_string );

  return initOK;
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

smspec_node_type::smspec_node_type() {}

smspec_node_type::smspec_node_type( ecl_smspec_var_type var_type_ ,
                                    const char * wgname  ,
                                    const char * keyword ,
                                    const char * unit    ,
                                    const char * key_join_string ,
                                    const int grid_dims[3] ,
                                    int num , int param_index_, float default_value_) {
  /*
    Well and group names in the wgname parameter is quite messy. The
    situation is as follows:

     o The ECLIPSE SMSPEC files are very verbose, and contain many
       entries like this:

            KEYWORD : "WWCT"
            WGNAME  : ":+:+:+:+"

       i.e. the keyword indicates that this is a perfectly legitimate
       well variable, however the special wgname value ":+:+:+:+"
       shows that this just a rubbish entry. We do not want to
       internalize these rubbish entries and this function should just
       return NULL.

       The ":+:+:+:+" string is in the #define symbol DUMMY_WELL and
       the macro IS_DUMMY_WELL(wgname) can used to compare with the
       DUMMY_WELL value.

     o When the ecl_sum instance is created in write mode; it must be
       possible to add smspec nodes for wells/groups which do not have
       a name yet. In this case we accept NULL as input value for the
       wgname parameter.

     o In the case of variables which do not use the wgname variable
       at all, e.g. like "FOPT" - the wgname input value is ignored
       completely.
  */

  if (smspec_node_need_wgname(var_type_) && IS_DUMMY_WELL(wgname))
    throw std::invalid_argument("Wrong input params to smspec_node constructor.");

  if (!smspec_node_type_supported(var_type_))
    throw std::invalid_argument("Wrong input params to smspec_node constructor.");

  /*
    TODO: The init function should be joined in this functions.
  */

  params_index  = param_index_;
  default_value = default_value_;

  var_type      = ECL_SMSPEC_INVALID_VAR;
  set_invalid_flags();

  bool initOK = init__( var_type_ , wgname , keyword , unit , key_join_string , grid_dims, num);
  if (!initOK)
    throw std::invalid_argument("Wrong input params to smspec_node constructor.");
}


void smspec_node_type::init_lgr( ecl_smspec_var_type var_type ,
                                 const char * wgname_  ,
                                 const char * keyword_ ,
                                 const char * unit    ,
                                 const char * lgr ,
                                 const char * key_join_string ,
                                 int   lgr_i, int lgr_j , int lgr_k
                                 ) {
  bool initOK = true;
  bool wgnameOK = true;
  if ((wgname_ != NULL) && (IS_DUMMY_WELL(wgname_)))
    wgnameOK = false;

  common_init( var_type , keyword_ , unit );
  switch (var_type) {
  case(ECL_SMSPEC_LOCAL_WELL_VAR):
    wgname = wgname_;
    lgr_name = lgr;
    initOK = wgnameOK;
    break;
  case(ECL_SMSPEC_LOCAL_BLOCK_VAR):
    lgr_name = lgr;
    set_lgr_ijk( lgr_i, lgr_j , lgr_k );
    break;
  case(ECL_SMSPEC_LOCAL_COMPLETION_VAR):
    lgr_name = lgr;
    wgname = wgname_;
    set_lgr_ijk( lgr_i, lgr_j , lgr_k );
    initOK = wgnameOK;
    break;
  default:
    util_abort("%s: internal error:  in LGR function with  non-LGR keyword:%s \n",__func__ , keyword_);
  }

  if (initOK)
    set_gen_keys( key_join_string );
}

smspec_node_type::smspec_node_type( ecl_smspec_var_type var_type_ ,
                                    const char * wgname_  ,
                                    const char * keyword_ ,
                                    const char * unit_    ,
                                    const char * lgr_ ,
                                    const char * key_join_string_ ,
                                    int   lgr_i, int lgr_j , int lgr_k,
                                    int param_index_ , float default_value_) {
  params_index  = param_index_;
  default_value = default_value_;

  var_type      = ECL_SMSPEC_INVALID_VAR;
  set_invalid_flags();

  init_lgr( var_type_ , wgname_ , keyword_ , unit_ , lgr_ , key_join_string_ , lgr_i, lgr_j , lgr_k);
  
}


smspec_node_type * smspec_node_type::copy() const {
  smspec_node_type * copy = new smspec_node_type();
  *copy = *this;
  return copy;
}



/*****************************************************************/

int smspec_node_type::get_params_index() const {
  return this->params_index;
}

void smspec_node_type::set_params_index( int params_index_) {
  this->params_index = params_index_;
}


namespace {

  const char * get_cstring(const std::string& s) {
    if (s.empty())
      return NULL;
    else
      return s.c_str();
  }

}


const char * smspec_node_type::get_gen_key1() const {
  return get_cstring( this->gen_key1 );
}

const char * smspec_node_type::get_gen_key2() const {
  return get_cstring( this->gen_key2 );
}

const char * smspec_node_type::get_wgname() const {
  return get_cstring( this->wgname );
}

const char * smspec_node_type::get_keyword() const {
  return get_cstring( this->keyword );
}

ecl_smspec_var_type smspec_node_type::get_var_type() const {
  return this->var_type;
}

int smspec_node_type::get_num() const {
  return this->num;
}

bool smspec_node_type::is_rate() const {
  return this->rate_variable;
}

bool smspec_node_type::is_total() const {
  return this->total_variable;
}

bool smspec_node_type::is_historical() const {
  return this->historical;
}

const char  * smspec_node_type::get_unit() const {
  return this->unit.c_str();
}

// Will be garbage for smspec_nodes which do not have i,j,k
const std::array<int,3>& smspec_node_type::get_ijk() const {
  return this->ijk;
}

// Will be NULL for smspec_nodes which are not related to an LGR.
const std::string& smspec_node_type::get_lgr_name() const {
  return this->lgr_name;
}

// Will be garbage for smspec_nodes which are not related to an LGR.
const std::array<int,3>&  smspec_node_type::get_lgr_ijk() const {
  return this->lgr_ijk;
}

/*
  Will return -1 for smspec_node variables which are not
  of type ECL_SMSPEC_REGION_2_REGION_VAR.
*/

int smspec_node_type::get_R1() const {
  if (var_type == ECL_SMSPEC_REGION_2_REGION_VAR) {
    int r1,r2;
    decode_R1R2( &r1 , &r2);
    return r1;
  } else
    return -1;
}

int smspec_node_type::get_R2() const {
  if (var_type == ECL_SMSPEC_REGION_2_REGION_VAR) {
    int r1,r2;
    decode_R1R2( &r1 , &r2);
    return r2;
  } else
    return -1;
}


bool smspec_node_type::need_nums() const {
  /*
    Check if this node needs the nums field; if at least one of the
    nodes need the NUMS field must be stored when writing a SMSPEC
    file.
  */
  {
    if (this->var_type == ECL_SMSPEC_COMPLETION_VAR      ||
        this->var_type == ECL_SMSPEC_SEGMENT_VAR         ||
        this->var_type == ECL_SMSPEC_REGION_VAR          ||
        this->var_type == ECL_SMSPEC_REGION_2_REGION_VAR ||
        this->var_type == ECL_SMSPEC_BLOCK_VAR           ||
        this->var_type == ECL_SMSPEC_AQUIFER_VAR)
      return true;
    else {
      if (this->num == SMSPEC_NUMS_INVALID)
        return false;
      else
        return true;
    }
  }
}


void smspec_node_type::fprintf__( FILE * stream) const {
  fprintf(stream, "KEYWORD: %s \n", this->keyword.c_str());
  fprintf(stream, "WGNAME : %s \n", this->wgname.c_str());
  fprintf(stream, "UNIT   : %s \n", this->unit.c_str());
  fprintf(stream, "TYPE   : %d \n", this->var_type);
  fprintf(stream, "NUM    : %d \n\n", this->num);
}

/*
  MISC variables are generally sorted to the end of the list,
  but some special case variables come at the very beginning.
*/

int smspec_node_type::cmp_MISC__( const smspec_node_type * node2) const {
  static const std::set<std::string> early_vars = {"TIME", "DAYS", "DAY", "MONTH", "YEAR", "YEARS"};

  if (smspec_node_type::equal_MISC( this, node2) )
    return 0;

  bool node1_early = !( early_vars.find(this->keyword) == early_vars.end() );
  bool node2_early = !( early_vars.find(node2->keyword) == early_vars.end() );


  if (node1_early && !node2_early)
    return -1;

  if (!node1_early && node2_early)
    return 1;

  return this->keyword.compare(node2->keyword);
}


int int_cmp(int v1, int v2) {
  if (v1 < v2)
    return -1;

  if (v1 > v2)
    return 1;

  return 0;
}

int smspec_node_type::cmp_LGRIJK__( const smspec_node_type * node2) const {
  int i_cmp = int_cmp( this->lgr_ijk[0] , node2->lgr_ijk[0]);
  if (i_cmp != 0)
    return i_cmp;

  int j_cmp = int_cmp(this->lgr_ijk[1] , node2->lgr_ijk[1]);
  if (j_cmp != 0)
    return j_cmp;

  return int_cmp( this->lgr_ijk[2] , node2->lgr_ijk[2]);
}

int smspec_node_type::cmp_KEYWORD_LGR_LGRIJK__( const smspec_node_type * node2) const {
  int keyword_cmp = this->keyword.compare(node2->keyword);
  if (keyword_cmp != 0)
    return keyword_cmp;

  int lgr_cmp = this->lgr_name.compare( node2->lgr_name );
  if (lgr_cmp != 0)
    return lgr_cmp;

  return smspec_node_type::cmp_LGRIJK( this, node2);
}

int smspec_node_type::cmp_KEYWORD_WGNAME_NUM__(const smspec_node_type * node2) const {
  int keyword_cmp = this->keyword.compare(node2->keyword);
  if (keyword_cmp != 0)
    return keyword_cmp;

  int wgname_cmp = this->wgname.compare(node2->wgname);
  if (wgname_cmp != 0)
    return wgname_cmp;

  return int_cmp( this->num , node2->num);
}

int smspec_node_type::cmp_KEYWORD_WGNAME_LGR__( const smspec_node_type * node2) const {
  int keyword_cmp = this->keyword.compare(node2->keyword);
  if (keyword_cmp != 0)
    return keyword_cmp;

  int wgname_cmp = this->wgname.compare(node2->wgname);
  if (wgname_cmp != 0)
    return wgname_cmp;

  return this->lgr_name.compare(node2->lgr_name);
}

int smspec_node_type::cmp_KEYWORD_WGNAME_LGR_LGRIJK__( const smspec_node_type * node2) const {
  int keyword_cmp = this->keyword.compare(node2->keyword);
  if (keyword_cmp != 0)
    return keyword_cmp;

  int wgname_cmp = this->wgname.compare(node2->wgname);
  if (wgname_cmp != 0)
    return wgname_cmp;

  int lgr_cmp = this->lgr_name.compare(node2->lgr_name);
  if (lgr_cmp != 0)
    return lgr_cmp;

  return smspec_node_type::cmp_LGRIJK( this, node2);
}

int smspec_node_type::cmp_KEYWORD_WGNAME__( const smspec_node_type * node2) const {
  int keyword_cmp = this->keyword.compare(node2->keyword);
  if (keyword_cmp != 0)
    return keyword_cmp;

  if (IS_DUMMY_WELL( this->wgname.c_str() )) {
    if (IS_DUMMY_WELL( node2->wgname.c_str() ))
      return 0;
    else
      return 1;
  }

  if (IS_DUMMY_WELL( node2->wgname.c_str() ))
    return -1;

  return this->wgname.compare(node2->wgname);
}


int smspec_node_type::cmp_KEYWORD_NUM__( const smspec_node_type * node2) const {
  int keyword_cmp = this->keyword.compare(node2->keyword);
  if (keyword_cmp != 0)
    return keyword_cmp;

  return int_cmp( this->num , node2->num);
}


int smspec_node_type::cmp_key1__( const smspec_node_type * node2) const {
  if (this->gen_key1.empty()) {
    if (node2->gen_key1.empty())
      return 0;
    else
      return -1;
  } else if (node2->gen_key1.empty()) {
    return 1;
  }
  return util_strcmp_int( this->gen_key1.c_str() , node2->gen_key1.c_str() );
}

int smspec_node_type::cmp__(const smspec_node_type * node2) const {
  /* 1: Start with special casing for the MISC variables. */
  if ((this->var_type == ECL_SMSPEC_MISC_VAR) || (node2->var_type == ECL_SMSPEC_MISC_VAR))
    return smspec_node_type::cmp_MISC( this , node2 );

  /* 2: Sort according to variable type */
  if (this->var_type < node2->var_type)
    return -1;

  if (this->var_type > node2->var_type)
    return 1;

  /* 3: Internal sort of variables of the same type. */
  switch (this->var_type) {

  case( ECL_SMSPEC_FIELD_VAR):
    return smspec_node_type::cmp_KEYWORD( this, node2);

  case( ECL_SMSPEC_WELL_VAR):
  case( ECL_SMSPEC_GROUP_VAR):
    return smspec_node_type::cmp_KEYWORD_WGNAME( this, node2);

  case( ECL_SMSPEC_BLOCK_VAR):
  case( ECL_SMSPEC_REGION_VAR):
  case( ECL_SMSPEC_REGION_2_REGION_VAR):
  case( ECL_SMSPEC_AQUIFER_VAR):
    return smspec_node_type::cmp_KEYWORD_NUM( this, node2);

  case( ECL_SMSPEC_COMPLETION_VAR):
  case( ECL_SMSPEC_SEGMENT_VAR):
    return smspec_node_type::cmp_KEYWORD_WGNAME_NUM( this, node2);

  case (ECL_SMSPEC_NETWORK_VAR):
    return smspec_node_type::cmp_key1( this, node2);

  case( ECL_SMSPEC_LOCAL_BLOCK_VAR):
    return smspec_node_type::cmp_KEYWORD_LGR_LGRIJK( this, node2);

  case( ECL_SMSPEC_LOCAL_WELL_VAR):
    return smspec_node_type::cmp_KEYWORD_WGNAME_LGR( this, node2);

  case( ECL_SMSPEC_LOCAL_COMPLETION_VAR):
    return smspec_node_type::cmp_KEYWORD_WGNAME_LGR_LGRIJK( this, node2);

  default:
    /* Should not really end up here. */
    return smspec_node_type::cmp_key1( this, node2);
  }
}


} // end namespace ecl

/**************************************  OLD API functions ***********************''''' */


void smspec_node_free( void * index ) {
  delete static_cast<ecl::smspec_node_type*>(index);
}

void smspec_node_free__( void * arg ) {
  smspec_node_free( arg );
}

float smspec_node_get_default( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_default();
}

int smspec_node_get_params_index( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_params_index();
}

void smspec_node_set_params_index( void * smspec_node , int params_index) {
  static_cast<ecl::smspec_node_type*>(smspec_node)->set_params_index( params_index );
}

const char * smspec_node_get_gen_key1( const void * smspec_node) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_gen_key1();
}

const char * smspec_node_get_gen_key2( const void * smspec_node) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_gen_key2();
}

const char * smspec_node_get_wgname( const void * smspec_node) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_wgname();
}

const char * smspec_node_get_keyword( const void * smspec_node) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_keyword( );
}

ecl_smspec_var_type smspec_node_get_var_type( const void * smspec_node) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_var_type();
}

int smspec_node_get_num( const void * smspec_node) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_num();
}

bool smspec_node_is_rate( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->is_rate();
}

bool smspec_node_is_total( const void * smspec_node ){
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->is_total();
}

bool smspec_node_is_historical( const void * smspec_node ){
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->is_historical();
}

const char  * smspec_node_get_unit( const void * smspec_node) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_unit();
}

// Will be garbage for smspec_nodes which do not have i,j,k
const int* smspec_node_get_ijk( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_ijk().data();
}

// Will be NULL for smspec_nodes which are not related to an LGR.
const char* smspec_node_get_lgr_name( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_lgr_name().c_str();
}


// Will be garbage for smspec_nodes which are not related to an LGR.
const int* smspec_node_get_lgr_ijk( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_lgr_ijk().data();
}

int smspec_node_get_R1( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_R1();
}


int smspec_node_get_R2( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->get_R2();
}

bool smspec_node_need_nums( const void * smspec_node ) {
  return static_cast<const ecl::smspec_node_type*>(smspec_node)->need_nums();
}

void smspec_node_fprintf( const void * smspec_node , FILE * stream) {
  static_cast<const ecl::smspec_node_type*>(smspec_node)->fprintf__(stream);
}

int smspec_node_cmp( const void * node1, const void * node2) {
  return ecl::smspec_node_type::cmp(static_cast<const ecl::smspec_node_type*>(node1), static_cast<const ecl::smspec_node_type*>(node2));
}

int smspec_node_cmp__( const void * node1, const void * node2) {
  return smspec_node_cmp(node1, node2);
}

/*
  This function should be removed from the public API.
*/
void smspec_node_init( void * smspec_node,
                       ecl_smspec_var_type var_type ,
                       const char * wgname  ,
                       const char * keyword ,
                       const char * unit    ,
                       const char * key_join_string ,
                       const int grid_dims[3] ,
                       int num) {

  static_cast<ecl::smspec_node_type*>(smspec_node)->init__( var_type,
                                              wgname,
                                              keyword,
                                              unit,
                                              key_join_string,
                       grid_dims,
                       num );

}


void * smspec_node_alloc( ecl_smspec_var_type var_type ,
                                      const char * wgname  ,
                                      const char * keyword ,
                                      const char * unit    ,
                                      const char * key_join_string ,
                                      const int grid_dims[3] ,
                                      int num , int param_index, float default_value) {
  ecl::smspec_node_type * node = NULL;
  try {
    node = new ecl::smspec_node_type(var_type, wgname, keyword, unit, key_join_string, grid_dims, num, param_index, default_value);
  }
  catch (const std::invalid_argument& e) {
    node = NULL;
  }
  return node;
}


void * smspec_node_alloc_lgr( ecl_smspec_var_type var_type ,
                                          const char * wgname  ,
                                          const char * keyword ,
                                          const char * unit    ,
                                          const char * lgr ,
                                          const char * key_join_string ,
                                          int   lgr_i, int lgr_j , int lgr_k,
                                          int param_index , float default_value) {

  return new ecl::smspec_node_type( var_type, wgname, keyword, unit, lgr, key_join_string, lgr_i, lgr_j, lgr_k, param_index, default_value);
}


void * smspec_node_alloc_copy( const void * node ) {
  if( !node ) return NULL;
  return static_cast<const ecl::smspec_node_type*>(node)->copy();
}


bool smspec_node_equal( const void * node1,  const void * node2) {
  return ecl::smspec_node_type::cmp( static_cast<const ecl::smspec_node_type*>(node1) , static_cast<const ecl::smspec_node_type*>(node2) ) == 0;
}


bool smspec_node_gt( const void * node1,  const void * node2) {
  return ecl::smspec_node_type::cmp( static_cast<const ecl::smspec_node_type*>(node1) , static_cast<const ecl::smspec_node_type*>(node2) ) > 0;
}

bool smspec_node_lt( const void * node1,  const void * node2) {
  return ecl::smspec_node_type::cmp( static_cast<const ecl::smspec_node_type*>(node1) , static_cast<const ecl::smspec_node_type*>(node2) ) < 0;
}

