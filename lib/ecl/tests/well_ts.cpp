#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <ert/ecl/ecl_util.hpp>
#include <ert/ecl/ecl_grid.hpp>

#include <ert/ecl_well/well_info.hpp>

int main(int argc, char **argv) {
    const char *case_path = argv[1];
    char *grid_file = util_alloc_filename(NULL, case_path, "EGRID");
    stringlist_type *file_list = stringlist_alloc_new();
    ecl_grid_type *grid = ecl_grid_alloc(grid_file);
    ecl_util_select_filelist(NULL, case_path, ECL_RESTART_FILE, false,
                             file_list);

    printf("Searching in:%s \n", case_path);
    test_assert_int_equal(4, stringlist_get_size(file_list));
    stringlist_sort(file_list, (string_cmp_ftype *)util_strcmp_int);

    {
        int i;
        for (i = 0; i < stringlist_get_size(file_list); i++) {
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
        well_info_type *well_info = well_info_alloc(grid);
        int i;
        for (i = 0; i < stringlist_get_size(file_list); i++) {
            printf("Loading file:%s \n", stringlist_iget(file_list, i));
            well_info_load_rstfile(well_info, stringlist_iget(file_list, i),
                                   true);
        }
        well_info_free(well_info);
    }

    {
        well_info_type *well_info = well_info_alloc(grid);
        int i;
        stringlist_reverse(file_list);
        for (i = 0; i < stringlist_get_size(file_list); i++)
            well_info_load_rstfile(well_info, stringlist_iget(file_list, i),
                                   true);
        well_info_free(well_info);
    }
    ecl_grid_free(grid);
    free(grid_file);

    exit(0);
}
