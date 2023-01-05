#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/util.h>
#include <ert/util/string_util.hpp>

void test_copy_file(const char *executable) {
    struct stat stat_buf;
    mode_t mode0, mode1;
    stat(executable, &stat_buf);

    mode0 = stat_buf.st_mode;
    {
        ecl::util::TestArea ta("copy_file");

        util_copy_file(executable, "test.x");
        test_assert_true(util_file_exists("test.x"));
        stat("test.x", &stat_buf);
        mode1 = stat_buf.st_mode;

        test_assert_true(mode0 == mode1);
    }
}

int main(int argc, char **argv) {
    const char *executable = argv[1];
    test_copy_file(executable);
    exit(0);
}
