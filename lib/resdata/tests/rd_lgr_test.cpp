#include <cstdlib>

#include <string>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw_magic.hpp>

int main(int argc, char **argv) {
    const char *grid_file = argv[1];
    rd_grid_ptr rd_grid = read_grid(grid_file);
    auto rd_file = rd::File::open(grid_file);

    rd_grid_test_lgr_consistency(rd_grid.get());

    if (rd_file_get_num_named_kw(rd_file.get(), COORD_KW))
        test_assert_int_equal(
            rd_file_get_num_named_kw(rd_file.get(), COORD_KW) - 1,
            rd_grid_get_num_lgr(rd_grid.get()));
    exit(0);
}
