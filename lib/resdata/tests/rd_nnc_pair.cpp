#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_nnc_geometry.hpp>

void test_pair(int grid1_1, int grid1_2, int grid2_1, int grid2_2,
               bool expected) {
    rd_nnc_pair_type pair1 = {grid1_1, grid1_2, 0, 0};
    rd_nnc_pair_type pair2 = {grid2_1, grid2_2, 0, 0};

    test_assert_bool_equal(rd_nnc_geometry_same_kw(&pair1, &pair2), expected);
}

int main(int argc, char **argv) {
    test_pair(1, 1, 1, 1, true);
    test_pair(1, 3, 1, 3, true);
    test_pair(1, 1, 3, 3, false);
    test_pair(1, 3, 3, 1, false);
}
