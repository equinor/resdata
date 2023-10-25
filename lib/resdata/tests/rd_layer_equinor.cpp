#include <stdlib.h>
#include <stdbool.h>

#include <vector>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/layer.hpp>

#include "detail/resdata/layer_cxx.hpp"

rd_kw_type *alloc_faultblock_kw(const char *filename, int grid_size) {
    FILE *stream = util_fopen(filename, "r");
    rd_kw_type *kw =
        rd_kw_fscanf_alloc_grdecl(stream, "FAULTBLK", grid_size, RD_INT);
    fclose(stream);

    return kw;
}

void test_layer(const rd_grid_type *rd_grid, const rd_kw_type *faultblock_kw,
                int k) {
    int nx = rd_grid_get_nx(rd_grid);
    int ny = rd_grid_get_ny(rd_grid);
    layer_type *layer = layer_alloc(nx, ny);
    int i, j;

    for (j = 0; j < ny; j++)
        for (i = 0; i < nx; i++) {
            int g = rd_grid_get_global_index3(rd_grid, i, j, k);
            int fblk = rd_kw_iget_int(faultblock_kw, g);
            layer_iset_cell_value(layer, i, j, fblk);
        }

    {
        std::vector<int_point2d_type> corner_list;
        int_vector_type *i_list = int_vector_alloc(0, 0);
        int_vector_type *j_list = int_vector_alloc(0, 0);
        int_vector_type *cell_list = int_vector_alloc(0, 0);

        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                int cell_value = layer_iget_cell_value(layer, i, j);
                if (cell_value != 0) {
                    test_assert_true(layer_trace_block_edge(
                        layer, i, j, cell_value, corner_list, cell_list));
                    test_assert_true(layer_trace_block_content(
                        layer, true, i, j, cell_value, i_list, j_list));
                }
            }
        }
        test_assert_int_equal(0, layer_get_cell_sum(layer));
        int_vector_free(i_list);
        int_vector_free(j_list);
        int_vector_free(cell_list);
    }

    layer_free(layer);
}

int main(int argc, char **argv) {
    rd_grid_type *rd_grid = rd_grid_alloc(argv[1]);
    rd_kw_type *faultblock_kw =
        alloc_faultblock_kw(argv[2], rd_grid_get_global_size(rd_grid));
    int k;

    for (k = 0; k < rd_grid_get_nz(rd_grid); k++)
        test_layer(rd_grid, faultblock_kw, k);

    rd_kw_free(faultblock_kw);
    rd_grid_free(rd_grid);
    exit(0);
}
