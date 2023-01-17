#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/util.h>

void test_path(const char *input_path, const char *expected_path) {
    char *normal_path = util_alloc_normal_path(input_path);
    test_assert_string_equal(normal_path, expected_path);
    free(normal_path);
}

void test_relative() {
    ecl::util::TestArea ta("relative_path");
    util_make_path("level0/level1/level2");

    test_path("level0/level1/../", "level0");
    test_path("level0", "level0");
    test_path("level0/././level1/../", "level0");
    test_path("level0/level1/../level1/level2/../", "level0/level1");
    test_path("level0/level1/../level1/level2/..", "level0/level1");
    test_path("level0/level1/../level1/level2/../file.txt",
              "level0/level1/file.txt");
    test_path("level0/level1/../level1a", "level0/level1a");

    test_path("a/b/c/../c/../..", "a");
    test_path("a/b/c/d/e/f", "a/b/c/d/e/f");
    util_chdir("level0/level1");
    test_path("../../level0/level1/level2/../file.txt", "file.txt");
    test_path("../../level0/level1/level2/../", "");
}

void test_beyond_root() {
    ecl::util::TestArea("beyond_root");
    char *cwd = util_alloc_cwd();
    char *backref_cwd1 =
        util_alloc_sprintf("../../../../../../../../../../../%s", cwd);
    char *backref_cwd2 =
        util_alloc_sprintf("/../../../../../../../../../../../%s", cwd);
    test_path(backref_cwd1, "");
    test_path(
        backref_cwd2,
        cwd); // The input is a semi-absolute path, and we compare with the absolute path cwd().
    free(backref_cwd1);
    free(backref_cwd2);
    free(cwd);
}

int main(int argc, char **argv) {
    test_relative();
    test_beyond_root();
    exit(0);
}
