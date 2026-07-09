#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_rsthead.hpp>

#include <resdata/well/well_conn.hpp>

void test_rstfile(const char *filename, bool fracture_connection) {
    rd_file_type *rst_file = rd_file_open(filename);
    const rd_kw_type *iwel_kw = rd_file_iget_named_kw(rst_file, IWEL_KW, 0);
    auto header = RSTHead::read(rd_file_get_global_view(rst_file),
                                rd_filename_report_nr(filename));

    auto wellhead = WellConnection::read_wellhead(iwel_kw, header, 0);

    if (fracture_connection) {
        test_assert_true(wellhead->is_fracture_connection());
        test_assert_false(wellhead->is_matrix_connection());
    } else {
        test_assert_false(wellhead->is_fracture_connection());
        test_assert_true(wellhead->is_matrix_connection());
    }

    test_assert_true(wellhead->get_k() < header.nz);

    rd_file_close(rst_file);
}

int main(int argc, char **argv) {
    const char *matrix_rstfile = argv[1];
    const char *fracture_rstfile = argv[2];

    test_rstfile(fracture_rstfile, true);
    test_rstfile(matrix_rstfile, false);

    exit(0);
}
