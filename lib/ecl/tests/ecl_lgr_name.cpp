#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <ert/ecl/ecl_grid.hpp>

int main(int argc, char **argv) {
    ecl_grid_type *grid = ecl_grid_alloc(argv[1]);

    int num_lgr = ecl_grid_get_num_lgr(grid);
    int lgr_index;
    for (lgr_index = 0; lgr_index < num_lgr; lgr_index++) {
        ecl_grid_type *lgr_from_index = ecl_grid_iget_lgr(grid, lgr_index);
        int lgr_nr = ecl_grid_get_lgr_nr(lgr_from_index);
        ecl_grid_type *lgr_from_nr = ecl_grid_get_lgr_from_lgr_nr(grid, lgr_nr);

        test_assert_ptr_equal(lgr_from_index, lgr_from_nr);

        test_assert_string_equal(ecl_grid_get_lgr_name(grid, lgr_nr),
                                 ecl_grid_iget_lgr_name(grid, lgr_index));
        printf("Grid[%d:%d] : %s \n", lgr_index, lgr_nr,
               ecl_grid_iget_lgr_name(grid, lgr_index));
    }
    ecl_grid_free(grid);

    exit(0);
}
