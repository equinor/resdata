#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>
#include <ert/util/type_vector_functions.hpp>
#include <ert/util/test_util.hpp>

void test_index_list() {
    int_vector_type *index_list = int_vector_alloc(0, 0);
    int_vector_append(index_list, 10);
    int_vector_append(index_list, 20);
    int_vector_append(index_list, 30);
    {
        bool_vector_type *mask = int_vector_alloc_mask(index_list);

        test_assert_false(bool_vector_get_default(mask));
        test_assert_int_equal(31, bool_vector_size(mask));
        test_assert_true(bool_vector_iget(mask, 10));
        test_assert_true(bool_vector_iget(mask, 20));
        test_assert_true(bool_vector_iget(mask, 30));
        test_assert_int_equal(3, bool_vector_count_equal(mask, true));

        bool_vector_free(mask);
    }
    int_vector_free(index_list);
}

void test_mask() {
    bool_vector_type *mask = bool_vector_alloc(0, false);

    bool_vector_iset(mask, 10, true);
    bool_vector_iset(mask, 15, true);
    bool_vector_iset(mask, 20, true);

    {
        int_vector_type *index_list = bool_vector_alloc_active_list(mask);

        test_assert_int_equal(3, int_vector_size(index_list));
        test_assert_int_equal(10, int_vector_iget(index_list, 0));
        test_assert_int_equal(15, int_vector_iget(index_list, 1));
        test_assert_int_equal(20, int_vector_iget(index_list, 2));

        int_vector_free(index_list);
    }
    bool_vector_free(mask);
}

void test_approx_equal() {
    double_vector_type *d1 = double_vector_alloc(0, 0);
    double_vector_type *d2 = double_vector_alloc(0, 0);
    double_vector_type *d3 = double_vector_alloc(0, 0);

    double_vector_append(d1, 1.0);
    double_vector_append(d1, 2.0);
    double_vector_append(d1, 3.0);

    double_vector_append(d2, 1.0);
    double_vector_append(d2, 2.0);

    test_assert_false(double_vector_approx_equal(d1, d2, 1e-6));

    double_vector_append(d2, 3.0);
    test_assert_true(double_vector_approx_equal(d1, d2, 1e-6));

    double_vector_append(d3, 1.0);
    double_vector_append(d3, 2.0);
    double_vector_append(d3, 3.0);

    double_vector_scale(d3, 1 + 1e-6);
    test_assert_true(double_vector_approx_equal(d1, d3, 1e-4));
    test_assert_false(double_vector_approx_equal(d1, d3, 1e-8));

    double_vector_free(d1);
    double_vector_free(d2);
    double_vector_free(d3);
}

int main(int argc, char **argv) {
    test_index_list();
    test_mask();
    test_approx_equal();
    exit(0);
}
