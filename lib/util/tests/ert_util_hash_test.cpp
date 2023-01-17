#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/hash.hpp>

int main(int argc, char **argv) {

    hash_type *h = hash_alloc();

    test_assert_bool_equal(hash_add_option(h, "Key"), false);
    test_assert_false(hash_add_option(h, "Key"));

    test_assert_true(hash_add_option(h, "Key1:Value"));
    test_assert_true(hash_add_option(h, "Key2:Value1:Value2"));
    test_assert_true(hash_add_option(h, "Key3:Value1:value2:Value3"));

    test_assert_string_equal((const char *)hash_get(h, "Key1"), "Value");
    test_assert_string_equal((const char *)hash_get(h, "Key2"),
                             "Value1:Value2");
    test_assert_string_equal((const char *)hash_get(h, "Key3"),
                             "Value1:value2:Value3");

    test_assert_false(hash_has_key(h, "Key"));

    hash_free(h);
    exit(0);
}
