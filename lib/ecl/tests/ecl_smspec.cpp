#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/ecl/ecl_sum.hpp>
#include <ert/ecl/ecl_smspec.hpp>

void test_copy(const ecl_smspec_type *smspec1) {
    ecl_sum_type *ecl_sum2 =
        ecl_sum_alloc_writer("CASE", false, true, ":", 0, true, 100, 100, 100);
    const ecl_smspec_type *smspec2 = ecl_sum_get_smspec(ecl_sum2);
    for (int i = 0; i < ecl_smspec_num_nodes(smspec1); i++) {
        const ecl::smspec_node &node =
            ecl_smspec_iget_node_w_node_index(smspec1, i);
        ecl_sum_add_smspec_node(ecl_sum2, &node);
    }
    test_assert_true(ecl_smspec_equal(smspec1, smspec2));
    ecl_sum_free(ecl_sum2);
}

int main(int argc, char **argv) {
    const char *case1 = argv[1];
    const char *case2 = argv[2];
    ecl_smspec_type *smspec1 = ecl_smspec_fread_alloc(case1, ":", false);
    ecl_smspec_type *smspec2 = ecl_smspec_fread_alloc(case2, ":", false);

    test_assert_true(ecl_smspec_equal(smspec2, smspec2));
    test_assert_true(ecl_smspec_equal(smspec1, smspec1));

    test_assert_false(ecl_smspec_equal(smspec1, smspec2));
    test_assert_false(ecl_smspec_equal(smspec2, smspec1));

    ecl_smspec_free(smspec1);
    ecl_smspec_free(smspec2);
}
