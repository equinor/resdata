#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/buffer.hpp>

void test_create() {
    buffer_type *buffer = buffer_alloc(1024);
    test_assert_true(buffer_is_instance(buffer));
    buffer_free(buffer);
}

void test_buffer_strchr() {
    buffer_type *buffer = buffer_alloc(1024);
    test_assert_false(buffer_strchr(buffer, 'A'));
    buffer_fwrite_char_ptr(buffer, "XX AB AB AB");
    test_assert_false(buffer_strchr(buffer, 'A'));

    buffer_rewind(buffer);
    test_assert_true(buffer_strchr(buffer, 'A'));
    buffer_fskip(buffer, 1);
    test_assert_true(buffer_strchr(buffer, 'A'));
    buffer_fskip(buffer, 1);
    test_assert_true(buffer_strchr(buffer, 'A'));
    buffer_fskip(buffer, 1);
    test_assert_false(buffer_strchr(buffer, 'A'));
    buffer_free(buffer);
}

void test_buffer_strstr() {
    buffer_type *buffer = buffer_alloc(1024);
    test_assert_false(buffer_strstr(buffer, "Hello World"));
    test_assert_false(buffer_strstr(buffer, ""));

    buffer_fwrite_char_ptr(buffer, "ABC");
    test_assert_int_equal(buffer_get_size(buffer), 4);
    test_assert_false(buffer_strstr(buffer, "ABC"));

    buffer_rewind(buffer);
    test_assert_true(buffer_strstr(buffer, "ABC"));
    test_assert_int_equal(buffer_get_offset(buffer), 0);
    test_assert_string_equal("ABC", (const char *)buffer_get_data(buffer));

    {
        size_t pos = buffer_get_offset(buffer);
        test_assert_false(buffer_strstr(buffer, "ABCD"));
        test_assert_size_t_equal(pos, buffer_get_offset(buffer));
    }
    buffer_rewind(buffer);
    test_assert_true(buffer_strstr(buffer, "AB"));
    test_assert_true(buffer_strstr(buffer, "ABC"));
    test_assert_true(buffer_strstr(buffer, "BC"));
    test_assert_false(buffer_strstr(buffer, "ABC"));
    buffer_free(buffer);
}

void test_buffer_search_replace1() {
    buffer_type *buffer = buffer_alloc(1024);
    test_assert_false(buffer_search_replace(buffer, "", "XYZ"));
    test_assert_false(buffer_search_replace(buffer, "XYZ", "ABC"));
    test_assert_false(buffer_search_replace(buffer, "XYZ", ""));

    buffer_fwrite_char_ptr(buffer, "ABC 123");
    buffer_rewind(buffer);
    test_assert_true(buffer_search_replace(buffer, "ABC", "XYZ"));
    buffer_rewind(buffer);
    test_assert_string_equal("XYZ 123", (const char *)buffer_get_data(buffer));

    buffer_rewind(buffer);
    test_assert_true(buffer_search_replace(buffer, "XYZ", "A"));
    buffer_rewind(buffer);
    test_assert_string_equal("A 123", (const char *)buffer_get_data(buffer));

    buffer_rewind(buffer);
    test_assert_true(buffer_search_replace(buffer, "A", "XYZ"));
    buffer_rewind(buffer);
    test_assert_string_equal("XYZ 123", (const char *)buffer_get_data(buffer));

    buffer_free(buffer);
}

void test_buffer_search_replace2() {
    buffer_type *buffer = buffer_alloc(1024);
    buffer_fwrite_char_ptr(buffer,
                           "MAGIC_PRINT  magic-list.txt  <ERTCASE>  __MAGIC__");

    buffer_rewind(buffer);
    test_assert_false(buffer_search_replace(buffer, "<CASE>", "SUPERCase"));
    test_assert_string_equal(
        "MAGIC_PRINT  magic-list.txt  <ERTCASE>  __MAGIC__",
        (const char *)buffer_get_data(buffer));

    buffer_rewind(buffer);
    test_assert_true(buffer_search_replace(buffer, "<ERTCASE>", "default"));
    test_assert_string_equal("MAGIC_PRINT  magic-list.txt  default  __MAGIC__",
                             (const char *)buffer_get_data(buffer));

    buffer_free(buffer);
}

void test_char_ptr() {
    buffer_type *buffer = buffer_alloc(1024);
    buffer_fwrite_char_ptr(buffer, "Hello World");
    test_assert_size_t_equal(buffer_get_size(buffer), 12);
    test_assert_int_equal(strlen((const char *)buffer_get_data(buffer)), 11);

    buffer_clear(buffer);
    buffer_fwrite_char_ptr(buffer, "Hello");
    buffer_strcat(buffer, " ");
    buffer_strcat(buffer, "World");
    test_assert_size_t_equal(buffer_get_size(buffer), 12);
    test_assert_int_equal(strlen((const char *)buffer_get_data(buffer)), 11);

    buffer_free(buffer);
}

int main(int argc, char **argv) {
    test_create();
    test_char_ptr();
    test_buffer_strchr();
    test_buffer_strstr();
    test_buffer_search_replace1();
    test_buffer_search_replace2();
    exit(0);
}
