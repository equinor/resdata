#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/util.h>
#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>

#include <ert/ecl/ecl_util.hpp>

void test_relative_access() {
    ecl::util::TestArea ta("ecl_access");
    test_assert_false(ecl_util_path_access("No/directory/does/not/exist"));

    util_make_path("path");
    test_assert_true(ecl_util_path_access("path"));
    test_assert_true(ecl_util_path_access("path/FILE_DOES_NOT_EXIST"));

    {
        FILE *f = util_fopen("path/file", "w");
        fprintf(f, "Hello\n");
        fclose(f);
    }
    test_assert_true(ecl_util_path_access("path/file"));
    chmod("path/file", 0);
    test_assert_false(ecl_util_path_access("path/file"));

    test_assert_true(ecl_util_path_access("ECLIPSE_CASE"));
}

int main(int argc, char **argv) { test_relative_access(); }
