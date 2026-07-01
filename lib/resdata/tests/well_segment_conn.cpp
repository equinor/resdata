#include <cstdlib>

#include <memory>

#include <ert/util/test_util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_conn.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();
    {
        double CF = 88;
        int segment_id = 78;
        int outlet_segment_id = 100;
        int branch_nr = WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE;
        WellSegment ws(segment_id, outlet_segment_id, branch_nr);
        auto conn1 =
            std::make_shared<WellConnection>(1, 1, 1, CF, WellConnDir::X, true,
                                             segment_id, true, RD_METRIC_UNITS);
        auto conn2 = std::make_shared<WellConnection>(
            1, 1, 1, CF, WellConnDir::X, true, segment_id + 1, true,
            RD_METRIC_UNITS);

        test_assert_false(ws.has_global_grid_connections());

        test_assert_true(ws.add_connection(RD_GRID_GLOBAL_GRID, conn1));
        test_assert_false(ws.add_connection(RD_GRID_GLOBAL_GRID, conn2));

        test_assert_true(ws.has_grid_connections(RD_GRID_GLOBAL_GRID));
        test_assert_true(ws.has_global_grid_connections());
        test_assert_false(ws.has_grid_connections("DoesNotExist"));

        test_assert_NULL(ws.get_connections("doesNotExist"));
    }
    exit(0);
}
