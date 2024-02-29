#include <stdlib.h>

#include <vector>
#include <cstring>

#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_nnc_export.hpp>
#include <resdata/nnc_info.hpp>
#include <resdata/rd_kw_magic.hpp>

/**
 * Return true if the NNC information is stored in the Intersect format, false otherwise.
 * In the Intersect format, the NNC information stored in the grid is unrealiable.
 * The correct NNC data is stored in the init file instead
 */
bool rd_nnc_intersect_format(const rd_grid_type *grid,
                             const rd_file_type *init_file) {
    if (!rd_file_has_kw(init_file, NNC1_KW) ||
        !rd_file_has_kw(init_file, NNC2_KW) ||
        !rd_file_has_kw(init_file, TRANNNC_KW))
        return false;
    // In the specific case we are treating, there should be just 1 occurrence of the kw
    const auto nnc1_num =
        rd_kw_get_size(rd_file_iget_named_kw(init_file, NNC1_KW, 0));
    const auto nnc2_num =
        rd_kw_get_size(rd_file_iget_named_kw(init_file, NNC2_KW, 0));
    const auto tran_num =
        rd_kw_get_size(rd_file_iget_named_kw(init_file, TRANNNC_KW, 0));
    return nnc1_num == tran_num && nnc2_num == tran_num;
}

int rd_nnc_export_get_size(const rd_grid_type *grid,
                           const rd_file_type *init_file) {
    return rd_nnc_intersect_format(grid, init_file)
               ? rd_kw_get_size(rd_file_iget_named_kw(init_file, TRANNNC_KW, 0))
               :                          // Intersect format
               rd_grid_get_num_nnc(grid); // Eclipse format
}

static int rd_nnc_export_intersect__(const rd_file_type *init_file,
                                     rd_nnc_type *nnc_data, int *nnc_offset) {
    const auto nnc1_kw = rd_file_iget_named_kw(init_file, NNC1_KW, 0);
    const auto nnc2_kw = rd_file_iget_named_kw(init_file, NNC2_KW, 0);
    const auto tran_kw = rd_file_iget_named_kw(init_file, TRANNNC_KW, 0);

    auto nnc_index = *nnc_offset;
    for (int i = 0; i < rd_kw_get_size(tran_kw); ++i) {
        auto const nnc1 = rd_kw_iget_int(nnc1_kw, i);
        auto const nnc2 = rd_kw_iget_int(nnc2_kw, i);
        auto const tran = rd_kw_iget_as_double(tran_kw, i);
        nnc_data[nnc_index] = rd_nnc_type{0, nnc1, 0, nnc2, i, tran};
        ++nnc_index;
    }
    *nnc_offset = nnc_index;
    return rd_kw_get_size(tran_kw); // Assume all valid
}

static int rd_nnc_export__(const rd_grid_type *grid, int lgr_index1,
                           const rd_file_type *init_file, rd_nnc_type *nnc_data,
                           int *nnc_offset) {
    int nnc_index = *nnc_offset;
    int lgr_nr1 = rd_grid_get_lgr_nr(grid);
    int global_index1;
    int valid_trans = 0;
    const rd_grid_type *global_grid = rd_grid_get_global_grid(grid);

    if (!global_grid)
        global_grid = grid;

    for (global_index1 = 0; global_index1 < rd_grid_get_global_size(grid);
         global_index1++) {
        const nnc_info_type *nnc_info =
            rd_grid_get_cell_nnc_info1(grid, global_index1);
        if (nnc_info) {
            int lgr_index2;
            for (lgr_index2 = 0; lgr_index2 < nnc_info_get_size(nnc_info);
                 lgr_index2++) {
                const nnc_vector_type *nnc_vector =
                    nnc_info_iget_vector(nnc_info, lgr_index2);
                const std::vector<int> &grid2_index_list =
                    nnc_vector_get_grid_index_list(nnc_vector);
                const std::vector<int> &nnc_index_list =
                    nnc_vector_get_nnc_index_list(nnc_vector);
                int lgr_nr2 = nnc_vector_get_lgr_nr(nnc_vector);
                const rd_kw_type *tran_kw = rd_nnc_export_get_tranx_kw(
                    global_grid, init_file, lgr_nr1, lgr_nr2);

                int index2;
                rd_nnc_type nnc;

                nnc.grid_nr1 = lgr_nr1;
                nnc.grid_nr2 = lgr_nr2;
                nnc.global_index1 = global_index1;

                for (index2 = 0; index2 < nnc_vector_get_size(nnc_vector);
                     index2++) {
                    nnc.global_index2 = grid2_index_list[index2];
                    nnc.input_index = nnc_index_list[index2];
                    if (tran_kw) {
                        nnc.trans =
                            rd_kw_iget_as_double(tran_kw, nnc.input_index);
                        valid_trans++;
                    } else {
                        nnc.trans = ERT_RD_DEFAULT_NNC_TRANS;
                    }

                    nnc_data[nnc_index] = nnc;
                    nnc_index++;
                }
            }
        }
    }
    *nnc_offset = nnc_index;
    return valid_trans;
}

int rd_nnc_export(const rd_grid_type *grid, const rd_file_type *init_file,
                  rd_nnc_type *nnc_data) {
    int nnc_index = 0;
    int total_valid_trans = 0;
    if (rd_nnc_intersect_format(grid, init_file)) {
        // Intersect format
        total_valid_trans =
            rd_nnc_export_intersect__(init_file, nnc_data, &nnc_index);
    } else {
        // Eclipse format
        total_valid_trans =
            rd_nnc_export__(grid, 0, init_file, nnc_data, &nnc_index);
        {
            for (int lgr_index = 0; lgr_index < rd_grid_get_num_lgr(grid);
                 lgr_index++) {
                rd_grid_type *igrid = rd_grid_iget_lgr(grid, lgr_index);
                total_valid_trans += rd_nnc_export__(
                    igrid, lgr_index, init_file, nnc_data, &nnc_index);
            }
        }
        nnc_index = rd_grid_get_num_nnc(grid);
    }
    rd_nnc_sort(nnc_data, nnc_index);
    return total_valid_trans;
}

int rd_nnc_sort_cmp(const rd_nnc_type *nnc1, const rd_nnc_type *nnc2) {

    if (nnc1->grid_nr1 != nnc2->grid_nr1) {
        if (nnc1->grid_nr1 < nnc2->grid_nr1)
            return -1;
        else
            return 1;
    }

    if (nnc1->grid_nr2 != nnc2->grid_nr2) {
        if (nnc1->grid_nr2 < nnc2->grid_nr2)
            return -1;
        else
            return 1;
    }

    if (nnc1->global_index1 != nnc2->global_index1) {
        if (nnc1->global_index1 < nnc2->global_index1)
            return -1;
        else
            return 1;
    }

    if (nnc1->global_index2 != nnc2->global_index2) {
        if (nnc1->global_index2 < nnc2->global_index2)
            return -1;
        else
            return 1;
    }

    return 0;
}

bool rd_nnc_equal(const rd_nnc_type *nnc1, const rd_nnc_type *nnc2) {

    if (rd_nnc_sort_cmp(nnc1, nnc2) == 0)
        return ((nnc1->trans == nnc2->trans) &&
                (nnc1->input_index == nnc2->input_index));
    else
        return false;
}

static int rd_nnc_sort_cmp__(const void *nnc1, const void *nnc2) {
    return rd_nnc_sort_cmp((const rd_nnc_type *)nnc1,
                           (const rd_nnc_type *)nnc2);
}

void rd_nnc_sort(rd_nnc_type *nnc_list, int size) {
    qsort(nnc_list, size, sizeof *nnc_list, rd_nnc_sort_cmp__);
}

rd_kw_type *rd_nnc_export_get_tranll_kw(const rd_grid_type *grid,
                                        const rd_file_type *init_file,
                                        int lgr_nr1, int lgr_nr2) {
    const char *lgr_name1 = rd_grid_get_lgr_name(grid, lgr_nr1);
    const char *lgr_name2 = rd_grid_get_lgr_name(grid, lgr_nr2);

    rd_kw_type *tran_kw = NULL;
    const int file_num_kw = rd_file_get_size(init_file);
    int global_kw_index = 0;

    while (true) {
        if (global_kw_index >= file_num_kw)
            break;
        {
            rd_kw_type *rd_kw = rd_file_iget_kw(init_file, global_kw_index);
            if (strcmp(LGRJOIN_KW, rd_kw_get_header(rd_kw)) == 0) {

                if (rd_kw_icmp_string(rd_kw, 0, lgr_name1) &&
                    rd_kw_icmp_string(rd_kw, 1, lgr_name2)) {
                    tran_kw = rd_file_iget_kw(init_file, global_kw_index + 1);
                    break;
                }
            }
            global_kw_index++;
        }
    }

    return tran_kw;
}

rd_kw_type *rd_nnc_export_get_tran_kw(const rd_file_type *init_file,
                                      const char *kw, int lgr_nr) {
    rd_kw_type *tran_kw = NULL;
    if (lgr_nr == 0) {
        if (strcmp(kw, TRANNNC_KW) == 0)
            if (rd_file_has_kw(init_file, kw)) {
                tran_kw = rd_file_iget_named_kw(init_file, TRANNNC_KW, 0);
            }
    } else {
        if ((strcmp(kw, TRANNNC_KW) == 0) || (strcmp(kw, TRANGL_KW) == 0)) {
            const int file_num_kw = rd_file_get_size(init_file);
            int global_kw_index = 0;
            bool finished = false;
            bool correct_lgrheadi = false;
            int head_index = 0;
            int steps = 0;

            while (!finished) {
                rd_kw_type *rd_kw = rd_file_iget_kw(init_file, global_kw_index);
                const char *current_kw = rd_kw_get_header(rd_kw);
                if (strcmp(LGRHEADI_KW, current_kw) == 0) {
                    if (rd_kw_iget_int(rd_kw, LGRHEADI_LGR_NR_INDEX) ==
                        lgr_nr) {
                        correct_lgrheadi = true;
                        head_index = global_kw_index;
                    } else {
                        correct_lgrheadi = false;
                    }
                }
                if (correct_lgrheadi) {
                    if (strcmp(kw, current_kw) == 0) {
                        steps =
                            global_kw_index -
                            head_index; /* This is to calculate who fare from lgrheadi we found the TRANGL/TRANNNC key word */
                        if (steps == 3 || steps == 4 ||
                            steps ==
                                6) { /* We only support a file format where TRANNNC is 3 steps and TRANGL is 4 or 6 steps from LGRHEADI */
                            tran_kw = rd_kw;
                            finished = true;
                            break;
                        }
                    }
                }
                global_kw_index++;
                if (global_kw_index == file_num_kw)
                    finished = true;
            }
        }
    }
    return tran_kw;
}

rd_kw_type *rd_nnc_export_get_tranx_kw(const rd_grid_type *grid,
                                       const rd_file_type *init_file,
                                       int lgr_nr1, int lgr_nr2) {
    if (lgr_nr1 == lgr_nr2)
        return rd_nnc_export_get_tran_kw(init_file, TRANNNC_KW, lgr_nr2);
    else {
        if (lgr_nr1 == 0)
            return rd_nnc_export_get_tran_kw(init_file, TRANGL_KW, lgr_nr2);
        else
            return rd_nnc_export_get_tranll_kw(grid, init_file, lgr_nr1,
                                               lgr_nr2);
    }
}
