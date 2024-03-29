#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

void test_get_cwd() {
    test_work_area_type *work_area = test_work_area_alloc("CWD-TEST");
    char *cwd = util_alloc_cwd();
    test_assert_string_equal(cwd, test_work_area_get_cwd(work_area));
    free(cwd);
    test_work_area_free(work_area);
}

void test_get_original_cwd() {
    char *cwd = util_alloc_cwd();
    test_work_area_type *work_area = test_work_area_alloc("CWD-ORG-TEST");
    test_assert_string_equal(cwd, test_work_area_get_original_cwd(work_area));
    free(cwd);
    test_work_area_free(work_area);
}

void create_test_area(const char *test_name, bool store) {
    char *pre_cwd = util_alloc_cwd();
    test_work_area_type *work_area = test_work_area_alloc__(test_name, store);
    char *work_path = util_alloc_string_copy(test_work_area_get_cwd(work_area));

    test_assert_true(util_is_directory(work_path));
    test_work_area_free(work_area);
    test_assert_bool_equal(store, util_entry_exists(work_path));

    {
        char *post_cwd = util_alloc_cwd();
        test_assert_string_equal(pre_cwd, post_cwd);
        free(post_cwd);
    }
    free(pre_cwd);
    free(work_path);
}

void test_install_file_exists(const char *filename) {
    char *abs_input_path = util_alloc_abs_path(filename);
    test_work_area_type *work_area = test_work_area_alloc("FILE-TEST");

    test_work_area_install_file(work_area, filename);
    test_assert_true(util_files_equal(abs_input_path, filename));
    test_work_area_free(work_area);
    free(abs_input_path);
}

void test_copy_directory(const char *rel_path) {
    test_work_area_type *work_area = test_work_area_alloc("FILE-TEST");
    test_work_area_copy_directory(work_area, rel_path);
    test_assert_true(util_is_directory(rel_path));
    test_work_area_free(work_area);
}

void test_input() {
    test_work_area_type *work_area = test_work_area_alloc(NULL);
    test_assert_NULL(work_area);
}

void test_copy_file(const char *src_file) {
    char *filename = util_split_alloc_filename(src_file);
    test_work_area_type *work_area = test_work_area_alloc("copy-file");
    test_work_area_copy_file(work_area, src_file);

    test_assert_true(util_file_exists(filename));

    test_work_area_free(work_area);
    free(filename);
}

void test_copy_parent_directory(const char *path) {
    test_work_area_type *work_area =
        test_work_area_alloc("copy-parent-directory");
    char *parent_path;

    {
        char *full_path = util_alloc_abs_path(path);
        util_alloc_file_components(path, &parent_path, NULL, NULL);
        free(full_path);
    }

    test_assert_false(
        test_work_area_copy_parent_directory(work_area, "Does/not/exist"));
    test_assert_true(test_work_area_copy_parent_directory(work_area, path));

    test_assert_true(util_entry_exists(parent_path));
    test_assert_true(util_is_directory(parent_path));

    test_work_area_free(work_area);
    free(parent_path);
}

void test_copy_parent_content(const char *path) {
    char *full_path = util_alloc_abs_path(path);
    char *parent_path = util_alloc_parent_path(full_path);
    test_work_area_type *work_area =
        test_work_area_alloc("copy-parent-content");

    test_assert_false(
        test_work_area_copy_parent_content(work_area, "Does/not/exist"));
    test_assert_true(test_work_area_copy_parent_content(work_area, path));

    {

        struct dirent **src_namelist;
        struct dirent **target_namelist;
        int src_size = scandir(parent_path, &src_namelist, NULL, alphasort);
        int target_size = scandir(test_work_area_get_cwd(work_area),
                                  &target_namelist, NULL, alphasort);

        test_assert_int_equal(src_size, target_size);
        for (int i = 0; i < src_size; i++) {
            test_assert_string_equal(src_namelist[i]->d_name,
                                     target_namelist[i]->d_name);

            free(src_namelist[i]);
            free(target_namelist[i]);
        }

        free(src_namelist);
        free(target_namelist);
    }
    free(parent_path);
    free(full_path);

    test_work_area_free(work_area);
}

void test_update_store() {
    {
        test_work_area_type *work_area =
            test_work_area_alloc__("update-store1", true);
        char *work_cwd =
            util_alloc_string_copy(test_work_area_get_cwd(work_area));
        test_work_area_free(work_area);
        test_assert_true(util_entry_exists(work_cwd));
        free(work_cwd);
    }

    {
        test_work_area_type *work_area = test_work_area_alloc("update-store2");
        char *work_cwd =
            util_alloc_string_copy(test_work_area_get_cwd(work_area));
        test_work_area_free(work_area);
        test_assert_false(util_entry_exists(work_cwd));
        free(work_cwd);
    }

    {
        test_work_area_type *work_area =
            test_work_area_alloc__("update-store3", false);
        char *work_cwd =
            util_alloc_string_copy(test_work_area_get_cwd(work_area));
        test_work_area_free(work_area);
        test_assert_false(util_entry_exists(work_cwd));
        free(work_cwd);
    }

    {
        test_work_area_type *work_area =
            test_work_area_alloc__("update-store4", true);
        char *work_cwd =
            util_alloc_string_copy(test_work_area_get_cwd(work_area));
        test_work_area_free(work_area);
        test_assert_true(util_entry_exists(work_cwd));
        free(work_cwd);
    }
}

int main(int argc, char **argv) {
    const char *rel_path_file = argv[1];
    const char *abs_path_file = argv[2];
    const char *rel_directory = argv[3];

    create_test_area("STORE-TEST", true);
    create_test_area("DEL-TEST", false);

    test_install_file_exists(rel_path_file);
    test_install_file_exists(abs_path_file);

    test_copy_directory(rel_directory);
    test_input();
    test_get_cwd();
    test_get_original_cwd();

    test_copy_file(rel_path_file);
    test_copy_file(abs_path_file);

    test_copy_parent_directory(rel_path_file);
    test_copy_parent_directory(abs_path_file);

    test_copy_parent_content(rel_path_file);
    test_copy_parent_content(abs_path_file);

    test_update_store();
    exit(0);
}
