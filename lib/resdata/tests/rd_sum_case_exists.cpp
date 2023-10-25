#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>
#include <ert/util/test_work_area.hpp>
#include <ert/util/path_stack.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_sum.hpp>

void test_case(const char *sum_case, bool expected_exist) {
    test_assert_bool_equal(expected_exist, rd_sum_case_exists(sum_case));
}

void test_case_no_path(const char *sum_case, bool expected_exist) {
    path_stack_type *path_stack = path_stack_alloc();
    path_stack_push_cwd(path_stack);
    {
        char *basename, *path;

        util_alloc_file_components(sum_case, &path, &basename, NULL);
        if (path)
            chdir(path);
        test_assert_bool_equal(expected_exist, rd_sum_case_exists(basename));

        free(path);
        free(basename);
    }
    path_stack_pop(path_stack);
    path_stack_free(path_stack);
}

int main(int argc, char **argv) {
    const char *existing_case = argv[1];
    const char *missing_header = argv[2];
    const char *missing_data = argv[3];
    test_assert_false(rd_sum_case_exists("/does/not/exist"));

    test_case(existing_case, true);
    test_case_no_path(existing_case, true);

    test_case(missing_header, false);
    test_case_no_path(missing_header, false);

    test_case(missing_data, false);
    test_case_no_path(missing_data, false);
}
