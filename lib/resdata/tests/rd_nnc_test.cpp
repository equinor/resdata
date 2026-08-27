#include <cstdio>
#include <cstdlib>

#include <memory>
#include <vector>
#include <algorithm>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/nnc_info.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>

static bool contains(const std::vector<size_t> &vec, int value) {
    if (value < 0)
        return false;
    return std::find(vec.begin(), vec.end(), static_cast<size_t>(value)) !=
           vec.end();
}

void test_scan(const char *grid_filename) {
    rd_grid_type *rd_grid = rd_grid_alloc(grid_filename);
    std::unique_ptr<rd::File> grid_file = rd::File::open(grid_filename);

    for (size_t block_nr = 0; block_nr < grid_file->num_named_kw(NNCHEAD_KW);
         block_nr++) {
        rd_grid_type *lgr = rd_grid;
        size_t lgr_nr;
        auto nnc_view = grid_file->blockview(NNCHEAD_KW, block_nr);
        {
            if (block_nr > 0)
                lgr = rd_grid_iget_lgr(rd_grid, block_nr - 1);
            lgr_nr = rd_grid_get_lgr_nr(lgr);

            /* Internal nnc */
            {
                if (nnc_view->has_kw(NNC1_KW)) {
                    rd_kw_type *nnc1_kw = nnc_view->get_kw(NNC1_KW, 0);
                    rd_kw_type *nnc2_kw = nnc_view->get_kw(NNC2_KW, 0);
                    size_t i;
                    for (i = 0; i < rd_kw_get_size(nnc1_kw); i++) {
                        const int g1 = rd_kw_iget_int(nnc1_kw, i) - 1;
                        const int g2 = rd_kw_iget_int(nnc2_kw, i) - 1;

                        // Skipping matrix <-> fracture link in dual poro.
                        if (g2 >= 0 && static_cast<size_t>(g2) <
                                           rd_grid_get_global_size(lgr)) {
                            const nnc_info_type *nnc_info =
                                rd_grid_get_cell_nnc_info1(
                                    lgr, static_cast<size_t>(g1));
                            const std::vector<size_t> &index_list =
                                nnc_info_get_grid_index_list(nnc_info, lgr_nr);
                            test_assert_not_NULL(nnc_info);
                            test_assert_true(contains(index_list, g2));
                        }
                    }
                }
            }
        }

        /* Global -> lgr */
        {
            if (nnc_view->has_kw(NNCG_KW)) {
                rd_kw_type *nnchead_kw = nnc_view->get_kw(NNCHEAD_KW, 0);
                rd_kw_type *nncg_kw = nnc_view->get_kw(NNCG_KW, 0);
                rd_kw_type *nncl_kw = nnc_view->get_kw(NNCL_KW, 0);
                size_t lgr_nr = static_cast<size_t>(
                    rd_kw_iget_int(nnchead_kw, NNCHEAD_LGR_INDEX));
                for (size_t i = 0; i < rd_kw_get_size(nncg_kw); i++) {
                    const int g = rd_kw_iget_int(nncg_kw, i) - 1;
                    const int l =
                        rd_kw_iget_int(nncl_kw, static_cast<size_t>(i)) - 1;

                    const nnc_info_type *nnc_info = rd_grid_get_cell_nnc_info1(
                        rd_grid, static_cast<size_t>(g));
                    test_assert_not_NULL(nnc_info);
                    {
                        const std::vector<size_t> &index_list =
                            nnc_info_get_grid_index_list(nnc_info, lgr_nr);
                        test_assert_true(
                            nnc_info_has_grid_index_list(nnc_info, lgr_nr));
                        test_assert_true(contains(index_list, l));
                    }
                }
            }
        }

        /* Amalgamated: LGR -> LGR */
        {
            if (nnc_view->has_kw(NNCHEADA_KW)) {
                rd_kw_type *nncheada_kw = nnc_view->get_kw(NNCHEADA_KW, 0);
                rd_kw_type *nnc1_kw = nnc_view->get_kw(NNA1_KW, 0);
                rd_kw_type *nnc2_kw = nnc_view->get_kw(NNA2_KW, 0);
                size_t lgr_nr1 = static_cast<size_t>(
                    rd_kw_iget_int(nncheada_kw, NNCHEADA_ILOC1_INDEX));
                size_t lgr_nr2 = static_cast<size_t>(
                    rd_kw_iget_int(nncheada_kw, NNCHEADA_ILOC2_INDEX));

                rd_grid_type *lgr1 =
                    rd_grid_get_lgr_from_lgr_nr(rd_grid, lgr_nr1);
                for (size_t i = 0; i < rd_kw_get_size(nnc1_kw); i++) {
                    const int g1 = rd_kw_iget_int(nnc1_kw, i) - 1;
                    const int g2 = rd_kw_iget_int(nnc2_kw, i) - 1;

                    const nnc_info_type *nnc_info = rd_grid_get_cell_nnc_info1(
                        lgr1, static_cast<size_t>(g1));
                    const std::vector<size_t> &index_list =
                        nnc_info_get_grid_index_list(nnc_info, lgr_nr2);
                    test_assert_not_NULL(nnc_info);
                    test_assert_true(contains(index_list, g2));
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    for (int iarg = 1; iarg < argc; iarg++) {
        printf("Checking file: %s \n", argv[iarg]);
        test_scan(argv[iarg]);
    }

    exit(0);
}
