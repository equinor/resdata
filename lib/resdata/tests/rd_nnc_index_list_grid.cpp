#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/nnc_index_list.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>

int main(int argc, char **argv) {
    const char *egrid_file = argv[1];

    rd_grid_type *grid = rd_grid_alloc(egrid_file);
    rd_file_type *gfile = rd_file_open(egrid_file, 0);
    const rd_kw_type *nnc1_kw = rd_file_iget_named_kw(gfile, "NNC1", 0);
    const rd_kw_type *nnc2_kw = rd_file_iget_named_kw(gfile, "NNC2", 0);
    const int_vector_type *index_list = rd_grid_get_nnc_index_list(grid);

    {
        int_vector_type *nnc = int_vector_alloc(0, 0);

        int_vector_set_many(nnc, 0, rd_kw_get_ptr(nnc1_kw),
                            rd_kw_get_size(nnc1_kw));
        int_vector_append_many(nnc, rd_kw_get_ptr(nnc2_kw),
                               rd_kw_get_size(nnc2_kw));
        int_vector_select_unique(nnc);
        test_assert_int_equal(int_vector_size(index_list),
                              int_vector_size(nnc));

        {
            int i;
            for (i = 0; i < int_vector_size(nnc); i++)
                test_assert_int_equal(int_vector_iget(nnc, i) - 1,
                                      int_vector_iget(index_list, i));
        }
        int_vector_free(nnc);
    }

    rd_file_close(gfile);
    rd_grid_free(grid);

    exit(0);
}
