#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/test_util.hpp>

#include <ert/ecl/ecl_grid.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_kw_magic.hpp>

int main(int argc, char **argv) {
    const char *grid_file = argv[1];
    ecl_grid_type *ecl_grid = ecl_grid_alloc(grid_file);
    ecl_file_type *ecl_file = ecl_file_open(grid_file, 0);

    ecl_grid_test_lgr_consistency(ecl_grid);

    if (ecl_file_get_num_named_kw(ecl_file, COORD_KW))
        test_assert_int_equal(ecl_file_get_num_named_kw(ecl_file, COORD_KW) - 1,
                              ecl_grid_get_num_lgr(ecl_grid));

    ecl_grid_free(ecl_grid);
    ecl_file_close(ecl_file);
    exit(0);
}
