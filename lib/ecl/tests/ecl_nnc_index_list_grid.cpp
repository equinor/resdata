#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>
#include <ert/util/int_vector.hpp>

#include <ert/ecl/nnc_index_list.hpp>
#include <ert/ecl/ecl_grid.hpp>
#include <ert/ecl/ecl_file.hpp>

int main(int argc, char **argv) {
    const char *egrid_file = argv[1];

    ecl_grid_type *grid = ecl_grid_alloc(egrid_file);
    ecl_file_type *gfile = ecl_file_open(egrid_file, 0);
    const ecl_kw_type *nnc1_kw = ecl_file_iget_named_kw(gfile, "NNC1", 0);
    const ecl_kw_type *nnc2_kw = ecl_file_iget_named_kw(gfile, "NNC2", 0);
    const int_vector_type *index_list = ecl_grid_get_nnc_index_list(grid);

    {
        int_vector_type *nnc = int_vector_alloc(0, 0);

        int_vector_set_many(nnc, 0, ecl_kw_get_ptr(nnc1_kw),
                            ecl_kw_get_size(nnc1_kw));
        int_vector_append_many(nnc, ecl_kw_get_ptr(nnc2_kw),
                               ecl_kw_get_size(nnc2_kw));
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

    ecl_file_close(gfile);
    ecl_grid_free(grid);

    exit(0);
}
