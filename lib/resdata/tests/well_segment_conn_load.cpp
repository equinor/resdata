#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_conn_collection.hpp>
#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment_collection.hpp>

int main(int argc, char **argv) {
    const char *Xfile = argv[1];
    rd_file_type *rst_file = rd_file_open(Xfile, 0);
    rd_rsthead_type *rst_head = rd_rsthead_alloc(rst_file);
    const rd_kw_type *iwel_kw = rd_file_iget_named_kw(rst_file, IWEL_KW, 0);
    const rd_kw_type *iseg_kw = rd_file_iget_named_kw(rst_file, ISEG_KW, 0);
    const rd_kw_type *rseg_kw = rd_file_iget_named_kw(rst_file, RSEG_KW, 0);
    const rd_kw_type *icon_kw = rd_file_iget_named_kw(rst_file, ICON_KW, 0);

    test_install_SIGNALS();
    test_assert_not_NULL(rst_file);
    test_assert_not_NULL(rst_head);
    {
        int well_nr;
        for (well_nr = 0; well_nr < rst_head->nwells; well_nr++) {
            well_conn_collection_type *connections =
                well_conn_collection_alloc();
            well_conn_collection_load_from_kw(connections, iwel_kw, icon_kw,
                                              well_nr, rst_head);
            {
                well_segment_collection_type *segments =
                    well_segment_collection_alloc();

                if (well_segment_collection_load_from_kw(segments, well_nr,
                                                         iwel_kw, iseg_kw,
                                                         rseg_kw, rst_head)) {
                    well_branch_collection_type *branches =
                        well_branch_collection_alloc();

                    test_assert_true(
                        well_segment_well_is_MSW(well_nr, iwel_kw, rst_head));
                    well_segment_collection_link(segments);
                    {
                        int is;
                        for (is = 0;
                             is < well_segment_collection_get_size(segments);
                             is++) {
                            well_segment_type *segment =
                                well_segment_collection_iget(segments, is);

                            if (well_segment_nearest_wellhead(segment))
                                test_assert_NULL(
                                    well_segment_get_outlet(segment));
                            else
                                test_assert_not_NULL(
                                    well_segment_get_outlet(segment));

                            test_assert_ptr_not_equal(
                                segment, well_segment_get_outlet(segment));
                        }
                    }

                    well_segment_collection_add_branches(segments, branches);
                    {
                        int ib;
                        for (ib = 0;
                             ib < well_branch_collection_get_size(branches);
                             ib++) {
                            const well_segment_type *start_segment =
                                well_branch_collection_iget_start_segment(
                                    branches, ib);
                            const well_segment_type *segment = start_segment;

                            printf("Branch %d/%d ", ib,
                                   well_branch_collection_get_size(branches));
                            while (segment) {
                                printf("[%p]%d -> \n", segment,
                                       well_segment_get_id(segment));
                                segment = well_segment_get_outlet(segment);
                            }
                            printf("\n");
                            sleep(1);
                        }
                    }
                    well_segment_collection_add_connections(
                        segments, RD_GRID_GLOBAL_GRID, connections);
                    well_branch_collection_free(branches);
                } else
                    test_assert_false(
                        well_segment_well_is_MSW(well_nr, iwel_kw, rst_head));

                well_segment_collection_free(segments);
            }
            well_conn_collection_free(connections);
        }
    }

    rd_file_close(rst_file);
    rd_rsthead_free(rst_head);
    exit(0);
}
