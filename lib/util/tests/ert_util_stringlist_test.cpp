#include <cstdlib>

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

void test_empty() {
    stringlist_type *s = stringlist_alloc_new();
    stringlist_free(s);
}

bool FILE_predicate(const char *name, const void *arg) {
    return util_string_equal("FILE.txt", name);
}

bool not_FILE_predicate(const char *name, const void *arg) {
    return !util_string_equal("FILE.txt", name);
}

int main(int argc, char **argv) {
    test_empty();
    test_char();
    exit(0);
}
