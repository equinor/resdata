#include <cstdlib>

#include <ctime>
#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_file_view.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment_collection.hpp>
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
        std::unique_ptr<rd::File> rst_file = rd::File::open(rst_file_name);
        auto rst_view = rst_file->get_global_view();
        auto header =
            RSTHead::read(rst_view.get(), rd_filename_report_nr(rst_file_name));
        const char *well_name = "WELL";
        int report_nr = 100;
        time_t valid_from = -1;
        bool open = false;
        auto type = WellType::GAS_INJECTOR;
        bool load_segment_information = false;

        for (int global_well_nr = 0; global_well_nr < header.nwells;
             global_well_nr++) {
            WellState well_state(well_name, global_well_nr, open, type,
                                 report_nr, valid_from);
            well_state.add_connections(grid, rst_view.get(), 0);
            well_state.add_MSW(rst_view.get(), global_well_nr,
                               load_segment_information);

            {
                const well_segment_collection_type *segments =
                    well_state.get_segments();

                if (!rst_view->has_kw(RSEG_KW))
                    test_assert_int_equal(
                        0, well_segment_collection_get_size(segments));
            }
        }
    }

    exit(0);
}
