#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw.hpp>

#include <resdata/well/well_conn.hpp>
#include <string>

void test_rstfile(const char *filename, bool fracture_connection) {
    auto rst_file = open_rd_file(std::string(filename));
    const rd_kw_type *iwel_kw =
        rd_file_iget_named_kw(rst_file.get(), IWEL_KW, 0);
    auto header = RSTHead::read(rd_file_get_global_view(rst_file.get()).get(),
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
}

int main(int argc, char **argv) {
    const char *matrix_rstfile = argv[1];
    const char *fracture_rstfile = argv[2];

    test_rstfile(fracture_rstfile, true);
    test_rstfile(matrix_rstfile, false);

    exit(0);
}
