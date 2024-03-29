#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

void test_dirname() {
    const char *src_file1 = "/some/very/long/path/file.txt";
    const char *src_file2 = "relative/path/file.txt";
    const char *src_file3 = "file.txt";

    char *path1 = util_split_alloc_dirname(src_file1);
    char *path2 = util_split_alloc_dirname(src_file2);
    char *path3 = util_split_alloc_dirname(src_file3);

    test_assert_string_equal("/some/very/long/path", path1);
    test_assert_string_equal("relative/path", path2);
    test_assert_NULL(path3);

    free(path1);
    free(path2);
}

void test_filename() {
    const char *src_file1 = "/some/very/long/path/file1.txt";
    const char *src_file2 = "relative/path/file2";
    const char *src_file3 = "/tmp";

    char *file1 = util_split_alloc_filename(src_file1);
    char *file2 = util_split_alloc_filename(src_file2);
    char *file3 = util_split_alloc_filename(src_file3);

    test_assert_string_equal("file1.txt", file1);
    test_assert_string_equal("file2", file2);
    test_assert_NULL(file3);
    free(file1);
    free(file2);
}

void test_alloc_filename_empty_strings() {
    const char *path = "";
    const char *filename = "file";
    const char *extension = "";

    char *alloc_filename = util_alloc_filename(path, filename, extension);
    test_assert_string_equal(alloc_filename, filename);
    free(alloc_filename);
}

int main(int argc, char **argv) {

    test_dirname();
    test_filename();
    test_alloc_filename_empty_strings();
    exit(0);
}
