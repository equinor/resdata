#include <cstdio>
#include <cstdlib>

#include <algorithm>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_info.hpp>
#include <string>

int main(int argc, char **argv) {
    const char *case_path = argv[1];
    char *grid_file = util_alloc_filename(NULL, case_path, "EGRID");
    rd_grid_ptr grid = read_grid(grid_file);
    auto file_list =
        rd_select_filelist(NULL, case_path, FileType::RESTART, false);

    printf("Searching in:%s \n", case_path);
    test_assert_size_t_equal(4, file_list.size());
    std::sort(file_list.begin(), file_list.end(), rd::natural_less);

    for (size_t i = 0; i < file_list.size(); i++) {
        char *ext;
        char *target_ext = util_alloc_sprintf("X%04zu", i);
        util_alloc_file_components(file_list[i].c_str(), NULL, NULL, &ext);

        test_assert_string_equal(ext, target_ext);
        free(ext);
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

    free(grid_file);

    exit(0);
}
