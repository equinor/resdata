#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

static std::string read_file(const std::string &filename) {
    std::ifstream stream(filename, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()};
}

void test_get_cwd() {
    test_work_area_type *work_area = test_work_area_alloc("CWD-TEST");
    const std::string cwd = std::filesystem::current_path().string();
    test_assert_string_equal(cwd.c_str(), test_work_area_get_cwd(work_area));
    test_work_area_free(work_area);
}

void test_get_original_cwd() {
    const std::string cwd = std::filesystem::current_path().string();
    test_work_area_type *work_area = test_work_area_alloc("CWD-ORG-TEST");
    test_assert_string_equal(cwd.c_str(),
                             test_work_area_get_original_cwd(work_area));
    test_work_area_free(work_area);
}

void create_test_area(const char *test_name, bool store) {
    const std::filesystem::path pre_cwd = std::filesystem::current_path();
    test_work_area_type *work_area = test_work_area_alloc__(test_name, store);
    const std::string work_path = test_work_area_get_cwd(work_area);

    test_assert_true(std::filesystem::is_directory(work_path));
    test_work_area_free(work_area);
    test_assert_bool_equal(store, std::filesystem::exists(work_path));

    test_assert_true(pre_cwd == std::filesystem::current_path());
}

void test_install_file_exists(const char *filename) {
    const std::string abs_input_path =
        std::filesystem::canonical(filename).string();
    test_work_area_type *work_area = test_work_area_alloc("FILE-TEST");

    test_work_area_install_file(work_area, filename);
    test_assert_true(read_file(abs_input_path) == read_file(filename));
    test_work_area_free(work_area);
}

void test_copy_directory(const char *rel_path) {
    test_work_area_type *work_area = test_work_area_alloc("FILE-TEST");
    test_work_area_copy_directory(work_area, rel_path);
    test_assert_true(std::filesystem::is_directory(rel_path));
    test_work_area_free(work_area);
}

void test_input() {
    test_work_area_type *work_area = test_work_area_alloc(NULL);
    test_assert_NULL(work_area);
}

void test_copy_file(const char *src_file) {
    const std::filesystem::path filename =
        std::filesystem::path(src_file).filename();
    test_work_area_type *work_area = test_work_area_alloc("copy-file");
    test_work_area_copy_file(work_area, src_file);

    test_assert_true(std::filesystem::exists(filename));

    test_work_area_free(work_area);
}

void test_copy_parent_directory(const char *path) {
    test_work_area_type *work_area =
        test_work_area_alloc("copy-parent-directory");
    const std::filesystem::path parent_path =
        std::filesystem::path(path).parent_path();

    test_assert_false(
        test_work_area_copy_parent_directory(work_area, "Does/not/exist"));
    test_assert_true(test_work_area_copy_parent_directory(work_area, path));

    test_assert_true(std::filesystem::is_directory(parent_path));

    test_work_area_free(work_area);
}

void test_copy_parent_content(const char *path) {
    const std::string parent_path =
        std::filesystem::canonical(path).parent_path().string();
    test_work_area_type *work_area =
        test_work_area_alloc("copy-parent-content");

    test_assert_false(
        test_work_area_copy_parent_content(work_area, "Does/not/exist"));
    test_assert_true(test_work_area_copy_parent_content(work_area, path));

    {

        struct dirent **src_namelist;
        struct dirent **target_namelist;
        int src_size =
            scandir(parent_path.c_str(), &src_namelist, NULL, alphasort);
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

    test_work_area_free(work_area);
}

void test_update_store() {
    {
        test_work_area_type *work_area =
            test_work_area_alloc__("update-store1", true);
        const std::string work_cwd = test_work_area_get_cwd(work_area);
        test_work_area_free(work_area);
        test_assert_true(std::filesystem::exists(work_cwd));
    }

    {
        test_work_area_type *work_area = test_work_area_alloc("update-store2");
        const std::string work_cwd = test_work_area_get_cwd(work_area);
        test_work_area_free(work_area);
        test_assert_false(std::filesystem::exists(work_cwd));
    }

    {
        test_work_area_type *work_area =
            test_work_area_alloc__("update-store3", false);
        const std::string work_cwd = test_work_area_get_cwd(work_area);
        test_work_area_free(work_area);
        test_assert_false(std::filesystem::exists(work_cwd));
    }

    {
        test_work_area_type *work_area =
            test_work_area_alloc__("update-store4", true);
        const std::string work_cwd = test_work_area_get_cwd(work_area);
        test_work_area_free(work_area);
        test_assert_true(std::filesystem::exists(work_cwd));
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
