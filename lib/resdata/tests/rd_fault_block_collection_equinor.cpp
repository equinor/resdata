#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/fault_block_collection.hpp>

void test_create(const rd_grid_type *grid, const rd_kw_type *fault_block_kw) {
    fault_block_collection_type *fault_blocks =
        fault_block_collection_alloc(grid);
    test_assert_true(fault_block_collection_is_instance(fault_blocks));
    test_assert_int_equal(rd_grid_get_nz(grid),
                          fault_block_collection_num_layers(fault_blocks));
    fault_block_collection_free(fault_blocks);
}

void test_get_layer(const rd_grid_type *grid,
                    const rd_kw_type *fault_block_kw) {
    fault_block_collection_type *fault_blocks =
        fault_block_collection_alloc(grid);

    test_assert_NULL(fault_block_collection_get_layer(fault_blocks, -1));
    test_assert_NULL(
        fault_block_collection_get_layer(fault_blocks, rd_grid_get_nz(grid)));
    {
        int k;
        for (k = 0; k < rd_grid_get_nz(grid); k++) {
            fault_block_layer_type *layer =
                fault_block_collection_get_layer(fault_blocks, k);
            test_assert_true(fault_block_layer_is_instance(layer));
        }
    }
    fault_block_collection_free(fault_blocks);
}

int main(int argc, char **argv) {
    const char *grid_file = argv[1];
    const char *fault_blk_file = argv[2];
    rd_grid_type *rd_grid = rd_grid_alloc(grid_file);
    rd_kw_type *fault_blk_kw;
    {
        FILE *stream = util_fopen(fault_blk_file, "r");
        fault_blk_kw = rd_kw_fscanf_alloc_grdecl(
            stream, "FAULTBLK", rd_grid_get_global_size(rd_grid), RD_INT_TYPE);
        fclose(stream);
    }

    test_create(rd_grid, fault_blk_kw);
    test_get_layer(rd_grid, fault_blk_kw);

    rd_grid_free(rd_grid);
    rd_kw_free(fault_blk_kw);
    exit(0);
}
