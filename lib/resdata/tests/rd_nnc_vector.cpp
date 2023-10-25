#include <stdlib.h>
#include <stdbool.h>

#include <vector>

#include <ert/util/test_util.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/nnc_vector.hpp>

void test_basic() {
    int lgr_nr = 100;
    nnc_vector_type *vector = nnc_vector_alloc(lgr_nr);

    test_assert_true(nnc_vector_is_instance(vector));
    test_assert_int_equal(lgr_nr, nnc_vector_get_lgr_nr(vector));

    nnc_vector_add_nnc(vector, 100, 1);
    nnc_vector_add_nnc(vector, 200, 2);
    nnc_vector_add_nnc(vector, 300, 3);

    nnc_vector_add_nnc(vector, 100, 4);
    nnc_vector_add_nnc(vector, 200, 5);
    nnc_vector_add_nnc(vector, 300, 6);

    test_assert_int_equal(6, nnc_vector_get_size(vector));

    {
        const std::vector<int> &grid_index_list =
            nnc_vector_get_grid_index_list(vector);
        const std::vector<int> &nnc_index_list =
            nnc_vector_get_nnc_index_list(vector);

        test_assert_int_equal(6, nnc_index_list.size());
        test_assert_int_equal(1, nnc_index_list[0]);
        test_assert_int_equal(6, nnc_index_list[5]);

        test_assert_int_equal(6, grid_index_list.size());
        test_assert_int_equal(100, grid_index_list[0]);
        test_assert_int_equal(200, grid_index_list[1]);
        test_assert_int_equal(300, grid_index_list[2]);
    }

    nnc_vector_free(vector);
}

void test_copy() {
    int lgr_nr = 100;
    nnc_vector_type *vector1 = nnc_vector_alloc(lgr_nr);
    nnc_vector_type *vector2 = nnc_vector_alloc(lgr_nr);
    nnc_vector_type *vector3 = NULL;

    test_assert_true(nnc_vector_equal(vector1, vector2));
    test_assert_false(nnc_vector_equal(vector1, vector3));
    test_assert_false(nnc_vector_equal(vector3, vector1));
    test_assert_true(nnc_vector_equal(vector3, vector3));

    nnc_vector_add_nnc(vector1, 100, 1);
    nnc_vector_add_nnc(vector1, 200, 2);
    test_assert_false(nnc_vector_equal(vector1, vector2));

    nnc_vector_add_nnc(vector2, 100, 1);
    nnc_vector_add_nnc(vector2, 200, 2);
    test_assert_true(nnc_vector_equal(vector1, vector2));

    nnc_vector_add_nnc(vector1, 300, 3);
    nnc_vector_add_nnc(vector2, 300, 30);
    test_assert_false(nnc_vector_equal(vector1, vector2));

    vector3 = nnc_vector_alloc_copy(vector1);
    test_assert_true(nnc_vector_is_instance(vector3));
    test_assert_true(nnc_vector_equal(vector1, vector3));

    nnc_vector_free(vector1);
    nnc_vector_free(vector2);
    nnc_vector_free(vector3);
}

int main(int argc, char **argv) {
    test_basic();
    test_copy();
    exit(0);
}
