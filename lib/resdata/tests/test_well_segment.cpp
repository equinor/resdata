#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>

#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_const.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();
    const double depth = 100;
    const double length = 20;
    const double total_length = 200;
    const double diameter = 10;
    {
        int segment_id = 78;
        int outlet_segment_id = 100;
        int branch_nr = WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE;
        WellSegment ws(segment_id, outlet_segment_id, branch_nr, depth, length,
                       total_length, diameter);

        test_assert_int_equal(0, ws.get_link_count());
        test_assert_NULL(ws.get_outlet());
        test_assert_int_equal(ws.get_outlet_id(), outlet_segment_id);
        test_assert_int_equal(ws.get_branch_id(), branch_nr);
        test_assert_int_equal(ws.get_id(), segment_id);

        test_assert_false(ws.is_nearest_wellhead());
        test_assert_true(ws.is_active());
        test_assert_true(ws.is_main_stem());

        test_assert_double_equal(depth, ws.get_depth());
        test_assert_double_equal(length, ws.get_length());
        test_assert_double_equal(total_length, ws.get_total_length());
        test_assert_double_equal(diameter, ws.get_diameter());
    }

    {
        int outlet_segment_id = WELL_SEGMENT_OUTLET_END_VALUE;
        int branch_nr = 100;
        WellSegment ws(12, outlet_segment_id, branch_nr, depth, length,
                       total_length, diameter);

        test_assert_true(ws.is_nearest_wellhead());
        test_assert_false(ws.is_main_stem());
    }

    {
        int outlet_segment_id = WELL_SEGMENT_OUTLET_END_VALUE;
        int branch_nr = WELL_SEGMENT_BRANCH_INACTIVE_VALUE;
        WellSegment ws(89, outlet_segment_id, branch_nr, depth, length,
                       total_length, diameter);

        test_assert_false(ws.is_active());
    }

    {
        int branch_nr = WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE;
        int outlet_id = 0;
        WellSegment outlet(outlet_id, WELL_SEGMENT_OUTLET_END_VALUE, branch_nr,
                           depth, length, total_length, diameter);
        WellSegment ws(100, outlet_id, branch_nr, depth, length, total_length,
                       diameter);

        test_assert_true(ws.link(&outlet));
        test_assert_ptr_equal(ws.get_outlet(), &outlet);
        test_assert_int_equal(outlet.get_link_count(), 1);
        test_assert_ptr_not_equal(&ws, ws.get_outlet());
    }

    {
        int branch_nr = WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE;
        int outlet_id = 0;
        WellSegment outlet(outlet_id, WELL_SEGMENT_OUTLET_END_VALUE, branch_nr,
                           depth, length, total_length, diameter);
        WellSegment ws(100, outlet_id + 1, branch_nr, depth, length,
                       total_length, diameter);

        test_assert_false(ws.link(&outlet));
        test_assert_NULL(ws.get_outlet());
        test_assert_int_equal(outlet.get_link_count(), 0);
    }
    exit(0);
}
