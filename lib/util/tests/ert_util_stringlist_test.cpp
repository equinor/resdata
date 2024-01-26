#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/test_work_area.hpp>

void test_char() {
    const char *S1 = "S1";
    const char *S2 = "S2";
    const char *S3 = "S3";
    stringlist_type *s = stringlist_alloc_new();
    stringlist_append_copy(s, S1);
    stringlist_append_copy(s, S2);
    stringlist_append_copy(s, S3);

    stringlist_free(s);
}

void test_reverse() {
    const char *s0 = "AAA";
    const char *s1 = "BBB";
    const char *s2 = "CCC";

    stringlist_type *s = stringlist_alloc_new();
    stringlist_append_copy(s, s0);
    stringlist_append_copy(s, s1);
    stringlist_append_copy(s, s2);

    stringlist_reverse(s);

    test_assert_string_equal(s2, stringlist_iget(s, 0));
    test_assert_string_equal(s1, stringlist_iget(s, 1));
    test_assert_string_equal(s0, stringlist_iget(s, 2));

    stringlist_free(s);
}

void test_iget_as_double() {
    stringlist_type *s = stringlist_alloc_new();
    stringlist_append_copy(s, "1000.90");
    stringlist_append_copy(s, "1000");
    stringlist_append_copy(s, "XXXX");

    {
        double value;
        bool valid;

        value = stringlist_iget_as_double(s, 0, &valid);
        test_assert_double_equal(value, 1000.90);
        test_assert_true(valid);

        value = stringlist_iget_as_double(s, 1, &valid);
        test_assert_double_equal(value, 1000.0);
        test_assert_true(valid);

        value = stringlist_iget_as_double(s, 2, &valid);
        test_assert_double_equal(value, -1);
        test_assert_false(valid);
    }
    stringlist_free(s);
}

void test_empty() {
    stringlist_type *s = stringlist_alloc_new();
    stringlist_free(s);
}

void test_front_back() {
    stringlist_type *s = stringlist_alloc_new();

    stringlist_append_copy(s, "First");
    test_assert_string_equal("First", stringlist_front(s));
    test_assert_string_equal("First", stringlist_back(s));

    stringlist_append_copy(s, "Last");
    test_assert_string_equal("First", stringlist_front(s));
    test_assert_string_equal("Last", stringlist_back(s));
    stringlist_free(s);
}

bool FILE_predicate(const char *name, const void *arg) {
    return util_string_equal("FILE.txt", name);
}

bool not_FILE_predicate(const char *name, const void *arg) {
    return !util_string_equal("FILE.txt", name);
}

void test_predicate_matching() {
    rd::util::TestArea ta("stringlist");
    stringlist_type *s = stringlist_alloc_new();
    stringlist_append_copy(s, "s");
    stringlist_select_files(s, "does/not/exist", NULL, NULL);
    test_assert_int_equal(stringlist_get_size(s), 0);

    {
        FILE *f = util_fopen("FILE.txt", "w");
        fclose(f);
    }
    stringlist_select_files(s, ta.test_cwd().c_str(), NULL, NULL);
    test_assert_int_equal(1, stringlist_get_size(s));
    {
        char *exp = util_alloc_abs_path("FILE.txt");
        test_assert_string_equal(exp, stringlist_iget(s, 0));
        free(exp);
    }

    stringlist_select_files(s, NULL, NULL, NULL);
    test_assert_int_equal(1, stringlist_get_size(s));
    test_assert_string_equal("FILE.txt", stringlist_iget(s, 0));

    stringlist_select_files(s, ta.test_cwd().c_str(), FILE_predicate, NULL);
    test_assert_int_equal(1, stringlist_get_size(s));
    {
        char *exp = util_alloc_abs_path("FILE.txt");
        test_assert_string_equal(exp, stringlist_iget(s, 0));
        free(exp);
    }

    stringlist_select_files(s, ta.test_cwd().c_str(), not_FILE_predicate, NULL);
    test_assert_int_equal(0, stringlist_get_size(s));

    stringlist_free(s);
}

int main(int argc, char **argv) {
    test_empty();
    test_char();
    test_reverse();
    test_iget_as_double();
    test_predicate_matching();
    exit(0);
}
