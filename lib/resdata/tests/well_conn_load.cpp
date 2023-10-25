#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw_magic.hpp>

#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_conn_collection.hpp>
#include <resdata/well/well_const.hpp>

int main(int argc, char **argv) {
    const char *Xfile = argv[1];
    bool MSW;
    rd_file_type *rst_file = rd_file_open(Xfile, 0);
    rd_rsthead_type *rst_head = rd_rsthead_alloc(
        rd_file_get_global_view(rst_file), rd_filename_report_nr(Xfile));

    test_install_SIGNALS();
    test_assert_true(util_sscanf_bool(argv[2], &MSW));
    test_assert_not_NULL(rst_file);
    test_assert_not_NULL(rst_head);

    {
        int iwell;
        const rd_kw_type *iwel_kw = rd_file_iget_named_kw(rst_file, IWEL_KW, 0);
        const rd_kw_type *icon_kw = rd_file_iget_named_kw(rst_file, ICON_KW, 0);
        rd_kw_type *scon_kw = NULL;
        rd_kw_type *xcon_kw = NULL;

        bool caseMSW = false;

        for (iwell = 0; iwell < rst_head->nwells; iwell++) {
            const int iwel_offset = rst_head->niwelz * iwell;
            int num_connections =
                rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_CONNECTIONS_INDEX);
            int iconn;
            well_conn_collection_type *wellcc = well_conn_collection_alloc();
            well_conn_collection_type *wellcc_ref =
                well_conn_collection_alloc();

            for (iconn = 0; iconn < num_connections; iconn++) {
                well_conn_type *conn = well_conn_alloc_from_kw(
                    icon_kw, scon_kw, xcon_kw, rst_head, iwell, iconn);

                test_assert_true(well_conn_is_instance(conn));
                test_assert_not_NULL(conn);
                if (!MSW)
                    test_assert_bool_equal(well_conn_MSW(conn), MSW);
                else
                    caseMSW |= well_conn_MSW(conn);

                well_conn_collection_add(wellcc, conn);
                well_conn_collection_add_ref(wellcc_ref, conn);
                test_assert_int_equal(iconn + 1,
                                      well_conn_collection_get_size(wellcc));
                test_assert_ptr_equal(
                    well_conn_collection_iget_const(wellcc, iconn), conn);
                test_assert_ptr_equal(
                    well_conn_collection_iget_const(wellcc_ref, iconn), conn);
            }
            well_conn_collection_free(wellcc_ref);
            {

                int i;
                for (i = 0; i < well_conn_collection_get_size(wellcc); i++)
                    test_assert_true(well_conn_is_instance(
                        well_conn_collection_iget_const(wellcc, i)));
            }
            {
                well_conn_collection_type *wellcc2 =
                    well_conn_collection_alloc();
                int i;

                test_assert_int_equal(well_conn_collection_get_size(wellcc),
                                      well_conn_collection_load_from_kw(
                                          wellcc2, iwel_kw, icon_kw, scon_kw,
                                          xcon_kw, iwell, rst_head));

                for (i = 0; i < well_conn_collection_get_size(wellcc2); i++) {
                    test_assert_true(well_conn_is_instance(
                        well_conn_collection_iget_const(wellcc2, i)));
                    test_assert_true(well_conn_equal(
                        well_conn_collection_iget_const(wellcc2, i),
                        well_conn_collection_iget_const(wellcc, i)));
                }
                well_conn_collection_free(wellcc2);
            }
            well_conn_collection_free(wellcc);
        }
        test_assert_bool_equal(caseMSW, MSW);
    }

    exit(0);
}
