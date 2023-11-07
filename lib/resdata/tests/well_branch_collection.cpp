#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>

#include <resdata/well/well_branch_collection.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_const.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();

    well_branch_collection_type *branches = well_branch_collection_alloc();
    double *rseg_data = (double *)util_calloc(100, sizeof *rseg_data);
    const double depth = 100;
    const double length = 20;
    const double total_length = 200;
    const double diameter = 10;

    rseg_data[RSEG_DEPTH_INDEX] = depth;
    rseg_data[RSEG_LENGTH_INDEX] = length;
    rseg_data[RSEG_TOTAL_LENGTH_INDEX] = total_length;
    rseg_data[RSEG_DIAMETER_INDEX] = diameter;

    test_assert_true(well_branch_collection_is_instance(branches));
    test_assert_int_equal(well_branch_collection_get_size(branches), 0);
    test_assert_NULL(well_branch_collection_iget_start_segment(branches, 0));
    test_assert_NULL(well_branch_collection_get_start_segment(branches, 0));
    test_assert_false(well_branch_collection_has_branch(branches, 0));
    {
        well_segment_type *segment1 =
            well_segment_alloc(189, 99, 78, rseg_data);
        well_segment_type *segment2 =
            well_segment_alloc(200, 189, 78, rseg_data);

        test_assert_false(
            well_branch_collection_add_start_segment(branches, segment1));

        test_assert_true(well_segment_link(segment2, segment1));
        test_assert_true(
            well_branch_collection_add_start_segment(branches, segment2));

        test_assert_int_equal(well_branch_collection_get_size(branches), 1);
        test_assert_true(well_segment_is_instance(
            well_branch_collection_iget_start_segment(branches, 0)));
        test_assert_true(well_segment_is_instance(
            well_branch_collection_get_start_segment(branches, 78)));
        test_assert_true(well_branch_collection_has_branch(branches, 78));

        well_segment_free(segment2);
        well_segment_free(segment1);
    }
    free(rseg_data);
    well_branch_collection_free(branches);

    exit(0);
}
