#include <cstdlib>
#include <memory>

#include <ert/util/test_util.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_segment.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();

    well_segment_collection_type *sc = well_segment_collection_alloc();
    test_assert_not_NULL(sc);
    test_assert_int_equal(well_segment_collection_get_size(sc), 0);

    {
        int outlet_segment_id = WELL_SEGMENT_OUTLET_END_VALUE;
        int branch_nr = WELL_SEGMENT_BRANCH_INACTIVE_VALUE;
        auto ws =
            std::make_shared<WellSegment>(89, outlet_segment_id, branch_nr);

        well_segment_collection_add(sc, ws);
        test_assert_int_equal(well_segment_collection_get_size(sc), 1);
        test_assert_ptr_equal(well_segment_collection_iget(sc, 0).get(),
                              ws.get());

        test_assert_false(well_segment_collection_has_segment(sc, 451));
        test_assert_true(well_segment_collection_has_segment(sc, 89));
        test_assert_ptr_equal(well_segment_collection_get(sc, 89).get(),
                              ws.get());
    }

    {
        int outlet_segment_id = WELL_SEGMENT_OUTLET_END_VALUE;
        int branch_nr = WELL_SEGMENT_BRANCH_INACTIVE_VALUE;
        auto ws =
            std::make_shared<WellSegment>(90, outlet_segment_id, branch_nr);

        well_segment_collection_add(sc, ws);
        test_assert_int_equal(well_segment_collection_get_size(sc), 2);
        test_assert_ptr_equal(well_segment_collection_iget(sc, 1).get(),
                              ws.get());

        test_assert_false(well_segment_collection_has_segment(sc, 451));
        test_assert_true(well_segment_collection_has_segment(sc, 89));
        test_assert_true(well_segment_collection_has_segment(sc, 90));
        test_assert_ptr_equal(well_segment_collection_get(sc, 90).get(),
                              ws.get());
        test_assert_NULL(well_segment_collection_get(sc, 76).get());
    }

    {
        int outlet_segment_id = WELL_SEGMENT_OUTLET_END_VALUE;
        int branch_nr = WELL_SEGMENT_BRANCH_INACTIVE_VALUE;
        auto ws =
            std::make_shared<WellSegment>(89, outlet_segment_id, branch_nr);

        well_segment_collection_add(sc, ws);
        test_assert_int_equal(well_segment_collection_get_size(sc), 2);
        test_assert_ptr_equal(well_segment_collection_iget(sc, 0).get(),
                              ws.get());

        test_assert_false(well_segment_collection_has_segment(sc, 451));
        test_assert_true(well_segment_collection_has_segment(sc, 89));
        test_assert_ptr_equal(well_segment_collection_get(sc, 89).get(),
                              ws.get());
    }

    well_segment_collection_free(sc);

    exit(0);
}
