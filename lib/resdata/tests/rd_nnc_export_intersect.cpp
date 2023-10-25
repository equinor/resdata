// #include <stdlib.h>
// #include <stdbool.h>
#include <signal.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_nnc_export.hpp>
#include <resdata/rd_endian_flip.h>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_nnc_data.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>

#include <ert/util/ert_unique_ptr.hpp>
#include <ert/util/util.hpp>

namespace {

const auto GRIDX = 10, GRIDY = 10, GRIDZ = 10;
const auto GRID_NNC_NUM = 342;
const auto INIT_NNC_NUM = 298;

ERT::ert_unique_ptr<rd_grid_type, rd_grid_free> make_intersect_grid() {
    auto out = ERT::ert_unique_ptr<rd_grid_type, rd_grid_free>(
        rd_grid_alloc_rectangular(GRIDX, GRIDY, GRIDZ, 1., 1., 1., nullptr));
    for (auto i = 0; i < GRID_NNC_NUM; ++i)
        rd_grid_add_self_nnc(out.get(), 2 * i + 1, 2 * i, i);
    return out;
}

ERT::ert_unique_ptr<rd_file_type, rd_file_close> make_intersect_init_file() {
    // Create keywords with useless data
    auto nnc1_kw = rd_kw_alloc(NNC1_KW, INIT_NNC_NUM, RD_INT);
    auto nnc2_kw = rd_kw_alloc(NNC2_KW, INIT_NNC_NUM, RD_INT);
    auto tran_kw = rd_kw_alloc(TRANNNC_KW, INIT_NNC_NUM, RD_DOUBLE);
    for (auto i = 0; i < INIT_NNC_NUM; ++i) {
        rd_kw_iset_int(nnc1_kw, i, 2 * i);
        rd_kw_iset_int(nnc2_kw, i, 2 * i + 1);
        rd_kw_iset_double(tran_kw, i, 2.5 * i);
    }

    // write to file directly using fortio
    auto init_filename =
        util_alloc_tmp_file("/tmp", "rd_nnc_export_intersect_init_file", false);
    auto fortio = fortio_open_writer(init_filename, false, RD_ENDIAN_FLIP);
    rd_kw_fwrite(nnc1_kw, fortio);
    rd_kw_fwrite(nnc2_kw, fortio);
    rd_kw_fwrite(tran_kw, fortio);
    fortio_fclose(fortio);

    // reopen the file as an ecl file
    auto out = ERT::ert_unique_ptr<rd_file_type, rd_file_close>(
        rd_file_open(init_filename, 0));

    rd_kw_free(nnc1_kw);
    rd_kw_free(nnc2_kw);
    rd_kw_free(tran_kw);
    free(init_filename);
    return out;
}
} /* unnamed namespace */

int main(int argc, char **argv) {
    util_install_signals();

    const auto grid = make_intersect_grid();
    const auto init_file = make_intersect_init_file();

    test_assert_true(rd_nnc_intersect_format(grid.get(), init_file.get()));
    test_assert_int_equal(rd_nnc_export_get_size(grid.get(), init_file.get()),
                          INIT_NNC_NUM);

    auto nnc_data = std::vector<rd_nnc_type>(
        rd_nnc_export_get_size(grid.get(), init_file.get()));
    auto const total_valid_trans =
        rd_nnc_export(grid.get(), init_file.get(), nnc_data.data());
    test_assert_int_equal(total_valid_trans, INIT_NNC_NUM);
    test_assert_int_equal(int(nnc_data.size()), INIT_NNC_NUM);

    for (auto i = 0; i < int(nnc_data.size()); ++i) {
        auto const &nnc = nnc_data[i];
        test_assert_int_equal(nnc.grid_nr1, 0);
        test_assert_int_equal(nnc.grid_nr2, 0);
        test_assert_int_equal(nnc.global_index1,
                              2 * i); // as set in make_intersect_init_file()
        test_assert_int_equal(nnc.global_index2,
                              2 * i +
                                  1); // as set in make_intersect_init_file()
        test_assert_double_equal(
            nnc.trans, 2.5 * i); // as set in make_intersect_init_file()
        test_assert_int_equal(nnc.input_index, i);
    }

    return 0;
}
