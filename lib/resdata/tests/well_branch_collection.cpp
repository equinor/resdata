#include <cstdlib>

#include <memory>

#include <ert/util/test_util.hpp>

#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_branch_collection.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_const.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();

    well_branch_collection_type *branches = well_branch_collection_alloc();
    const double depth = 100;
    const double length = 20;
    const double total_length = 200;
    const double diameter = 10;

    test_assert_true(well_branch_collection_is_instance(branches));
    test_assert_int_equal(well_branch_collection_get_size(branches), 0);
    test_assert_NULL(
        well_branch_collection_iget_start_segment(branches, 0).get());
    test_assert_NULL(
        well_branch_collection_get_start_segment(branches, 0).get());
    test_assert_false(well_branch_collection_has_branch(branches, 0));
    {
        auto segment1 = std::make_shared<WellSegment>(
            189, 99, 78, depth, length, total_length, diameter);
        auto segment2 = std::make_shared<WellSegment>(
            200, 189, 78, depth, length, total_length, diameter);

        test_assert_false(
            well_branch_collection_add_start_segment(branches, segment1));

        test_assert_true(segment2->link(segment1.get()));
        test_assert_true(
            well_branch_collection_add_start_segment(branches, segment2));

        test_assert_int_equal(well_branch_collection_get_size(branches), 1);
        test_assert_true(well_branch_collection_has_branch(branches, 78));
    }
    well_branch_collection_free(branches);

    exit(0);
}
