#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/int_vector.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/test_util.hpp>

int test_iset() {
    vector_type *vector = vector_alloc_new();
    vector_iset_ref(vector, 2, vector);

    test_assert_true(vector_get_size(vector) == 3);
    test_assert_true(vector_iget(vector, 0) == NULL);
    test_assert_true(vector_iget(vector, 1) == NULL);
    test_assert_true(vector_iget(vector, 2) == vector);
    vector_free(vector);
    return 0;
}

void test_reverse() {
    const char *val1 = "value1";
    const char *val2 = "value2";
    const char *val3 = "value3";
    const char *val4 = "value4";

    vector_type *vector1 = vector_alloc_new();
    vector_type *vector2 = vector_alloc_new();

    vector_append_ref(vector1, val1);
    vector_append_ref(vector1, val2);
    vector_append_ref(vector1, val3);
    vector_append_ref(vector1, val4);

    vector_append_ref(vector2, val1);
    vector_append_ref(vector2, val2);
    vector_append_ref(vector2, val3);
    vector_append_ref(vector2, val4);

    vector_inplace_reverse(vector1);

    {
        int i;
        int size = vector_get_size(vector1);
        for (i = 0; i < vector_get_size(vector1); i++)
            test_assert_ptr_equal(vector_iget_const(vector2, i),
                                  vector_iget_const(vector1, size - 1 - i));
    }
    vector_free(vector1);
    vector_free(vector2);
}

void test_sort() {
    vector_type *v1 = vector_alloc_new();
    vector_type *v2 = vector_alloc_new();

    vector_append_ref(v1, "2");
    vector_append_ref(v2, "2");

    vector_append_ref(v1, "0");
    vector_append_ref(v2, "0");

    vector_append_ref(v1, "1");
    vector_append_ref(v2, "1");

    vector_append_ref(v1, "4");
    vector_append_ref(v2, "4");

    vector_append_ref(v1, "3");
    vector_append_ref(v2, "3");

    {
        int_vector_type *sort_map =
            vector_alloc_sort_perm(v1, (vector_cmp_ftype *)util_strcmp_int);
        vector_sort(v2, (vector_cmp_ftype *)util_strcmp_int);

        test_assert_int_equal(1, int_vector_iget(sort_map, 0));
        test_assert_int_equal(2, int_vector_iget(sort_map, 1));
        test_assert_int_equal(0, int_vector_iget(sort_map, 2));
        test_assert_int_equal(4, int_vector_iget(sort_map, 3));
        test_assert_int_equal(3, int_vector_iget(sort_map, 4));

        test_assert_string_equal("0", (const char *)vector_iget(v2, 0));
        test_assert_string_equal("1", (const char *)vector_iget(v2, 1));
        test_assert_string_equal("2", (const char *)vector_iget(v2, 2));
        test_assert_string_equal("3", (const char *)vector_iget(v2, 3));
        test_assert_string_equal("4", (const char *)vector_iget(v2, 4));

        vector_permute(v1, sort_map);

        test_assert_string_equal("0", (const char *)vector_iget(v1, 0));
        test_assert_string_equal("1", (const char *)vector_iget(v1, 1));
        test_assert_string_equal("2", (const char *)vector_iget(v1, 2));
        test_assert_string_equal("3", (const char *)vector_iget(v1, 3));
        test_assert_string_equal("4", (const char *)vector_iget(v1, 4));
        int_vector_free(sort_map);
    }
    vector_free(v1);
    vector_free(v2);
}

int main(int argc, char **argv) {
    test_iset();
    test_reverse();
    test_sort();
    exit(0);
}
