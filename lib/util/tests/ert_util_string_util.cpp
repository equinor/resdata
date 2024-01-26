#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/string_util.hpp>

void test_int_vector(const int_vector_type *list, int length, ...) {
    va_list ap;
    int i;
    va_start(ap, length);
    test_assert_int_equal(length, int_vector_size(list));

    for (i = 0; i < int_vector_size(list); i++) {
        int value = va_arg(ap, int);
        test_assert_int_equal(int_vector_iget(list, i), value);
    }

    va_end(ap);
}

void test_active_list() {
    int_vector_type *active_list = string_util_alloc_active_list("1,3- 10,15");
    test_int_vector(active_list, 10, 1, 3, 4, 5, 6, 7, 8, 9, 10, 15);
    int_vector_free(active_list);
}

static void test2(const bool_vector_type *active_mask) {
    test_assert_int_equal(bool_vector_size(active_mask), 16);

    test_assert_true(bool_vector_iget(active_mask, 1));
    test_assert_true(bool_vector_iget(active_mask, 3));
    test_assert_true(bool_vector_iget(active_mask, 4));
    test_assert_true(bool_vector_iget(active_mask, 9));
    test_assert_true(bool_vector_iget(active_mask, 10));
    test_assert_true(bool_vector_iget(active_mask, 15));

    test_assert_false(bool_vector_iget(active_mask, 0));
    test_assert_false(bool_vector_iget(active_mask, 2));
    test_assert_false(bool_vector_iget(active_mask, 11));
    test_assert_false(bool_vector_iget(active_mask, 14));
}

void test_active_mask() {
    bool_vector_type *active_mask =
        string_util_alloc_active_mask("1,3 -6,6-  10, 15");

    test2(active_mask);

    test_assert_false(string_util_update_active_mask("11,X", active_mask));
    test2(active_mask);

    bool_vector_free(active_mask);
}

void test_value_list() {
    {
        int_vector_type *int_vector = string_util_alloc_value_list("1,2,4-7");
        test_int_vector(int_vector, 6, 1, 2, 4, 5, 6, 7);
        int_vector_free(int_vector);
    }

    {
        int_vector_type *int_vector = string_util_alloc_value_list("1,2,X");
        test_int_vector(int_vector, 0);
        int_vector_free(int_vector);
    }

    {
        int_vector_type *int_vector = string_util_alloc_value_list("1,2,4-7");
        test_int_vector(int_vector, 6, 1, 2, 4, 5, 6, 7);
        int_vector_free(int_vector);
    }

    {
        int_vector_type *int_vector = string_util_alloc_value_list("5,5,5,5");
        test_int_vector(int_vector, 4, 5, 5, 5, 5);
        int_vector_free(int_vector);
    }
}

int main(int argc, char **argv) {
    test_active_list();
    test_active_mask();
    test_value_list();
    exit(0);
}
