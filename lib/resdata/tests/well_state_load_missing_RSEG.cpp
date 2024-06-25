#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw_magic.hpp>

#include <resdata/well/well_state.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();
    {
        const char *grid_file = argv[1];
        const char *rst_file_name = argv[2];

        rd_grid_type *grid = rd_grid_alloc(grid_file);
        rd_file_type *rst_file = rd_file_open(rst_file_name, 0);
        rd_file_view_type *rst_view = rd_file_get_active_view(rst_file);
        rd_rsthead_type *header =
            rd_rsthead_alloc(rst_view, rd_filename_report_nr(rst_file_name));
        const char *well_name = "WELL";
        int report_nr = 100;
        time_t valid_from = -1;
        bool open = false;
        well_type_enum type = RD_WELL_GAS_INJECTOR;
        int global_well_nr = 0;
        bool load_segment_information = false;

        for (global_well_nr = 0; global_well_nr < header->nwells;
             global_well_nr++) {
            well_state_type *well_state = well_state_alloc(
                well_name, global_well_nr, open, type, report_nr, valid_from);
            test_assert_true(well_state_is_instance(well_state));
            well_state_add_connections2(well_state, grid, rst_view, 0);
            well_state_add_MSW2(well_state, rst_view, global_well_nr,
                                load_segment_information);

            {
                const well_segment_collection_type *segments =
                    well_state_get_segments(well_state);

                if (!rd_file_view_has_kw(rst_view, RSEG_KW))
                    test_assert_int_equal(
                        0, well_segment_collection_get_size(segments));
            }
            well_state_free(well_state);
        }
    }

    exit(0);
}
