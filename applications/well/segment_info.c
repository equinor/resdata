/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'segment_info.c' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <ert/util/stringlist.hpp>
#include <ert/util/test_util.hpp>

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
    fprintf(
        stderr,
        "** Warning: segment_info is deprecated. Use res2df rft instead.\n");
    const char *Xfile = argv[1];
    rd_file_type *rst_file = rd_file_open(Xfile, 0);
    rd_rsthead_type *rst_head = rd_rsthead_alloc(
        rd_file_get_global_view(rst_file), rd_filename_report_nr(Xfile));
    const rd_kw_type *iwel_kw = rd_file_iget_named_kw(rst_file, IWEL_KW, 0);
    const rd_kw_type *iseg_kw = rd_file_iget_named_kw(rst_file, ISEG_KW, 0);
    well_rseg_loader_type *rseg_loader =
        well_rseg_loader_alloc(rd_file_get_global_view(rst_file));
    const rd_kw_type *icon_kw = rd_file_iget_named_kw(rst_file, ICON_KW, 0);
    const rd_kw_type *scon_kw = rd_file_iget_named_kw(rst_file, SCON_KW, 0);
    const rd_kw_type *xcon_kw = rd_file_iget_named_kw(rst_file, XCON_KW, 0);
    const rd_kw_type *zwel_kw = rd_file_iget_named_kw(rst_file, ZWEL_KW, 0);

    {
        int well_nr;
        for (well_nr = 0; well_nr < rst_head->nwells; well_nr++) {
            const int zwel_offset = rst_head->nzwelz * well_nr;
            char *well_name =
                util_alloc_strip_copy(rd_kw_iget_ptr(zwel_kw, zwel_offset));

            printf("==========================================================="
                   "======\n");
            printf("Well: %s ", well_name);
            {
                well_conn_collection_type *connections =
                    well_conn_collection_alloc();
                well_segment_collection_type *segments =
                    well_segment_collection_alloc();
                bool load_segment_information = true;
                bool is_MSW_well = false;

                if (well_segment_collection_load_from_kw(
                        segments, well_nr, iwel_kw, iseg_kw, rseg_loader,
                        rst_head, load_segment_information, &is_MSW_well)) {
                    well_branch_collection_type *branches =
                        well_branch_collection_alloc();

                    well_conn_collection_load_from_kw(connections, iwel_kw,
                                                      icon_kw, scon_kw, xcon_kw,
                                                      well_nr, rst_head);
                    well_segment_collection_link(segments);
                    well_segment_collection_add_branches(segments, branches);
                    well_segment_collection_add_connections(
                        segments, RD_GRID_GLOBAL_GRID, connections);

                    printf("\n");
                    printf("Segments:\n");
                    {
                        int is;
                        for (is = 0;
                             is < well_segment_collection_get_size(segments);
                             is++) {
                            well_segment_type *segment =
                                well_segment_collection_iget(segments, is);
                            printf("-------------------------------------------"
                                   "----------------------\n");
                            printf("ID          : %d \n",
                                   well_segment_get_id(segment));
                            printf("Outlet      : %d \n",
                                   well_segment_get_outlet_id(segment));
                            printf("Branch      : %d \n",
                                   well_segment_get_branch_id(segment));
                            printf("Connections : [");
                            {
                                const well_conn_collection_type *connections =
                                    well_segment_get_global_connections(
                                        segment);
                                if (connections) {
                                    int ic;
                                    for (ic = 0;
                                         ic < well_conn_collection_get_size(
                                                  connections);
                                         ic++) {
                                        const well_conn_type *conn =
                                            well_conn_collection_iget(
                                                connections, ic);
                                        printf("(%d , %d , %d)  ",
                                               well_conn_get_i(conn),
                                               well_conn_get_j(conn),
                                               well_conn_get_k(conn));
                                    }
                                }
                            }

                            printf("]\n");
                        }
                    }
                    well_branch_collection_free(branches);
                } else
                    printf("not MSW well\n\n");

                well_conn_collection_free(connections);
                well_segment_collection_free(segments);
            }
        }
    }

    rd_file_close(rst_file);
    rd_rsthead_free(rst_head);
}
