#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>

#include <resdata/well/well_info.hpp>

int main(int argc, char **argv) {
    util_install_signals();
    rd_grid_ptr grid = read_grid(argv[1]);
    test_assert_not_NULL(grid.get());

    WellInfo well_info(grid.get());
    if (argc >= 3)
        well_info.load_rstfile(argv[2], true);
    exit(0);
}
