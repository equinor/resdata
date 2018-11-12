/*
  Copyright (C) 2012  Statoil ASA, Norway.

  The file 'smspec_node.h' is part of ERT - Ensemble based Reservoir Tool.

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


#ifndef ERT_SMSPEC_NODE_HPP
#define ERT_SMSPEC_NODE_HPP

#include <stdbool.h>
#include <stdio.h>

#include <string>
#include <array>

#include <ert/util/type_macros.hpp>

#include <ert/ecl/smspec_node.h>

struct smspec_node_struct {
  private:

    std::string            wgname;
    std::string            keyword;            /* The value of the KEYWORDS vector for this elements. */
    std::string            unit;               /* The value of the UNITS vector for this elements. */
    int                    num;                /* The value of the NUMS vector for this elements - NB this will have the value SMSPEC_NUMS_INVALID if the smspec file does not have a NUMS   vector. */
    std::string            lgr_name;           /* The lgr name of the current variable - will be NULL for non-lgr variables. */
    std::array<int,3>      lgr_ijk;

    /*------------------------------------------- All members below this line are *derived* quantities. */

    std::string            gen_key1;           /* The main composite key, i.e. WWCT:OP3 for this element. */
    std::string            gen_key2;           /* Some of the ijk based elements will have both a xxx:i,j,k and a xxx:num key. Some of the region_2_region elements will have both a xxx:num and a xxx:r2-r2 key. Mostly NULL. */
    ecl_smspec_var_type    var_type;           /* The variable type */
    std::array<int,3>      ijk;                /* The ijk coordinates (NB: OFFSET 1) corresponding to the nums value - will be NULL if not relevant. */
    bool                   rate_variable;      /* Is this a rate variable (i.e. WOPR) or a state variable (i.e. BPR). Relevant when doing time interpolation. */
    bool                   total_variable;     /* Is this a total variable like WOPT? */
    bool                   historical;         /* Does the name end with 'H'? */
    int                    params_index;       /* The index of this variable (applies to all the vectors - in particular the PARAMS vectors of the summary files *.Snnnn / *.UNSMRY ). */
    float                  default_value;      /* Default value for this variable. */

    smspec_node_struct();

    void set_invalid_flags();
    void init_lgr( ecl_smspec_var_type var_type ,
                   const char * wgname  ,
                   const char * keyword ,
                   const char * unit    ,
                   const char * lgr ,
                   const char * key_join_string ,
                   int   lgr_i, int lgr_j , int lgr_k );
    void common_init( ecl_smspec_var_type var_type_ , const char * keyword , const std::string& unit );
    void set_num( const int grid_dims[3] , int num_);
    void set_keyword( const std::string& keyword_ );
    void set_flags();
    void init_num( ecl_smspec_var_type var_type_);
    void set_gen_keys( const char * key_join_string_);
    void decode_R1R2( int * r1 , int * r2)  const;
    void set_lgr_ijk( int lgr_i , int lgr_j , int lgr_k);


    int cmp__(const smspec_node_type * node2) const;
    int cmp_KEYWORD_WGNAME_NUM__(const smspec_node_type * node2) const;
    static int cmp_KEYWORD_WGNAME_NUM( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_KEYWORD_WGNAME_NUM__(node2);
    }
    int cmp_KEYWORD_WGNAME_LGR__( const smspec_node_type * node2) const;
    static int cmp_KEYWORD_WGNAME_LGR( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_KEYWORD_WGNAME_LGR__(node2);
    }
    int cmp_KEYWORD_WGNAME_LGR_LGRIJK__( const smspec_node_type * node2) const;
    static int cmp_KEYWORD_WGNAME_LGR_LGRIJK( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_KEYWORD_WGNAME_LGR_LGRIJK__(node2);
    }
    int cmp_KEYWORD_WGNAME__( const smspec_node_type * node2) const;
    static int cmp_KEYWORD_WGNAME( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_KEYWORD_WGNAME__(node2);
    }
    static bool equal_MISC( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->keyword == node2->keyword;
    }
    int cmp_MISC__( const smspec_node_type * node2) const;
    static int cmp_MISC( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_MISC__(node2);
    }
    int cmp_LGRIJK__( const smspec_node_type * node2) const;
    static int cmp_LGRIJK( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_LGRIJK__(node2);
    }
    int cmp_KEYWORD_LGR_LGRIJK__( const smspec_node_type * node2) const;
    static int cmp_KEYWORD_LGR_LGRIJK( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_KEYWORD_LGR_LGRIJK__(node2);
    }
    int cmp_KEYWORD_NUM__( const smspec_node_type * node2) const;
    static int cmp_KEYWORD_NUM( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_KEYWORD_NUM__(node2);
    }
    static int cmp_KEYWORD( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->keyword.compare(node2->keyword);
    }
    int cmp_key1__( const smspec_node_type * node2) const;
    static int cmp_key1( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp_key1__(node2);
    }

  public:

    UTIL_TYPE_ID_DECLARATION;

    smspec_node_struct(ecl_smspec_var_type var_type ,
                     const char * wgname  ,
                     const char * keyword ,
                     const char * unit    ,
                     const char * key_join_string ,
                     const int grid_dims[3] ,
                     int num , int param_index, float default_value);

    smspec_node_struct(ecl_smspec_var_type var_type ,
                     const char * wgname  ,
                     const char * keyword ,
                     const char * unit    ,
                     const char * lgr ,
                     const char * key_join_string ,
                     int   lgr_i, int lgr_j , int lgr_k,
                     int param_index , float default_value);

    bool init__( ecl_smspec_var_type var_type ,
                 const char * wgname  ,
                 const char * keyword ,
                 const char * unit    ,
                 const char * key_join_string ,
                 const int grid_dims[3] ,
                 int num);


    smspec_node_type * copy() const;

    static int cmp( const smspec_node_type * node1, const smspec_node_type * node2) {
      return node1->cmp__(node2);
    }

    int                   get_R1() const;
    int                   get_R2() const;
    const char          * get_gen_key1() const;
    const char          * get_gen_key2() const;
    ecl_smspec_var_type   get_var_type() const;
    int                   get_num() const;
    const char          * get_wgname() const;
    const char          * get_keyword() const;
    const char          * get_unit() const;
    bool                  is_rate() const;
    bool                  is_total() const;
    bool                  is_historical() const;
    bool                  need_nums() const;
    void                  fprintf__( FILE * stream) const;
    int                   get_params_index() const;
    void                  set_params_index( int params_index_);
    float                 get_default() const;
    const                 std::array<int,3>& get_ijk() const;
    const                 std::string& get_lgr_name() const;
    const                 std::array<int,3>&  get_lgr_ijk() const;

};


#endif
