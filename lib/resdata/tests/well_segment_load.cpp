#include <cstdlib>

#include <ert/util/test_util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>

#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_rseg_loader.hpp>

int main(int argc, char **argv) {
    const char *Xfile = argv[1];
    auto rst_file = rd::File::open(Xfile);
    auto rst_view = rd_file_get_active_view(rst_file.get());
    auto rst_head = RSTHead::read(rst_view.get(), rd_filename_report_nr(Xfile));
    const rd_kw_type *iwel_kw =
        rd_file_iget_named_kw(rst_file.get(), IWEL_KW, 0);
    const rd_kw_type *iseg_kw =
        rd_file_iget_named_kw(rst_file.get(), ISEG_KW, 0);
    well_rseg_loader_type *rseg_loader = well_rseg_loader_alloc(rst_view.get());

    test_install_SIGNALS();
    test_assert_not_NULL(rst_file.get());

    for (int well_nr = 0; well_nr < rst_head.nwells; well_nr++) {
        int iwel_offset = rst_head.niwelz * well_nr;
        well_segment_collection_type *segments =
            well_segment_collection_alloc();
        int seg_well_nr =
            rd_kw_iget_int(iwel_kw,
                           iwel_offset + IWEL_SEGMENTED_WELL_NR_INDEX) -
            1; // -1: Ordinary well.
        if (seg_well_nr >= 0) {
            int segment_count = 0;

            for (int segment_index = 0; segment_index < rst_head.nsegmx;
                 segment_index++) {
                int segment_id = segment_index + WELL_SEGMENT_OFFSET;
                auto segment = WellSegment::from_kw(iseg_kw, rseg_loader,
                                                    rst_head, seg_well_nr,
                                                    segment_index, segment_id);

                if (segment->is_active()) {
                    well_segment_collection_add(segments, segment);
                    test_assert_int_equal(
                        well_segment_collection_get_size(segments),
                        segment_count + 1);
                    test_assert_ptr_equal(
                        well_segment_collection_iget(segments, segment_count)
                            .get(),
                        segment.get());
                    segment_count++;
                }
            }
        }

        {
            well_segment_collection_type *segments2 =
                well_segment_collection_alloc();
            bool load_segments = true;
            bool is_MSW_well = false;
            test_assert_int_equal(well_segment_collection_load_from_kw(
                                      segments2, well_nr, iwel_kw, iseg_kw,
                                      rseg_loader, rst_head, load_segments,
                                      &is_MSW_well),
                                  well_segment_collection_get_size(segments));

            if (well_segment_collection_get_size(segments) > 0)
                test_assert_true(is_MSW_well);
            else
                test_assert_false(is_MSW_well);

            well_segment_collection_link(segments);
            well_segment_collection_link(segments2);
            well_segment_collection_free(segments);
            well_segment_collection_free(segments2);
        }
    }
    exit(0);
}
