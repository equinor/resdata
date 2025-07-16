#include <stdlib.h>
#include <stdbool.h>
#include <string>
#include <tuple>
#include <vector>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/smspec_node.hpp>

static void test_identify_rate_variable() {
    /*
   NOTE: Not all of the following vectors are actual Eclipse vectors,
   but they are patterns that are supposed to be recognized as (likely) rates.
*/
    std::vector<const char *> rateIdentifiers = {
        "ROPR",  "WOIR",  "GOVPR", "FOVIR",  "COFRL", "LWOPP", "LCOPI", "WOMR",
        "RGPR",  "WGIR",  "GGVPR", "FGVIR",  "CGFRL", "LWGPP", "LCGPI", "WGMR",
        "GWGPR", "FWGPR", "RWPR",  "WWIR",   "GWVPR", "FWVIR", "CWFRL", "LWWPP",
        "LCWPI", "WWMR",  "RLPR",  "WLFR",   "GVPR",  "FVIR",  "CVFR",  "RGLIR",
        "WRGR",  "GEGR",  "FEXGR", "CSGR",   "LWGSR", "LCFGR", "WGIMR", "GGCR",
        "RNPR",  "WNIR",  "GCPR",  "FCIR",   "CSIR",  "LWSPR", "LCTIR", "WTPR",
        "RGOR",  "WWCT",  "GOGR",  "FWGR",   "CGLRL", "SOFR",  "SGFR",  "SWFR",
        "SCFR",  "SSFR",  "STFR",  "SCVPR",  "SWCT",  "SGOR",  "SOGR",  "SWGR",
        "ROFR",  "RGFR-", "RNLFR", "RNLFR+", "NJOPR", "NJWPR", "NJGFR", "NPGFR",
        "NPOFR"};

    for (const auto &identifier : rateIdentifiers) {
        test_assert_true(smspec_node_identify_rate(identifier));
    }

    std::vector<const char *> invalidRateIdentifiers = {
        "",      "HEI",      "GBHP", "FOPT", "CWIT",
        "LWGPT", "HELICOPR", "SOPT", "RFR",  "ROFT"};

    for (const auto &identifier : invalidRateIdentifiers) {
        test_assert_false(smspec_node_identify_rate(identifier));
    }
}

static void test_identify_total_variable() {
    /*
   NOTE: Not all of the following vectors are actual Eclipse vectors,
   but they are patterns that are supposed to be recognized as (likely) totals.
*/
    std::vector<std::tuple<bool, const char *, rd_smspec_var_type>>
        totalIdentifiers = {{true, "ROPT", RD_SMSPEC_REGION_VAR},
                            {true, "WOIT", RD_SMSPEC_WELL_VAR},
                            {true, "GOVPT", RD_SMSPEC_GROUP_VAR},
                            {true, "FOVIT", RD_SMSPEC_FIELD_VAR},
                            {true, "COMT", RD_SMSPEC_COMPLETION_VAR},
                            {true, "RGPT", RD_SMSPEC_REGION_VAR},
                            {true, "WGIT", RD_SMSPEC_WELL_VAR},
                            {true, "FGMIT", RD_SMSPEC_FIELD_VAR},
                            {true, "GGVPT", RD_SMSPEC_GROUP_VAR},
                            {true, "FGVIT", RD_SMSPEC_FIELD_VAR},
                            {true, "WGMT", RD_SMSPEC_WELL_VAR},
                            {true, "GWGPT", RD_SMSPEC_GROUP_VAR},
                            {true, "FWGPT", RD_SMSPEC_FIELD_VAR},
                            {true, "RWPT", RD_SMSPEC_REGION_VAR},
                            {true, "WWIT", RD_SMSPEC_WELL_VAR},
                            {true, "GWVPT", RD_SMSPEC_GROUP_VAR},
                            {true, "FWVIT", RD_SMSPEC_FIELD_VAR},
                            {true, "WWMT", RD_SMSPEC_WELL_VAR},
                            {true, "RLPT", RD_SMSPEC_REGION_VAR},
                            {true, "GVPT", RD_SMSPEC_GROUP_VAR},
                            {true, "FVIT", RD_SMSPEC_FIELD_VAR},
                            {true, "WRGT", RD_SMSPEC_WELL_VAR},
                            {true, "GEGT", RD_SMSPEC_GROUP_VAR},
                            {true, "FEXGT", RD_SMSPEC_FIELD_VAR},
                            {true, "CSGT", RD_SMSPEC_COMPLETION_VAR},
                            {true, "LWGST", RD_SMSPEC_LOCAL_WELL_VAR},
                            {true, "LCFGT", RD_SMSPEC_LOCAL_COMPLETION_VAR},
                            {true, "WGIMT", RD_SMSPEC_WELL_VAR},
                            {true, "GGCT", RD_SMSPEC_GROUP_VAR},
                            {true, "RNPT", RD_SMSPEC_REGION_VAR},
                            {true, "WNIT", RD_SMSPEC_WELL_VAR},
                            {true, "GCPT", RD_SMSPEC_GROUP_VAR},
                            {true, "FCIT", RD_SMSPEC_FIELD_VAR},
                            {true, "CSIT", RD_SMSPEC_COMPLETION_VAR},
                            {true, "LWSPT", RD_SMSPEC_LOCAL_WELL_VAR},
                            {true, "LCTIT", RD_SMSPEC_LOCAL_COMPLETION_VAR},
                            {true, "WTPT", RD_SMSPEC_WELL_VAR},
                            {true, "SOFT", RD_SMSPEC_SEGMENT_VAR},
                            {true, "SGFT", RD_SMSPEC_SEGMENT_VAR},
                            {true, "SWFT", RD_SMSPEC_SEGMENT_VAR},
                            {true, "RGFT", RD_SMSPEC_REGION_2_REGION_VAR},
                            {true, "RWFT-", RD_SMSPEC_REGION_2_REGION_VAR},
                            {true, "RNLFT", RD_SMSPEC_REGION_2_REGION_VAR},
                            {true, "RNLFT+", RD_SMSPEC_REGION_2_REGION_VAR},
                            {false, "", RD_SMSPEC_REGION_VAR},
                            {false, "HEI", RD_SMSPEC_WELL_VAR},
                            {false, "GBHP", RD_SMSPEC_GROUP_VAR},
                            {false, "FOPR", RD_SMSPEC_FIELD_VAR},
                            {false, "CWIR", RD_SMSPEC_COMPLETION_VAR},
                            {false, "LWGPR", RD_SMSPEC_LOCAL_WELL_VAR},
                            {false, "CWCT", RD_SMSPEC_LOCAL_COMPLETION_VAR},
                            {false, "SGPT", RD_SMSPEC_SEGMENT_VAR},
                            {false, "RFT", RD_SMSPEC_REGION_2_REGION_VAR},
                            {false, "ROFR", RD_SMSPEC_REGION_2_REGION_VAR}};

    for (const auto &identifier : totalIdentifiers) {
        bool expected = std::get<0>(identifier);
        const char *keyword = std::get<1>(identifier);
        rd_smspec_var_type type = std::get<2>(identifier);
        test_assert_true(smspec_node_identify_total(keyword, type) == expected);
    }
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

void test_history_vectors() {
    const int dims[3] = {10, 10, 10};

    std::vector<std::tuple<bool, const char *, const char *, int, const int *,
                           const char *>>
        allVectors = {// historical vectors
                      {true, "FOPTH", "", 0, nullptr, nullptr},
                      {true, "GOPTH", "G1", 0, nullptr, ":"},
                      {true, "WOPTH", "W1", 0, nullptr, ":"},
                      {true, "FGPTH", "", 0, nullptr, nullptr},
                      {true, "GGPTH", "G1", 0, nullptr, ":"},
                      {true, "WGPTH", "W1", 0, nullptr, ":"},
                      {true, "FWIRH", "", 0, nullptr, nullptr},
                      {true, "GWIRH", "G1", 0, nullptr, ":"},
                      {true, "WWIRH", "W1", 0, nullptr, ":"},
                      {true, "FWPRH", "", 0, nullptr, nullptr},
                      {true, "GWPRH", "G1", 0, nullptr, ":"},
                      {true, "WWPRH", "W1", 0, nullptr, ":"},
                      // correct type, but not history vectors
                      {false, "FOPT", "", 0, nullptr, nullptr},
                      {false, "GOPT", "G1", 0, nullptr, ":"},
                      {false, "WOPT", "W1", 0, nullptr, ":"},
                      // not historical vectors
                      {false, "AAQENTH", "", 10, dims, ":"},
                      {false, "BKRWOH", "", 10, dims, ":"},
                      {false, "BSOWNH", "", 10, dims, ":"},
                      {false, "CRREXCH", "C1", 10, dims, ":"},
                      {false, "MONTH", "", 0, nullptr, nullptr},
                      {false, "RPRH", "", 10, dims, ":"},
                      {false, "SPRDH", "W1", 10, dims, ":"},
                      {false, "TCPUH", "", 0, nullptr, nullptr},
                      {false, "TCPUTSH", "", 0, nullptr, nullptr}};

    for (const auto &args : allVectors) {
        rd::smspec_node node = rd::smspec_node(
            0, std::get<1>(args), std::get<2>(args), std::get<3>(args), "UNIT",
            std::get<4>(args), 0, std::get<5>(args));

        test_assert_true(node.is_historical() == std::get<0>(args));
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_cmp_types();
    test_cmp_well();
    test_cmp_region();
    test_identify_rate_variable();
    test_identify_total_variable();
    test_nums_default();
    test_history_vectors();
}
