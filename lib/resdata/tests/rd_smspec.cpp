#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_smspec.hpp>

void test_copy(const rd_smspec_type *smspec1) {
    rd_sum_type *rd_sum2 =
        rd_sum_alloc_writer("CASE", false, true, ":", 0, true, 100, 100, 100);
    const rd_smspec_type *smspec2 = rd_sum_get_smspec(rd_sum2);
    for (int i = 0; i < rd_smspec_num_nodes(smspec1); i++) {
        const rd::smspec_node &node =
            rd_smspec_iget_node_w_node_index(smspec1, i);
        rd_sum_add_smspec_node(rd_sum2, &node);
    }
    test_assert_true(rd_smspec_equal(smspec1, smspec2));
    rd_sum_free(rd_sum2);
}

int main(int argc, char **argv) {
    const char *case1 = argv[1];
    const char *case2 = argv[2];
    rd_smspec_type *smspec1 = rd_smspec_fread_alloc(case1, ":", false);
    rd_smspec_type *smspec2 = rd_smspec_fread_alloc(case2, ":", false);

    test_assert_true(rd_smspec_equal(smspec2, smspec2));
    test_assert_true(rd_smspec_equal(smspec1, smspec1));

    test_assert_false(rd_smspec_equal(smspec1, smspec2));
    test_assert_false(rd_smspec_equal(smspec2, smspec1));

    rd_smspec_free(smspec1);
    rd_smspec_free(smspec2);
}
