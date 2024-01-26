#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw_magic.hpp>

#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_conn_collection.hpp>
#include <resdata/well/well_const.hpp>

void well_conn_test_CF(const rd_kw_type *iwel_kw, const rd_kw_type *icon_kw,
                       const rd_kw_type *scon_kw, const rd_kw_type *xcon_kw,
                       const rd_rsthead_type *rst_head, int iwell, int iconn,
                       double CF) {
    well_conn_type *conn = well_conn_alloc_from_kw(icon_kw, scon_kw, xcon_kw,
                                                   rst_head, iwell, iconn);
    test_assert_double_equal(CF, well_conn_get_connection_factor(conn));
    well_conn_free(conn);
}

int main(int argc, char **argv) {
    const char *Xfile = argv[1];
    rd_file_type *rst_file = rd_file_open(Xfile, 0);
    rd_rsthead_type *rst_head = rd_rsthead_alloc(
        rd_file_get_global_view(rst_file), rd_filename_report_nr(Xfile));
    const rd_kw_type *iwel_kw = rd_file_iget_named_kw(rst_file, IWEL_KW, 0);
    const rd_kw_type *icon_kw = rd_file_iget_named_kw(rst_file, ICON_KW, 0);
    const rd_kw_type *scon_kw = rd_file_iget_named_kw(rst_file, SCON_KW, 0);
    const rd_kw_type *xcon_kw = 0;

    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 0, 0,
                      32.948);
    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 0, 1,
                      46.825);
    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 0, 2,
                      51.867);

    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 1, 0,
                      1.168);
    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 1, 1,
                      15.071);
    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 1, 2,
                      6.242);

    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 2, 0,
                      27.412);
    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 2, 1,
                      55.195);
    well_conn_test_CF(iwel_kw, icon_kw, scon_kw, xcon_kw, rst_head, 2, 2,
                      18.032);

    rd_file_close(rst_file);
    rd_rsthead_free(rst_head);
    exit(0);
}
