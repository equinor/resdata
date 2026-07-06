#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_info.hpp>

int main(int argc, char **argv) {
    util_install_signals();
    rd_grid_ptr grid = read_grid(argv[1]);
    test_assert_not_NULL(grid.get());
    {
        well_info_type *well_info = well_info_alloc(grid.get());
        test_assert_not_NULL(well_info);
        if (argc >= 3)
            well_info_load_rstfile(well_info, argv[2], true);

        well_info_free(well_info);
    }
    exit(0);
}
