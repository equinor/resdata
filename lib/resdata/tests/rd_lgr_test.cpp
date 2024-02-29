#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw_magic.hpp>

int main(int argc, char **argv) {
    const char *grid_file = argv[1];
    rd_grid_type *rd_grid = rd_grid_alloc(grid_file);
    rd_file_type *rd_file = rd_file_open(grid_file, 0);

    rd_grid_test_lgr_consistency(rd_grid);

    if (rd_file_get_num_named_kw(rd_file, COORD_KW))
        test_assert_int_equal(rd_file_get_num_named_kw(rd_file, COORD_KW) - 1,
                              rd_grid_get_num_lgr(rd_grid));

    rd_grid_free(rd_grid);
    rd_file_close(rd_file);
    exit(0);
}
