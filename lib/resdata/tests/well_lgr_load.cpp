#include <cstdlib>
#include <ctime>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>

#include <resdata/well/well_info.hpp>

int main(int argc, char **argv) {
    {
        rd_grid_type *grid = rd_grid_alloc(argv[1]);
        WellInfo well_info(grid);

        well_info.load_rstfile(argv[2], true);

        // List all wells:
        for (size_t iwell = 0; iwell < well_info.num_wells(); iwell++) {
            auto well_ts = well_info.get_ts(well_info.get_well_name(iwell));
            auto well_state = well_ts->at(well_ts->size() - 1);
            test_assert_not_NULL(well_state.get());
        }
    }

    exit(0);
}
