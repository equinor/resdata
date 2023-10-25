#ifndef ERT_RD_NNC_EXPORT
#define ERT_RD_NNC_EXPORT

#include <math.h>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#define ERT_RD_DEFAULT_NNC_TRANS HUGE_VAL

typedef struct {
    int grid_nr1;
    int global_index1;
    int grid_nr2;
    int global_index2;

    int input_index; /* corresponds to the input ordering of this nnc */

    double trans;
} rd_nnc_type;

bool rd_nnc_intersect_format(const rd_grid_type *grid,
                             const rd_file_type *init_file);
int rd_nnc_export_get_size(const rd_grid_type *grid,
                           const rd_file_type *init_file);
int rd_nnc_export(const rd_grid_type *grid, const rd_file_type *init_file,
                  rd_nnc_type *nnc_data);

rd_kw_type *rd_nnc_export_get_tranx_kw(const rd_grid_type *grid,
                                       const rd_file_type *init_file,
                                       int lgr_nr1, int lgr_nr2);
rd_kw_type *rd_nnc_export_get_tranll_kw(const rd_grid_type *grid,
                                        const rd_file_type *init_file,
                                        int lgr_nr1, int lgr_nr2);
rd_kw_type *rd_nnc_export_get_tran_kw(const rd_file_type *init_file,
                                      const char *kw, int lgr_nr);

bool rd_nnc_equal(const rd_nnc_type *nnc1, const rd_nnc_type *nnc2);
int rd_nnc_sort_cmp(const rd_nnc_type *nnc1, const rd_nnc_type *nnc2);
void rd_nnc_sort(rd_nnc_type *nnc_list, int size);

#ifdef __cplusplus
}
#endif
#endif
