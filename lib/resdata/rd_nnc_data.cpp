#define RD_NNC_DATA_TYPE_ID 83756236
#include <cstring>

#include <resdata/rd_nnc_data.hpp>
#include <resdata/rd_nnc_geometry.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw_magic.hpp>

enum kw_data_type {
    TRANS_DATA = 1,
    WTR_FLUX_DATA = 2,
    OIL_FLUX_DATA = 3,
    GAS_FLUX_DATA = 4
};

struct rd_nnc_data_struct {
    UTIL_TYPE_ID_DECLARATION;
    int size;
    double *values;
};

static const char *rd_nnc_data_get_str_kw(int kw_type, int grid1, int grid2) {
    const char *kw = NULL;
    switch (kw_type) {

    case TRANS_DATA:
        if (grid1 == grid2)
            kw = TRANNNC_KW;
        else if (grid1 == 0)
            kw = TRANGL_KW;
        else
            kw = TRANLL_KW;
        break;

    case WTR_FLUX_DATA:
        if (grid1 == grid2)
            kw = FLRWATNNC_KW;
        else if (grid1 == 0)
            kw = FLRWATLG_KW;
        else
            kw = FLRWATLL_KW;
        break;

    case OIL_FLUX_DATA:
        if (grid1 == grid2)
            kw = FLROILNNC_KW;
        else if (grid1 == 0)
            kw = FLROILLG_KW;
        else
            kw = FLROILLL_KW;
        break;

    case GAS_FLUX_DATA:
        if (grid1 == grid2)
            kw = FLRGASNNC_KW;
        else if (grid1 == 0)
            kw = FLRGASLG_KW;
        else
            kw = FLRGASLL_KW;
        break;

    default:
        kw = NULL;
    }
    return kw;
}

static rd_kw_type *
rd_nnc_data_get_gl_kw(const rd_file_view_type *init_file_view, const char *kw,
                      int kw_type, int lgr_nr) {
    if (!kw)
        return NULL;

    if (lgr_nr == 0) {
        if (rd_file_view_has_kw(init_file_view, kw))
            return rd_file_view_iget_named_kw(init_file_view, kw, 0);
        else
            return NULL;
    }

    bool correct_lgrheadi = false;
    const int file_num_kw = rd_file_view_get_size(init_file_view);
    for (int kw_index = 0, head_index = 0; kw_index < file_num_kw; ++kw_index) {
        rd_kw_type *rd_kw = rd_file_view_iget_kw(init_file_view, kw_index);
        const char *current_kw = rd_kw_get_header(rd_kw);

        if (strcmp(LGRHEADI_KW, current_kw) == 0) {
            if (rd_kw_iget_int(rd_kw, LGRHEADI_LGR_NR_INDEX) == lgr_nr) {
                correct_lgrheadi = true;
                head_index = kw_index;
            } else {
                correct_lgrheadi = false;
            }
        }

        if (correct_lgrheadi && strcmp(kw, current_kw) == 0) {
            /* This is to calculate who fare from lgrheadi we found the TRANGL/TRANNNC key word */
            int steps = kw_index - head_index;
            /* We only support a file format where TRANNNC is 3 steps and TRANGL is 4 or 6 steps from LGRHEADI */
            if (kw_type != TRANS_DATA || steps == 3 || steps == 4 ||
                steps == 6) {
                return rd_kw;
            }
        }
    }

    return NULL;
}

static rd_kw_type *
rd_nnc_data_get_tranll_kw(const rd_grid_type *grid,
                          const rd_file_view_type *init_file_view, int lgr_nr1,
                          int lgr_nr2) {
    const char *lgr_name1 = rd_grid_get_lgr_name(grid, lgr_nr1);
    const char *lgr_name2 = rd_grid_get_lgr_name(grid, lgr_nr2);

    rd_kw_type *tran_kw = NULL;
    const int file_num_kw = rd_file_view_get_size(init_file_view);
    int global_kw_index = 0;

    while (true) {
        if (global_kw_index >= file_num_kw)
            break;
        {
            rd_kw_type *rd_kw =
                rd_file_view_iget_kw(init_file_view, global_kw_index);
            if (strcmp(LGRJOIN_KW, rd_kw_get_header(rd_kw)) == 0) {

                if (rd_kw_icmp_string(rd_kw, 0, lgr_name1) &&
                    rd_kw_icmp_string(rd_kw, 1, lgr_name2)) {
                    tran_kw = rd_file_view_iget_kw(init_file_view,
                                                   global_kw_index + 1);
                    break;
                }
            }
            global_kw_index++;
        }
    }

    return tran_kw;
}

static rd_kw_type *rd_nnc_data_get_kw(const rd_grid_type *grid,
                                      const rd_file_view_type *init_file_view,
                                      int lgr_nr1, int lgr_nr2, int kw_type) {

    const char *kw = rd_nnc_data_get_str_kw(kw_type, lgr_nr1, lgr_nr2);
    if (lgr_nr1 == 0 || lgr_nr1 == lgr_nr2)
        return rd_nnc_data_get_gl_kw(init_file_view, kw, kw_type, lgr_nr2);
    else if (kw_type == TRANS_DATA)
        return rd_nnc_data_get_tranll_kw(grid, init_file_view, lgr_nr1,
                                         lgr_nr2);
    else
        return NULL;
}

static void assert_correct_kw_count(rd_kw_type *kw, const char *function_name,
                                    int correct_kw_count, int kw_count) {
    if (correct_kw_count != kw_count)
        util_abort("In function %s, reading kw: %s. %d != %d", function_name,
                   rd_kw_get_header(kw), correct_kw_count, kw_count);
}

static bool rd_nnc_data_set_values(rd_nnc_data_type *data,
                                   const rd_grid_type *grid,
                                   const rd_nnc_geometry_type *nnc_geo,
                                   const rd_file_view_type *init_file,
                                   int kw_type) {

    int current_grid1 = -1;
    int current_grid2 = -1;
    rd_kw_type *current_kw = NULL;
    int correct_kw_count = 0;
    int kw_count = 0;
    int nnc_size = rd_nnc_geometry_size(nnc_geo);

    for (int nnc_index = 0; nnc_index < nnc_size; nnc_index++) {
        const rd_nnc_pair_type *pair = rd_nnc_geometry_iget(nnc_geo, nnc_index);
        int grid1 = pair->grid_nr1;
        int grid2 = pair->grid_nr2;

        if (grid1 != current_grid1 || grid2 != current_grid2) {
            current_grid1 = grid1;
            current_grid2 = grid2;
            assert_correct_kw_count(current_kw, __func__, correct_kw_count,
                                    kw_count);
            current_kw =
                rd_nnc_data_get_kw(grid, init_file, grid1, grid2, kw_type);
            kw_count = 0;
            if (current_kw) {
                correct_kw_count = rd_kw_get_size(current_kw);
            } else {
                return false;
            }
        }
        if (current_kw) {
            data->values[nnc_index] =
                rd_kw_iget_as_double(current_kw, pair->input_index);
            kw_count++;
        }
    }
    assert_correct_kw_count(current_kw, __func__, correct_kw_count, kw_count);
    return true;
}

static rd_nnc_data_type *
rd_nnc_data_alloc__(const rd_grid_type *grid,
                    const rd_nnc_geometry_type *nnc_geo,
                    const rd_file_view_type *init_file, int kw_type) {
    rd_nnc_data_type *data = (rd_nnc_data_type *)util_malloc(sizeof *data);

    int nnc_size = rd_nnc_geometry_size(nnc_geo);
    data->size = nnc_size;

    data->values = (double *)util_malloc(nnc_size * sizeof(double));

    if (rd_nnc_data_set_values(data, grid, nnc_geo, init_file, kw_type))
        return data;
    else {
        rd_nnc_data_free(data);
        return NULL;
    }
}

rd_nnc_data_type *rd_nnc_data_alloc_tran(const rd_grid_type *grid,
                                         const rd_nnc_geometry_type *nnc_geo,
                                         const rd_file_view_type *init_file) {
    return rd_nnc_data_alloc__(grid, nnc_geo, init_file, TRANS_DATA);
}

rd_nnc_data_type *
rd_nnc_data_alloc_wat_flux(const rd_grid_type *grid,
                           const rd_nnc_geometry_type *nnc_geo,
                           const rd_file_view_type *init_file) {
    return rd_nnc_data_alloc__(grid, nnc_geo, init_file, WTR_FLUX_DATA);
}

rd_nnc_data_type *
rd_nnc_data_alloc_oil_flux(const rd_grid_type *grid,
                           const rd_nnc_geometry_type *nnc_geo,
                           const rd_file_view_type *init_file) {
    return rd_nnc_data_alloc__(grid, nnc_geo, init_file, OIL_FLUX_DATA);
}

rd_nnc_data_type *
rd_nnc_data_alloc_gas_flux(const rd_grid_type *grid,
                           const rd_nnc_geometry_type *nnc_geo,
                           const rd_file_view_type *init_file) {
    return rd_nnc_data_alloc__(grid, nnc_geo, init_file, GAS_FLUX_DATA);
}

void rd_nnc_data_free(rd_nnc_data_type *data) {
    free(data->values);
    free(data);
}

int rd_nnc_data_get_size(rd_nnc_data_type *data) { return data->size; }

const double *rd_nnc_data_get_values(const rd_nnc_data_type *data) {
    return data->values;
}

double rd_nnc_data_iget_value(const rd_nnc_data_type *data, int index) {
    if (index >= data->size)
        util_abort("%s: index value:%d out range: [0,%d) \n", __func__, index,
                   data->size);

    return data->values[index];
}
