#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/test_util.hpp>

#include <resdata/smspec_node.hpp>

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

    test_assert_true(smspec_node_identify_rate("NJOPR"));
    test_assert_true(smspec_node_identify_rate("NJWPR"));
    test_assert_true(smspec_node_identify_rate("NJGFR"));
    test_assert_true(smspec_node_identify_rate("NPGFR"));
    test_assert_true(smspec_node_identify_rate("NPOFR"));

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
    test_assert_true(smspec_node_identify_total("ROPT", RD_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WOIT", RD_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GOVPT", RD_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FOVIT", RD_SMSPEC_FIELD_VAR));
    test_assert_true(
        smspec_node_identify_total("COMT", RD_SMSPEC_COMPLETION_VAR));

    test_assert_true(smspec_node_identify_total("RGPT", RD_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WGIT", RD_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GGVPT", RD_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FGVIT", RD_SMSPEC_FIELD_VAR));
    test_assert_true(smspec_node_identify_total("WGMT", RD_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GWGPT", RD_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FWGPT", RD_SMSPEC_FIELD_VAR));

    test_assert_true(smspec_node_identify_total("RWPT", RD_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WWIT", RD_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GWVPT", RD_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FWVIT", RD_SMSPEC_FIELD_VAR));
    test_assert_true(smspec_node_identify_total("WWMT", RD_SMSPEC_WELL_VAR));

    test_assert_true(smspec_node_identify_total("RLPT", RD_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("GVPT", RD_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FVIT", RD_SMSPEC_FIELD_VAR));

    test_assert_true(smspec_node_identify_total("WRGT", RD_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GEGT", RD_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FEXGT", RD_SMSPEC_FIELD_VAR));
    test_assert_true(
        smspec_node_identify_total("CSGT", RD_SMSPEC_COMPLETION_VAR));
    test_assert_true(
        smspec_node_identify_total("LWGST", RD_SMSPEC_LOCAL_WELL_VAR));
    test_assert_true(
        smspec_node_identify_total("LCFGT", RD_SMSPEC_LOCAL_COMPLETION_VAR));
    test_assert_true(smspec_node_identify_total("WGIMT", RD_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GGCT", RD_SMSPEC_GROUP_VAR));

    test_assert_true(smspec_node_identify_total("RNPT", RD_SMSPEC_REGION_VAR));
    test_assert_true(smspec_node_identify_total("WNIT", RD_SMSPEC_WELL_VAR));
    test_assert_true(smspec_node_identify_total("GCPT", RD_SMSPEC_GROUP_VAR));
    test_assert_true(smspec_node_identify_total("FCIT", RD_SMSPEC_FIELD_VAR));
    test_assert_true(
        smspec_node_identify_total("CSIT", RD_SMSPEC_COMPLETION_VAR));
    test_assert_true(
        smspec_node_identify_total("LWSPT", RD_SMSPEC_LOCAL_WELL_VAR));
    test_assert_true(
        smspec_node_identify_total("LCTIT", RD_SMSPEC_LOCAL_COMPLETION_VAR));
    test_assert_true(smspec_node_identify_total("WTPT", RD_SMSPEC_WELL_VAR));

    test_assert_true(smspec_node_identify_total("SOFT", RD_SMSPEC_SEGMENT_VAR));
    test_assert_true(smspec_node_identify_total("SGFT", RD_SMSPEC_SEGMENT_VAR));
    test_assert_true(smspec_node_identify_total("SWFT", RD_SMSPEC_SEGMENT_VAR));

    test_assert_true(
        smspec_node_identify_total("RGFT", RD_SMSPEC_REGION_2_REGION_VAR));
    test_assert_true(
        smspec_node_identify_total("RWFT-", RD_SMSPEC_REGION_2_REGION_VAR));
    test_assert_true(
        smspec_node_identify_total("RNLFT", RD_SMSPEC_REGION_2_REGION_VAR));
    test_assert_true(
        smspec_node_identify_total("RNLFT+", RD_SMSPEC_REGION_2_REGION_VAR));

    test_assert_false(smspec_node_identify_total("", RD_SMSPEC_REGION_VAR));
    test_assert_false(smspec_node_identify_total("HEI", RD_SMSPEC_WELL_VAR));
    test_assert_false(smspec_node_identify_total("GBHP", RD_SMSPEC_GROUP_VAR));
    test_assert_false(smspec_node_identify_total("FOPR", RD_SMSPEC_FIELD_VAR));
    test_assert_false(
        smspec_node_identify_total("CWIR", RD_SMSPEC_COMPLETION_VAR));
    test_assert_false(
        smspec_node_identify_total("LWGPR", RD_SMSPEC_LOCAL_WELL_VAR));
    test_assert_false(
        smspec_node_identify_total("CWCT", RD_SMSPEC_LOCAL_COMPLETION_VAR));
    test_assert_false(
        smspec_node_identify_total("SGPT", RD_SMSPEC_SEGMENT_VAR));
    test_assert_false(
        smspec_node_identify_total("RFT", RD_SMSPEC_REGION_2_REGION_VAR));
    test_assert_false(
        smspec_node_identify_total("ROFR", RD_SMSPEC_REGION_2_REGION_VAR));
}

void test_nums_default() {
    rd::smspec_node field_node(0, "FOPT", "UNIT", 0);
    rd::smspec_node group_node(0, "GOPR", "G1", "UNIT", 0, ":");
    rd::smspec_node well_node(0, "WOPR", "W1", "UNIT", 0, ":");

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
    rd::smspec_node field_node(0, "FOPT", "UNIT", 0);
    rd::smspec_node region_node(0, "RPR", 10, "UNIT", dims, 0, ":");
    rd::smspec_node group_node(0, "GOPR", "G1", "UNIT", 0, ":");
    rd::smspec_node well_node(0, "WOPR", "W1", "UNIT", 0, ":");
    rd::smspec_node block_node(0, "BPR", 10, "UNIT", dims, 0, ":");
    rd::smspec_node aquifer_node(0, "AAQP", 10, "UNIT", dims, 0, ":");
    rd::smspec_node segment_node(0, "SGOR", "W1", 10, "UNIT", dims, 0, ":");
    rd::smspec_node misc_node1(0, "TIME", "UNIT", 0);
    rd::smspec_node misc_node2(0, "TCPU", "UNIT", 0);

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
    rd::smspec_node well_node1_1(0, "WOPR", "W1", "UNIT", 10, ":");
    rd::smspec_node well_node1_2(0, "WOPR", "W2", "UNIT", 10, ":");
    rd::smspec_node well_node2_1(0, "WWCT", "W1", "UNIT", 10, ":");
    rd::smspec_node well_node2_2(0, "WWWT", "W2", "UNIT", 10, ":");

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
    rd::smspec_node region_node1_1(0, "ROIP", 10, "UNIT", dims, 0, ":");
    rd::smspec_node region_node1_2(0, "ROIP", 11, "UNIT", dims, 0, ":");
    rd::smspec_node region_node2_1(0, "RPR", 10, "UNIT", dims, 0, ":");
    rd::smspec_node region_node2_2(0, "RPR", 12, "UNIT", dims, 0, ":");

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
