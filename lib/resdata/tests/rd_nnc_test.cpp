#include <cstdio>
#include <cstdlib>

#include <memory>
#include <vector>
#include <algorithm>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/nnc_info.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>

template <class T> int vector_util_index(const std::vector<T> &vec, T value) {

    int index;
    auto iter = find(vec.begin(), vec.end(), value);
    if (iter == vec.end())
        index = -1;
    else
        index = iter - vec.begin();
    return index;
}

void test_scan(const char *grid_filename) {
    rd_grid_type *rd_grid = rd_grid_alloc(grid_filename);
    rd_file_ptr grid_file = open_rd_file(std::string(grid_filename));

    for (int block_nr = 0;
         block_nr < rd_file_get_num_named_kw(grid_file.get(), NNCHEAD_KW);
         block_nr++) {
        rd_grid_type *lgr = rd_grid;
        int lgr_nr;
        auto nnc_view = rd_file_get_global_view(grid_file.get())
                            ->blockview(NNCHEAD_KW, NNCHEAD_KW, block_nr);
        {
            if (block_nr > 0)
                lgr = rd_grid_iget_lgr(rd_grid, block_nr - 1);
            lgr_nr = rd_grid_get_lgr_nr(lgr);

            /* Internal nnc */
            {
                if (nnc_view->has_kw(NNC1_KW)) {
                    rd_kw_type *nnc1_kw = nnc_view->get_kw(NNC1_KW, 0);
                    rd_kw_type *nnc2_kw = nnc_view->get_kw(NNC2_KW, 0);
                    int i;
                    for (i = 0; i < rd_kw_get_size(nnc1_kw); i++) {
                        const int g1 = rd_kw_iget_int(nnc1_kw, i) - 1;
                        const int g2 = rd_kw_iget_int(nnc2_kw, i) - 1;

                        if (g2 <
                            rd_grid_get_global_size(
                                lgr)) { // Skipping matrix <-> fracture link in dual poro.
                            const nnc_info_type *nnc_info =
                                rd_grid_get_cell_nnc_info1(lgr, g1);
                            const std::vector<int> &index_list =
                                nnc_info_get_grid_index_list(nnc_info, lgr_nr);
                            test_assert_not_NULL(nnc_info);
                            test_assert_int_not_equal(
                                -1, vector_util_index<int>(index_list, g2));
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
                int i;
                int lgr_nr = rd_kw_iget_int(nnchead_kw, NNCHEAD_LGR_INDEX);
                for (i = 0; i < rd_kw_get_size(nncg_kw); i++) {
                    const int g = rd_kw_iget_int(nncg_kw, i) - 1;
                    const int l = rd_kw_iget_int(nncl_kw, i) - 1;

                    const nnc_info_type *nnc_info =
                        rd_grid_get_cell_nnc_info1(rd_grid, g);
                    test_assert_not_NULL(nnc_info);
                    {
                        const std::vector<int> &index_list =
                            nnc_info_get_grid_index_list(nnc_info, lgr_nr);
                        test_assert_true(
                            nnc_info_has_grid_index_list(nnc_info, lgr_nr));
                        test_assert_int_not_equal(
                            -1, vector_util_index<int>(index_list, l));
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
                int lgr_nr1 = rd_kw_iget_int(nncheada_kw, NNCHEADA_ILOC1_INDEX);
                int lgr_nr2 = rd_kw_iget_int(nncheada_kw, NNCHEADA_ILOC2_INDEX);

                rd_grid_type *lgr1 =
                    rd_grid_get_lgr_from_lgr_nr(rd_grid, lgr_nr1);
                for (int i = 0; i < rd_kw_get_size(nnc1_kw); i++) {
                    const int g1 = rd_kw_iget_int(nnc1_kw, i) - 1;
                    const int g2 = rd_kw_iget_int(nnc2_kw, i) - 1;

                    const nnc_info_type *nnc_info =
                        rd_grid_get_cell_nnc_info1(lgr1, g1);
                    const std::vector<int> &index_list =
                        nnc_info_get_grid_index_list(nnc_info, lgr_nr2);
                    test_assert_not_NULL(nnc_info);
                    test_assert_int_not_equal(
                        -1, vector_util_index<int>(index_list, g2));
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
