#include <cstdlib>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>

int main(int argc, char **argv) {
    rd_grid_ptr grid = read_grid(argv[1]);

    int num_lgr = rd_grid_get_num_lgr(grid.get());
    int lgr_index;
    for (lgr_index = 0; lgr_index < num_lgr; lgr_index++) {
        rd_grid_type *lgr_from_index = rd_grid_iget_lgr(grid.get(), lgr_index);
        int lgr_nr = rd_grid_get_lgr_nr(lgr_from_index);
        rd_grid_type *lgr_from_nr =
            rd_grid_get_lgr_from_lgr_nr(grid.get(), lgr_nr);

        test_assert_ptr_equal(lgr_from_index, lgr_from_nr);

        test_assert_string_equal(rd_grid_get_lgr_name(grid.get(), lgr_nr),
                                 rd_grid_iget_lgr_name(grid.get(), lgr_index));
        printf("Grid[%d:%d] : %s \n", lgr_index, lgr_nr,
               rd_grid_iget_lgr_name(grid.get(), lgr_index));
    }
    exit(0);
}
