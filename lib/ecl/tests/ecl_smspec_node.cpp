/*
   Copyright (C) 2017  Statoil ASA, Norway.

   This file is part of ERT - Ensemble based Reservoir Tool.

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

#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/test_util.hpp>

#include <ert/ecl/smspec_node.h>


static void test_identify_rate_variable() {
  const char* rate_vars[] = {
    "WOPR" , "GGPR" , "FWPR" , "WLPR" , "WOIR" , "FGIR" , "GWIR" ,
    "WLIR" , "GGOR" , "FWCT" , "SOFR" , "SGFR" , "SWFR" ,
  };

  const auto n_var = sizeof(rate_vars) / sizeof(rate_vars[0]);

  for (auto var = rate_vars, end = rate_vars + n_var; var != end; ++var)
    test_assert_true(smspec_node_identify_rate(*var));

  test_assert_false(smspec_node_identify_rate("SPR"));
  test_assert_false(smspec_node_identify_rate("SOFT"));
  test_assert_false(smspec_node_identify_rate("SGFT"));
  test_assert_false(smspec_node_identify_rate("SWFT"));
}

static void test_identify_total_variable() {
  test_assert_true(smspec_node_identify_total("WOPT", ECL_SMSPEC_WELL_VAR));
  test_assert_true(smspec_node_identify_total("GGPT", ECL_SMSPEC_GROUP_VAR));
  test_assert_true(smspec_node_identify_total("FWPT", ECL_SMSPEC_FIELD_VAR));
  test_assert_true(smspec_node_identify_total("RGIT", ECL_SMSPEC_REGION_VAR));
  test_assert_true(smspec_node_identify_total("CWIT", ECL_SMSPEC_COMPLETION_VAR));

  test_assert_true(smspec_node_identify_total("WOPTF", ECL_SMSPEC_WELL_VAR));
  test_assert_true(smspec_node_identify_total("GOPTS", ECL_SMSPEC_GROUP_VAR));
  test_assert_true(smspec_node_identify_total("FOIT", ECL_SMSPEC_FIELD_VAR));
  test_assert_true(smspec_node_identify_total("ROVPT", ECL_SMSPEC_REGION_VAR));
  test_assert_true(smspec_node_identify_total("COVIT", ECL_SMSPEC_COMPLETION_VAR));

  test_assert_true(smspec_node_identify_total("WMWT", ECL_SMSPEC_WELL_VAR));
  test_assert_true(smspec_node_identify_total("GWVPT", ECL_SMSPEC_GROUP_VAR));
  test_assert_true(smspec_node_identify_total("FWVIT", ECL_SMSPEC_FIELD_VAR));
  test_assert_true(smspec_node_identify_total("RGMT", ECL_SMSPEC_REGION_VAR));
  test_assert_true(smspec_node_identify_total("CGPTF", ECL_SMSPEC_COMPLETION_VAR));

  test_assert_true(smspec_node_identify_total("WSGT", ECL_SMSPEC_WELL_VAR));
  test_assert_true(smspec_node_identify_total("GGST", ECL_SMSPEC_GROUP_VAR));
  test_assert_true(smspec_node_identify_total("FFGT", ECL_SMSPEC_FIELD_VAR));
  test_assert_true(smspec_node_identify_total("RGCT", ECL_SMSPEC_REGION_VAR));
  test_assert_true(smspec_node_identify_total("CGIMT", ECL_SMSPEC_COMPLETION_VAR));

  test_assert_true(smspec_node_identify_total("WWGPT", ECL_SMSPEC_WELL_VAR));
  test_assert_true(smspec_node_identify_total("GWGIT", ECL_SMSPEC_GROUP_VAR));
  test_assert_true(smspec_node_identify_total("FEGT", ECL_SMSPEC_FIELD_VAR));
  test_assert_true(smspec_node_identify_total("REXGT", ECL_SMSPEC_REGION_VAR));
  test_assert_true(smspec_node_identify_total("CGVPT", ECL_SMSPEC_COMPLETION_VAR));

  test_assert_true(smspec_node_identify_total("WGVIT", ECL_SMSPEC_WELL_VAR));
  test_assert_true(smspec_node_identify_total("GLPT", ECL_SMSPEC_GROUP_VAR));
  test_assert_true(smspec_node_identify_total("FVPT", ECL_SMSPEC_FIELD_VAR));
  test_assert_true(smspec_node_identify_total("RVIT", ECL_SMSPEC_REGION_VAR));
  test_assert_true(smspec_node_identify_total("CNPT", ECL_SMSPEC_COMPLETION_VAR));

  test_assert_true(smspec_node_identify_total("WNIT", ECL_SMSPEC_WELL_VAR));
  test_assert_true(smspec_node_identify_total("GCPT", ECL_SMSPEC_GROUP_VAR));
  test_assert_true(smspec_node_identify_total("FCIT", ECL_SMSPEC_FIELD_VAR));

  test_assert_true(smspec_node_identify_total("SOFT", ECL_SMSPEC_SEGMENT_VAR));
  test_assert_true(smspec_node_identify_total("SGFT", ECL_SMSPEC_SEGMENT_VAR));
  test_assert_true(smspec_node_identify_total("SWFT", ECL_SMSPEC_SEGMENT_VAR));

  test_assert_false(smspec_node_identify_total("SOPT", ECL_SMSPEC_SEGMENT_VAR));
  test_assert_false(smspec_node_identify_total("HEI!", ECL_SMSPEC_SEGMENT_VAR));
  test_assert_false(smspec_node_identify_total("xXx", ECL_SMSPEC_SEGMENT_VAR));
  test_assert_false(smspec_node_identify_total("SPR", ECL_SMSPEC_SEGMENT_VAR));
}

void test_cmp_types() {
  const int dims[3] = {10,10,10};
  smspec_node_type * field_node = smspec_node_alloc( ECL_SMSPEC_FIELD_VAR , NULL , "FOPT" , "UNIT" , ":" , dims , 0 , 0 , 0 );
  smspec_node_type * region_node = smspec_node_alloc( ECL_SMSPEC_REGION_VAR , NULL , "RPR" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * group_node = smspec_node_alloc( ECL_SMSPEC_GROUP_VAR , "G1" , "GOPR" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * well_node = smspec_node_alloc( ECL_SMSPEC_WELL_VAR , "W1" , "WOPR" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * block_node = smspec_node_alloc( ECL_SMSPEC_BLOCK_VAR , NULL , "BPR" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * aquifer_node = smspec_node_alloc( ECL_SMSPEC_AQUIFER_VAR , NULL , "AAQP" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * segment_node = smspec_node_alloc( ECL_SMSPEC_SEGMENT_VAR , "W1" , "SGOR" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * misc_node1 = smspec_node_alloc( ECL_SMSPEC_MISC_VAR , NULL , "TIME" , "UNIT" , ":", dims, 10 , 0, 0);
  smspec_node_type * misc_node2 = smspec_node_alloc( ECL_SMSPEC_MISC_VAR , NULL , "TCPU" , "UNIT" , ":", dims, 10 , 0, 0);

  test_assert_int_equal( smspec_node_cmp( field_node , field_node ), 0);
  test_assert_int_equal( smspec_node_cmp( region_node , region_node ), 0);
  test_assert_int_equal( smspec_node_cmp( well_node , well_node ), 0);
  test_assert_int_equal( smspec_node_cmp( group_node , group_node ), 0);
  test_assert_int_equal( smspec_node_cmp( block_node , block_node ), 0);

  test_assert_true( smspec_node_cmp( misc_node1 , field_node ) < 0 );
  test_assert_true( smspec_node_cmp( field_node , region_node ) < 0 );
  test_assert_true( smspec_node_cmp( region_node , group_node ) < 0 );
  test_assert_true( smspec_node_cmp( group_node , well_node ) < 0 );
  test_assert_true( smspec_node_cmp( well_node , segment_node ) < 0 );
  test_assert_true( smspec_node_cmp( segment_node , block_node ) < 0 );
  test_assert_true( smspec_node_cmp( block_node , aquifer_node) < 0 );
  test_assert_true( smspec_node_cmp( aquifer_node , misc_node2 ) < 0 );

  test_assert_true( smspec_node_cmp( field_node, misc_node1) > 0 );
  test_assert_true( smspec_node_cmp( misc_node2, aquifer_node) > 0 );
  test_assert_true( smspec_node_cmp( misc_node1, misc_node2) < 0 );
  test_assert_true( smspec_node_cmp( misc_node2, misc_node1) > 0 );

  smspec_node_free( segment_node );
  smspec_node_free( aquifer_node );
  smspec_node_free( block_node );
  smspec_node_free( group_node );
  smspec_node_free( well_node );
  smspec_node_free( region_node );
  smspec_node_free( field_node );
  smspec_node_free( misc_node1 );
  smspec_node_free( misc_node2 );
}

void test_cmp_well() {
  const int dims[3] = {10,10,10};
  smspec_node_type * well_node1_1 = smspec_node_alloc( ECL_SMSPEC_WELL_VAR , "W1" , "WOPR" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * well_node1_2 = smspec_node_alloc( ECL_SMSPEC_WELL_VAR , "W2" , "WOPR" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * well_node2_1 = smspec_node_alloc( ECL_SMSPEC_WELL_VAR , "W1" , "WWCT" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * well_node2_2 = smspec_node_alloc( ECL_SMSPEC_WELL_VAR , "W2" , "WWWT" , "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * well_node_dummy = smspec_node_alloc( ECL_SMSPEC_WELL_VAR , DUMMY_WELL , "WOPR" , "UNIT" , ":" , dims , 10 , 0 , 0 );

  test_assert_NULL( well_node_dummy);
  test_assert_int_equal( smspec_node_cmp( well_node1_1 , well_node1_1 ), 0);
  test_assert_int_equal( smspec_node_cmp( well_node2_2 , well_node2_2 ), 0);

  test_assert_true( smspec_node_cmp( well_node1_1, well_node1_2) < 0 );
  test_assert_true( smspec_node_cmp( well_node1_1, well_node2_1) < 0 );
  test_assert_true( smspec_node_cmp( well_node1_1, well_node2_2) < 0 );

  test_assert_true( smspec_node_cmp( well_node1_2, well_node1_1) > 0 );
  test_assert_true( smspec_node_cmp( well_node1_2, well_node2_1) < 0 );

  smspec_node_free( well_node1_1 );
  smspec_node_free( well_node2_1 );
  smspec_node_free( well_node1_2 );
  smspec_node_free( well_node2_2 );
}



void test_cmp_region() {
  const int dims[3] = {10,10,10};
  smspec_node_type * region_node1_1 = smspec_node_alloc( ECL_SMSPEC_REGION_VAR , NULL , "ROIP" ,  "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * region_node1_2 = smspec_node_alloc( ECL_SMSPEC_REGION_VAR , NULL , "ROIP" ,  "UNIT" , ":" , dims , 11 , 0 , 0 );
  smspec_node_type * region_node2_1 = smspec_node_alloc( ECL_SMSPEC_REGION_VAR , NULL , "RPR" ,   "UNIT" , ":" , dims , 10 , 0 , 0 );
  smspec_node_type * region_node2_2 = smspec_node_alloc( ECL_SMSPEC_REGION_VAR , NULL , "RPR" ,   "UNIT" , ":" , dims , 12 , 0 , 0 );

  test_assert_true( smspec_node_cmp( region_node1_1, region_node1_2) < 0 );
  test_assert_true( smspec_node_cmp( region_node1_1, region_node2_1) < 0 );
  test_assert_true( smspec_node_cmp( region_node1_1, region_node2_2) < 0 );

  test_assert_true( smspec_node_cmp( region_node1_2, region_node1_1) > 0 );
  test_assert_true( smspec_node_cmp( region_node1_2, region_node2_1) < 0 );

  smspec_node_free( region_node1_1 );
  smspec_node_free( region_node2_1 );
  smspec_node_free( region_node1_2 );
  smspec_node_free( region_node2_2 );
}


int main(int argc, char ** argv) {
  util_install_signals();
  test_cmp_types();
  test_cmp_well();
  test_cmp_region( );
  test_identify_rate_variable();
  test_identify_total_variable();
}
