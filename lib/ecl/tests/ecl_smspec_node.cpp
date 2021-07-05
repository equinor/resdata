/*
   Copyright (C) 2017  Equinor ASA, Norway.

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

#include <ert/ecl/smspec_node.hpp>

static void test_identify_rate_variable() {
    /*
   NOTE: Not all of the following vectors are actual Eclipse vectors,
   but they are patterns that are supposed to be recognized as (likely) rates.
*/
    test_assert_true(smspec_node_identify_rate("ROPR"));
    test_assert_true(smspec_node_identify_rate("WOIR"));
    test_assert_true(smspec_node_identify_rate("GOVPR"));
    test_assert_true(smspec_node_identify_rate("FOVIR"));
    test_assert_true(smspec_node_identify_rate("COFRL"));
    test_assert_true(smspec_node_identify_rate("LWOPP"));
    test_assert_true(smspec_node_identify_rate("LCOPI"));
    test_assert_true(smspec_node_identify_rate("WOMR"));

    test_assert_true(smspec_node_identify_rate("RGPR"));
    test_assert_true(smspec_node_identify_rate("WGIR"));
    test_assert_true(smspec_node_identify_rate("GGVPR"));
    test_assert_true(smspec_node_identify_rate("FGVIR"));
    test_assert_true(smspec_node_identify_rate("CGFRL"));
    test_assert_true(smspec_node_identify_rate("LWGPP"));
    test_assert_true(smspec_node_identify_rate("LCGPI"));
    test_assert_true(smspec_node_identify_rate("WGMR"));
    test_assert_true(smspec_node_identify_rate("GWGPR"));
    test_assert_true(smspec_node_identify_rate("FWGPR"));

    test_assert_true(smspec_node_identify_rate("RWPR"));
    test_assert_true(smspec_node_identify_rate("WWIR"));
    test_assert_true(smspec_node_identify_rate("GWVPR"));
    test_assert_true(smspec_node_identify_rate("FWVIR"));
    test_assert_true(smspec_node_identify_rate("CWFRL"));
    test_assert_true(smspec_node_identify_rate("LWWPP"));
    test_assert_true(smspec_node_identify_rate("LCWPI"));
    test_assert_true(smspec_node_identify_rate("WWMR"));

    test_assert_true(smspec_node_identify_rate("RLPR"));
    test_assert_true(smspec_node_identify_rate("WLFR"));
    test_assert_true(smspec_node_identify_rate("GVPR"));
    test_assert_true(smspec_node_identify_rate("FVIR"));
    test_assert_true(smspec_node_identify_rate("CVFR"));

    test_assert_true(smspec_node_identify_rate("RGLIR"));
    test_assert_true(smspec_node_identify_rate("WRGR"));
    test_assert_true(smspec_node_identify_rate("GEGR"));
    test_assert_true(smspec_node_identify_rate("FEXGR"));
    test_assert_true(smspec_node_identify_rate("CSGR"));
    test_assert_true(smspec_node_identify_rate("LWGSR"));
    test_assert_true(smspec_node_identify_rate("LCFGR"));
    test_assert_true(smspec_node_identify_rate("WGIMR"));
    test_assert_true(smspec_node_identify_rate("GGCR"));

    test_assert_true(smspec_node_identify_rate("RNPR"));
    test_assert_true(smspec_node_identify_rate("WNIR"));
    test_assert_true(smspec_node_identify_rate("GCPR"));
    test_assert_true(smspec_node_identify_rate("FCIR"));
    test_assert_true(smspec_node_identify_rate("CSIR"));
    test_assert_true(smspec_node_identify_rate("LWSPR"));
    test_assert_true(smspec_node_identify_rate("LCTIR"));
    test_assert_true(smspec_node_identify_rate("WTPR"));

    test_assert_true(smspec_node_identify_rate("RGOR"));
    test_assert_true(smspec_node_identify_rate("WWCT"));
    test_assert_true(smspec_node_identify_rate("GOGR"));
    test_assert_true(smspec_node_identify_rate("FWGR"));
    test_assert_true(smspec_node_identify_rate("CGLRL"));

    test_assert_true(smspec_node_identify_rate("SOFR"));
    test_assert_true(smspec_node_identify_rate("SGFR"));
    test_assert_true(smspec_node_identify_rate("SWFR"));
    test_assert_true(smspec_node_identify_rate("SCFR"));
    test_assert_true(smspec_node_identify_rate("SSFR"));
    test_assert_true(smspec_node_identify_rate("STFR"));
    test_assert_true(smspec_node_identify_rate("SCVPR"));
    test_assert_true(smspec_node_identify_rate("SWCT"));
    test_assert_true(smspec_node_identify_rate("SGOR"));
    test_assert_true(smspec_node_identify_rate("SOGR"));
    test_assert_true(smspec_node_identify_rate("SWGR"));

    test_assert_true(smspec_node_identify_rate("ROFR"));
    test_assert_true(smspec_node_identify_rate("RGFR-"));
    test_assert_true(smspec_node_identify_rate("RNLFR"));
    test_assert_true(smspec_node_identify_rate("RNLFR+"));

    test_assert_false(smspec_node_identify_rate(""));
    test_assert_false(smspec_node_identify_rate("HEI"));
    test_assert_false(smspec_node_identify_rate("GBHP"));
    test_assert_false(smspec_node_identify_rate("FOPT"));
    test_assert_false(smspec_node_identify_rate("CWIT"));
    test_assert_false(smspec_node_identify_rate("LWGPT"));
    test_assert_false(smspec_node_identify_rate("HELICOPR"));
    test_assert_false(smspec_node_identify_rate("SOPT"));
    test_assert_false(smspec_node_identify_rate("RFR"));
    test_assert_false(smspec_node_identify_rate("ROFT"));
}

static void test_identify_total_variable() {
    /*
   NOTE: Not all of the following vectors are actual Eclipse vectors,
   but they are patterns that are supposed to be recognized as (likely) totals.
*/
    test_assert_true(smspec_node_identify_total("ROPT", ECL_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WOIT", ECL_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GOVPT", ECL_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FOVIT", ECL_SMSPEC_FIELD_VAR));
    test_assert_true(
        smspec_node_identify_total("COMT", ECL_SMSPEC_COMPLETION_VAR));

    test_assert_true(smspec_node_identify_total("RGPT", ECL_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WGIT", ECL_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GGVPT", ECL_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FGVIT", ECL_SMSPEC_FIELD_VAR));
    test_assert_true(smspec_node_identify_total("WGMT", ECL_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GWGPT", ECL_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FWGPT", ECL_SMSPEC_FIELD_VAR));

    test_assert_true(smspec_node_identify_total("RWPT", ECL_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WWIT", ECL_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GWVPT", ECL_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FWVIT", ECL_SMSPEC_FIELD_VAR));
    test_assert_true(smspec_node_identify_total("WWMT", ECL_SMSPEC_WELL_VAR));

    test_assert_true(smspec_node_identify_total("RLPT", ECL_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("GVPT", ECL_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FVIT", ECL_SMSPEC_FIELD_VAR));

    test_assert_true(smspec_node_identify_total("WRGT", ECL_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GEGT", ECL_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FEXGT", ECL_SMSPEC_FIELD_VAR));
    test_assert_true(
        smspec_node_identify_total("CSGT", ECL_SMSPEC_COMPLETION_VAR));
    test_assert_true(
        smspec_node_identify_total("LWGST", ECL_SMSPEC_LOCAL_WELL_VAR));
    test_assert_true(
        smspec_node_identify_total("LCFGT", ECL_SMSPEC_LOCAL_COMPLETION_VAR));
    test_assert_true(smspec_node_identify_total("WGIMT", ECL_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GGCT", ECL_SMSPEC_GROUP_VAR));

    test_assert_true(smspec_node_identify_total("RNPT", ECL_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WNIT", ECL_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GCPT", ECL_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FCIT", ECL_SMSPEC_FIELD_VAR));
    test_assert_true(
        smspec_node_identify_total("CSIT", ECL_SMSPEC_COMPLETION_VAR));
    test_assert_true(
        smspec_node_identify_total("LWSPT", ECL_SMSPEC_LOCAL_WELL_VAR));
    test_assert_true(
        smspec_node_identify_total("LCTIT", ECL_SMSPEC_LOCAL_COMPLETION_VAR));
    test_assert_true(smspec_node_identify_total("WTPT", ECL_SMSPEC_WELL_VAR));

    test_assert_true(
        smspec_node_identify_total("SOFT", ECL_SMSPEC_SEGMENT_VAR));
    test_assert_true(
        smspec_node_identify_total("SGFT", ECL_SMSPEC_SEGMENT_VAR));
    test_assert_true(
        smspec_node_identify_total("SWFT", ECL_SMSPEC_SEGMENT_VAR));

    test_assert_true(
        smspec_node_identify_total("RGFT", ECL_SMSPEC_REGION_2_REGION_VAR));
    test_assert_true(
        smspec_node_identify_total("RWFT-", ECL_SMSPEC_REGION_2_REGION_VAR));
    test_assert_true(
        smspec_node_identify_total("RNLFT", ECL_SMSPEC_REGION_2_REGION_VAR));
    test_assert_true(
        smspec_node_identify_total("RNLFT+", ECL_SMSPEC_REGION_2_REGION_VAR));

    test_assert_false(smspec_node_identify_total("", ECL_SMSPEC_REGION_VAR));
    test_assert_false(smspec_node_identify_total("HEI", ECL_SMSPEC_WELL_VAR));
    test_assert_false(smspec_node_identify_total("GBHP", ECL_SMSPEC_GROUP_VAR));
    test_assert_false(smspec_node_identify_total("FOPR", ECL_SMSPEC_FIELD_VAR));
    test_assert_false(
        smspec_node_identify_total("CWIR", ECL_SMSPEC_COMPLETION_VAR));
    test_assert_false(
        smspec_node_identify_total("LWGPR", ECL_SMSPEC_LOCAL_WELL_VAR));
    test_assert_false(
        smspec_node_identify_total("CWCT", ECL_SMSPEC_LOCAL_COMPLETION_VAR));
    test_assert_false(
        smspec_node_identify_total("SGPT", ECL_SMSPEC_SEGMENT_VAR));
    test_assert_false(
        smspec_node_identify_total("RFT", ECL_SMSPEC_REGION_2_REGION_VAR));
    test_assert_false(
        smspec_node_identify_total("ROFR", ECL_SMSPEC_REGION_2_REGION_VAR));
}

void test_nums_default() {
    ecl::smspec_node field_node(0, "FOPT", "UNIT", 0);
    ecl::smspec_node group_node(0, "GOPR", "G1", "UNIT", 0, ":");
    ecl::smspec_node well_node(0, "WOPR", "W1", "UNIT", 0, ":");

    int default_nums = 0;
    /*
    The integer constant default nums corresponds to the symbol nums_unused
    in smspec_node.cpp. It is duplicated here to avoid exporting it - it should
    not really be a publically available symbol.
  */

    test_assert_int_equal(field_node.get_num(), default_nums);
    test_assert_int_equal(group_node.get_num(), default_nums);
    test_assert_int_equal(well_node.get_num(), default_nums);
}

void test_cmp_types() {
    const int dims[3] = {10, 10, 10};
    ecl::smspec_node field_node(0, "FOPT", "UNIT", 0);
    ecl::smspec_node region_node(0, "RPR", 10, "UNIT", dims, 0, ":");
    ecl::smspec_node group_node(0, "GOPR", "G1", "UNIT", 0, ":");
    ecl::smspec_node well_node(0, "WOPR", "W1", "UNIT", 0, ":");
    ecl::smspec_node block_node(0, "BPR", 10, "UNIT", dims, 0, ":");
    ecl::smspec_node aquifer_node(0, "AAQP", 10, "UNIT", dims, 0, ":");
    ecl::smspec_node segment_node(0, "SGOR", "W1", 10, "UNIT", dims, 0, ":");
    ecl::smspec_node misc_node1(0, "TIME", "UNIT", 0);
    ecl::smspec_node misc_node2(0, "TCPU", "UNIT", 0);

    test_assert_int_equal(field_node.cmp(field_node), 0);
    test_assert_int_equal(region_node.cmp(region_node), 0);
    test_assert_int_equal(well_node.cmp(well_node), 0);
    test_assert_int_equal(group_node.cmp(group_node), 0);
    test_assert_int_equal(block_node.cmp(block_node), 0);

    test_assert_true(misc_node1.cmp(field_node) < 0);
    test_assert_true(field_node.cmp(region_node) < 0);
    test_assert_true(region_node.cmp(group_node) < 0);
    test_assert_true(group_node.cmp(well_node) < 0);
    test_assert_true(well_node.cmp(segment_node) < 0);
    test_assert_true(segment_node.cmp(block_node) < 0);
    test_assert_true(block_node.cmp(aquifer_node) < 0);
    test_assert_true(aquifer_node.cmp(misc_node2) < 0);

    test_assert_true(field_node.cmp(misc_node1) > 0);
    test_assert_true(misc_node2.cmp(aquifer_node) > 0);
    test_assert_true(misc_node1.cmp(misc_node2) < 0);
    test_assert_true(misc_node2.cmp(misc_node1) > 0);
}

void test_cmp_well() {
    ecl::smspec_node well_node1_1(0, "WOPR", "W1", "UNIT", 10, ":");
    ecl::smspec_node well_node1_2(0, "WOPR", "W2", "UNIT", 10, ":");
    ecl::smspec_node well_node2_1(0, "WWCT", "W1", "UNIT", 10, ":");
    ecl::smspec_node well_node2_2(0, "WWWT", "W2", "UNIT", 10, ":");

    test_assert_int_equal(well_node1_1.cmp(well_node1_1), 0);
    test_assert_int_equal(well_node2_2.cmp(well_node2_2), 0);

    test_assert_true(well_node1_1.cmp(well_node1_2) < 0);
    test_assert_true(well_node1_1.cmp(well_node2_1) < 0);
    test_assert_true(well_node1_1.cmp(well_node2_2) < 0);

    test_assert_true(well_node1_2.cmp(well_node1_1) > 0);
    test_assert_true(well_node1_2.cmp(well_node2_1) < 0);
}

void test_cmp_region() {
    const int dims[3] = {10, 10, 10};
    ecl::smspec_node region_node1_1(0, "ROIP", 10, "UNIT", dims, 0, ":");
    ecl::smspec_node region_node1_2(0, "ROIP", 11, "UNIT", dims, 0, ":");
    ecl::smspec_node region_node2_1(0, "RPR", 10, "UNIT", dims, 0, ":");
    ecl::smspec_node region_node2_2(0, "RPR", 12, "UNIT", dims, 0, ":");

    test_assert_true(region_node1_1.cmp(region_node1_2) < 0);
    test_assert_true(region_node1_1.cmp(region_node2_1) < 0);
    test_assert_true(region_node1_1.cmp(region_node2_2) < 0);

    test_assert_true(region_node1_2.cmp(region_node1_1) > 0);
    test_assert_true(region_node1_2.cmp(region_node2_1) < 0);
}

int main(int argc, char **argv) {
    util_install_signals();
    test_cmp_types();
    test_cmp_well();
    test_cmp_region();
    test_identify_rate_variable();
    test_identify_total_variable();
    test_nums_default();
}
