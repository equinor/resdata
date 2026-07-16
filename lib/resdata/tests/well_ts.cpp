#include <cstdio>
#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_info.hpp>

int main(int argc, char **argv) {
    const char *case_path = argv[1];
    char *grid_file = util_alloc_filename(NULL, case_path, "EGRID");
    stringlist_type *file_list = stringlist_alloc_new();
    rd_grid_ptr grid = read_grid(grid_file);
    rd_select_filelist(NULL, case_path, FileType::RESTART, false, file_list);

    printf("Searching in:%s \n", case_path);
    test_assert_int_equal(4, stringlist_get_size(file_list));
    stringlist_sort(file_list, (string_cmp_ftype *)util_strcmp_int);

    {
        for (int i = 0; i < stringlist_get_size(file_list); i++) {
            char *ext;
            char *target_ext = util_alloc_sprintf("X%04d", i);
            util_alloc_file_components(stringlist_iget(file_list, i), NULL,
                                       NULL, &ext);

            test_assert_string_equal(ext, target_ext);
            free(ext);
            free(target_ext);
        }
    }
    {
        WellInfo well_info(grid.get());
        for (int i = 0; i < stringlist_get_size(file_list); i++) {
            printf("Loading file:%s \n", stringlist_iget(file_list, i));
            well_info.load_rstfile(stringlist_iget(file_list, i), true);
        }
    }

    {
        WellInfo well_info(grid.get());
        stringlist_reverse(file_list);
        for (int i = 0; i < stringlist_get_size(file_list); i++)
            well_info.load_rstfile(stringlist_iget(file_list, i), true);
    }
    free(grid_file);

    exit(0);
}
