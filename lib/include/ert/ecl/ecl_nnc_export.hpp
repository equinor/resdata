#ifndef ERT_ECL_NNC_EXPORT
#define ERT_ECL_NNC_EXPORT

#include <math.h>

#include <ert/ecl/ecl_grid.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_kw.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#define ERT_ECL_DEFAULT_NNC_TRANS HUGE_VAL

typedef struct {
    int grid_nr1;
    int global_index1;
    int grid_nr2;
    int global_index2;

    int input_index; /* corresponds to the input ordering of this nnc */

    double trans;
} ecl_nnc_type;

bool ecl_nnc_intersect_format(const ecl_grid_type *grid,
                              const ecl_file_type *init_file);
int ecl_nnc_export_get_size(const ecl_grid_type *grid,
                            const ecl_file_type *init_file);
int ecl_nnc_export(const ecl_grid_type *grid, const ecl_file_type *init_file,
                   ecl_nnc_type *nnc_data);

ecl_kw_type *ecl_nnc_export_get_tranx_kw(const ecl_grid_type *grid,
                                         const ecl_file_type *init_file,
                                         int lgr_nr1, int lgr_nr2);
ecl_kw_type *ecl_nnc_export_get_tranll_kw(const ecl_grid_type *grid,
                                          const ecl_file_type *init_file,
                                          int lgr_nr1, int lgr_nr2);
ecl_kw_type *ecl_nnc_export_get_tran_kw(const ecl_file_type *init_file,
                                        const char *kw, int lgr_nr);

bool ecl_nnc_equal(const ecl_nnc_type *nnc1, const ecl_nnc_type *nnc2);
int ecl_nnc_sort_cmp(const ecl_nnc_type *nnc1, const ecl_nnc_type *nnc2);
void ecl_nnc_sort(ecl_nnc_type *nnc_list, int size);

#ifdef __cplusplus
}
#endif
#endif
