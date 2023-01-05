#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <ert/ecl/ecl_util.hpp>
#include <ert/ecl/ecl_grid.hpp>

#include <ert/ecl_well/well_info.hpp>

int main(int argc, char **argv) {
    util_install_signals();
    const char *grid_file = argv[1];

    ecl_grid_type *grid = ecl_grid_alloc(grid_file);
    test_assert_not_NULL(grid);
    {
        well_info_type *well_info = well_info_alloc(grid);
        test_assert_not_NULL(well_info);
        if (argc >= 3)
            well_info_load_rstfile(well_info, argv[2], true);

        well_info_free(well_info);
    }
    ecl_grid_free(grid);
    exit(0);
}
