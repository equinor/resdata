#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>

/*
  Name ..................: LG003017
  Grid nr ...............: 104

  Name ..................: LG006024
  Grid nr ...............: 2

  Name ..................: LG005025
  Grid nr ...............: 4

  Name ..................: LG011029
  Grid nr ...............: 82

  Name ..................: LG007021
  Grid nr ...............: 100

  Name ..................: LG003014
  Grid nr ...............: 110
*/

void test_name(const rd_grid_type *grid, int lgr_nr, const char *name) {
    test_assert_string_equal(name, rd_grid_get_lgr_name(grid, lgr_nr));
    test_assert_int_equal(lgr_nr, rd_grid_get_lgr_nr_from_name(grid, name));
}

int main(int argc, char **argv) {
    const char *grid_file = argv[1];
    rd_grid_type *grid = rd_grid_alloc(grid_file);

    test_name(grid, 104, "LG003017");
    test_name(grid, 2, "LG006024");
    test_name(grid, 4, "LG005025");
    test_name(grid, 82, "LG011029");
    test_name(grid, 100, "LG007021");
    test_name(grid, 110, "LG003014");
    test_name(grid, 0, grid_file);

    rd_grid_free(grid);
    exit(0);
}
