#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

int main(int argc, char **argv) {
    rd::util::TestArea ta("mkdir");

    // Regular use
    test_assert_true(util_mkdir_p("some/path/with/many/levels"));

    // Absolute path where the root exists.
    {
        char *abs_path = util_alloc_abs_path("a/path/with/abs/prefix");
        test_assert_true(util_mkdir_p(abs_path));

        // Already exists:
        test_assert_true(util_mkdir_p(abs_path));
        free(abs_path);
    }

    // Permission denied
    test_assert_true(util_mkdir_p("read_only"));
    chmod("read_only", 0555);
    test_assert_false(util_mkdir_p("read_only/no/not/this"));

    exit(0);
}
