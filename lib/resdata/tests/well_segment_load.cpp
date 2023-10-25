#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw_magic.hpp>

#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_rseg_loader.hpp>

int main(int argc, char **argv) {
    const char *Xfile = argv[1];
    rd_file_type *rst_file = rd_file_open(Xfile, 0);
    rd_file_view_type *rst_view = rd_file_get_active_view(rst_file);
    rd_rsthead_type *rst_head =
        rd_rsthead_alloc(rst_view, rd_filename_report_nr(Xfile));
    const rd_kw_type *iwel_kw = rd_file_iget_named_kw(rst_file, IWEL_KW, 0);
    const rd_kw_type *iseg_kw = rd_file_iget_named_kw(rst_file, ISEG_KW, 0);
    well_rseg_loader_type *rseg_loader = well_rseg_loader_alloc(rst_view);

    test_install_SIGNALS();
    test_assert_not_NULL(rst_file);
    test_assert_not_NULL(rst_head);

    {
        int well_nr;

        for (well_nr = 0; well_nr < rst_head->nwells; well_nr++) {
            int iwel_offset = rst_head->niwelz * well_nr;
            well_segment_collection_type *segments =
                well_segment_collection_alloc();
            int seg_well_nr =
                rd_kw_iget_int(iwel_kw,
                               iwel_offset + IWEL_SEGMENTED_WELL_NR_INDEX) -
                1; // -1: Ordinary well.
            if (seg_well_nr >= 0) {
                int segment_index;
                int segment_count = 0;

                for (segment_index = 0; segment_index < rst_head->nsegmx;
                     segment_index++) {
                    int segment_id = segment_index + WELL_SEGMENT_OFFSET;
                    well_segment_type *segment = well_segment_alloc_from_kw(
                        iseg_kw, rseg_loader, rst_head, seg_well_nr,
                        segment_index, segment_id);

                    test_assert_true(well_segment_is_instance(segment));

                    if (well_segment_active(segment)) {
                        well_segment_collection_add(segments, segment);
                        test_assert_int_equal(
                            well_segment_collection_get_size(segments),
                            segment_count + 1);
                        test_assert_ptr_equal(well_segment_collection_iget(
                                                  segments, segment_count),
                                              segment);
                        segment_count++;
                    } else
                        well_segment_free(segment);
                }
            }

            {
                well_segment_collection_type *segments2 =
                    well_segment_collection_alloc();
                bool load_segments = true;
                bool is_MSW_well = false;
                test_assert_int_equal(
                    well_segment_collection_load_from_kw(
                        segments2, well_nr, iwel_kw, iseg_kw, rseg_loader,
                        rst_head, load_segments, &is_MSW_well),
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
    }
    rd_file_close(rst_file);
    rd_rsthead_free(rst_head);
    exit(0);
}
