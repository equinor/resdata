#include <cstdlib>
#include <filesystem>
#ifndef ERT_WINDOWS
#include <sys/stat.h>
#endif

#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_util.hpp>

void test_relative_access() {
    rd::util::TestArea ta("rd_access");
    test_assert_false(rd_path_access("No/directory/does/not/exist"));

    std::filesystem::create_directories("path");
    test_assert_true(rd_path_access("path"));
    test_assert_true(rd_path_access("path/FILE_DOES_NOT_EXIST"));

    {
        FILE *f = util_fopen("path/file", "w");
        fprintf(f, "Hello\n");
        fclose(f);
    }
    test_assert_true(rd_path_access("path/file"));
#ifndef ERT_WINDOWS
    // Windows has no permission bits that deny reading
    chmod("path/file", 0);
    test_assert_false(rd_path_access("path/file"));
#endif

    test_assert_true(rd_path_access("ECLIPSE_CASE"));
}

int main(int argc, char **argv) { test_relative_access(); }
