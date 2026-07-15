#include <cstdlib>
#include <ctime>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/well/well_branch_collection.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_state.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();
    const char *grid_file = argv[1];
    const char *rst_file_name = argv[2];

    rd_grid_type *grid = rd_grid_alloc(grid_file);
    rd_file_ptr rst_file = rd::File::open(rst_file_name);
    auto rst_view = rd_file_get_global_view(rst_file.get());
    auto header =
        RSTHead::read(rst_view.get(), rd_filename_report_nr(rst_file_name));
    const char *well_name = "WELL";
    int report_nr = 100;
    time_t valid_from = -1;
    bool open = false;
    auto type = WellType::GAS_INJECTOR;
    bool load_segment_information = true;

    for (int global_well_nr = 0; global_well_nr < header.nwells;
         global_well_nr++) {
        WellState well_state(well_name, global_well_nr, open, type, report_nr,
                             valid_from);
        well_state.add_connections(grid, rst_view.get(), 0);

        test_assert_true(well_state.has_grid_connections(RD_GRID_GLOBAL_GRID));
        test_assert_false(well_state.has_grid_connections("???"));

        well_state.add_MSW(rst_view.get(), global_well_nr,
                           load_segment_information);
        {
            const well_segment_collection_type *segments =
                well_state.get_segments();
            const well_branch_collection_type *branches =
                well_state.get_branches();

            if (well_state.is_MSW()) {
                test_assert_true(rd_file_has_kw(rst_file.get(), ISEG_KW));
                test_assert_int_not_equal(
                    well_segment_collection_get_size(segments), 0);
                test_assert_int_not_equal(
                    well_branch_collection_get_size(branches), 0);
            } else {
                test_assert_int_equal(
                    well_segment_collection_get_size(segments), 0);
                test_assert_int_equal(well_branch_collection_get_size(branches),
                                      0);
                test_assert_false(well_state.is_MSW());
            }
        }
    }
    exit(0);
}
