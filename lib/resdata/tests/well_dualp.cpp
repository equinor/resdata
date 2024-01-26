#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_rsthead.hpp>

#include <resdata/well/well_conn.hpp>

void test_rstfile(const char *filename, bool fracture_connection) {
    rd_file_type *rst_file = rd_file_open(filename, 0);
    const rd_kw_type *iwel_kw = rd_file_iget_named_kw(rst_file, IWEL_KW, 0);
    rd_rsthead_type *header = rd_rsthead_alloc(
        rd_file_get_global_view(rst_file), rd_filename_report_nr(filename));

    well_conn_type *wellhead = well_conn_alloc_wellhead(iwel_kw, header, 0);

    if (fracture_connection) {
        test_assert_true(well_conn_fracture_connection(wellhead));
        test_assert_false(well_conn_matrix_connection(wellhead));
    } else {
        test_assert_false(well_conn_fracture_connection(wellhead));
        test_assert_true(well_conn_matrix_connection(wellhead));
    }

    test_assert_true(well_conn_get_k(wellhead) < header->nz);

    rd_rsthead_free(header);
    well_conn_free(wellhead);
    rd_file_close(rst_file);
}

int main(int argc, char **argv) {
    const char *matrix_rstfile = argv[1];
    const char *fracture_rstfile = argv[2];

    test_rstfile(fracture_rstfile, true);
    test_rstfile(matrix_rstfile, false);

    exit(0);
}
