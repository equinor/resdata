#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_conn_collection.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();
    double *rseg_data = (double *)util_calloc(100, sizeof *rseg_data);
    {
        double CF = 88;
        int segment_id = 78;
        int outlet_segment_id = 100;
        int branch_nr = WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE;
        well_segment_type *ws = well_segment_alloc(
            segment_id, outlet_segment_id, branch_nr, rseg_data);
        well_conn_type *conn1 =
            well_conn_alloc_MSW(1, 1, 1, CF, well_conn_dirX, true, segment_id);
        well_conn_type *conn2 = well_conn_alloc_MSW(1, 1, 1, CF, well_conn_dirX,
                                                    true, segment_id + 1);

        test_assert_false(well_segment_has_global_grid_connections(ws));

        test_assert_true(
            well_segment_add_connection(ws, RD_GRID_GLOBAL_GRID, conn1));
        test_assert_false(
            well_segment_add_connection(ws, RD_GRID_GLOBAL_GRID, conn2));

        test_assert_true(
            well_segment_has_grid_connections(ws, RD_GRID_GLOBAL_GRID));
        test_assert_true(well_segment_has_global_grid_connections(ws));
        test_assert_false(
            well_segment_has_grid_connections(ws, "DoesNotExist"));

        test_assert_true(well_conn_collection_is_instance(
            well_segment_get_connections(ws, RD_GRID_GLOBAL_GRID)));
        test_assert_true(well_conn_collection_is_instance(
            well_segment_get_global_connections(ws)));
        test_assert_NULL(well_segment_get_connections(ws, "doesNotExist"));

        well_conn_free(conn1);
        well_conn_free(conn2);
        well_segment_free(ws);
    }
    free(rseg_data);
    exit(0);
}
