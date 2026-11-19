#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <filesystem>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_info.hpp>
#include <string>

int main(int argc, char **argv) {
    const char *case_path = argv[1];
    const std::string grid_file = std::string(case_path) + ".EGRID";
    rd_grid_ptr grid = read_grid(grid_file.c_str());
    auto file_list =
        rd_select_filelist(NULL, case_path, FileType::RESTART, false);

    printf("Searching in:%s \n", case_path);
    test_assert_size_t_equal(4, file_list.size());
    std::sort(file_list.begin(), file_list.end(), rd::natural_less);

    for (size_t i = 0; i < file_list.size(); i++) {
        char *target_ext = util_alloc_sprintf(".X%04zu", i);
        const std::string ext =
            std::filesystem::path(file_list[i]).extension().string();

        test_assert_string_equal(ext.c_str(), target_ext);
        free(target_ext);
    }
    {
        WellInfo well_info(grid.get());
        for (const auto &file_name : file_list) {
            printf("Loading file:%s \n", file_name.c_str());
            well_info.load_rstfile(file_name, true);
        }
    }
    {
        WellInfo well_info(grid.get());
        std::reverse(file_list.begin(), file_list.end());
        for (const auto &file_name : file_list)
            well_info.load_rstfile(file_name, true);
    }

    exit(0);
}
