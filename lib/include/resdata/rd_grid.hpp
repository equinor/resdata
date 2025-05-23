#ifndef ERT_RD_GRID_H
#define ERT_RD_GRID_H

#include <stdbool.h>

#include <ert/util/double_vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/rd_coarse_cell.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/nnc_info.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#define RD_GRID_COORD_SIZE(nx, ny) (((nx) + 1) * ((ny) + 1) * 6)
#define RD_GRID_ZCORN_SIZE(nx, ny, nz) (((nx) * (ny) * (nz) * 8))

#define RD_GRID_GLOBAL_GRID "Global" // used as key in hash tables over grids.
#define RD_GRID_MAINGRID_LGR_NR 0

typedef double(block_function_ftype)(const double_vector_type *);
typedef struct rd_grid_struct rd_grid_type;

bool rd_grid_have_coarse_cells(const rd_grid_type *main_grid);
bool rd_grid_cell_in_coarse_group1(const rd_grid_type *main_grid,
                                   int global_index);
int rd_grid_get_num_coarse_groups(const rd_grid_type *main_grid);
rd_coarse_cell_type *rd_grid_iget_coarse_group(const rd_grid_type *rd_grid,
                                               int coarse_nr);
void rd_grid_get_column_property(const rd_grid_type *rd_grid,
                                 const rd_kw_type *rd_kw, int i, int j,
                                 double_vector_type *column);
void rd_grid_get_cell_corner_xyz1(const rd_grid_type *grid, int global_index,
                                  int corner_nr, double *xpos, double *ypos,
                                  double *zpos);
void rd_grid_get_corner_xyz(const rd_grid_type *grid, int i, int j, int k,
                            double *xpos, double *ypos, double *zpos);

double rd_grid_get_cell_dx1A(const rd_grid_type *grid, int active_index);
double rd_grid_get_cell_dy1A(const rd_grid_type *grid, int active_index);
double rd_grid_get_cell_dz1A(const rd_grid_type *grid, int active_index);

double rd_grid_get_cell_dx1(const rd_grid_type *grid, int global_index);
double rd_grid_get_cell_dy1(const rd_grid_type *grid, int global_index);
double rd_grid_get_cell_dz1(const rd_grid_type *grid, int global_index);
double rd_grid_get_cell_thickness1(const rd_grid_type *grid, int global_index);

void rd_grid_get_distance(const rd_grid_type *grid, int global_index1,
                          int global_index2, double *dx, double *dy,
                          double *dz);
double rd_grid_get_cdepth1A(const rd_grid_type *grid, int active_index);
double rd_grid_get_cdepth1(const rd_grid_type *grid, int global_index);
int rd_grid_get_global_index_from_xy(const rd_grid_type *rd_grid, int k,
                                     bool lower_layer, double x, double y);
bool rd_grid_cell_contains_xyz1(const rd_grid_type *rd_grid, int global_index,
                                double x, double y, double z);
double rd_grid_get_cell_volume1(const rd_grid_type *rd_grid, int global_index);
double rd_grid_get_cell_volume1A(const rd_grid_type *rd_grid, int active_index);
bool rd_grid_cell_contains1(const rd_grid_type *grid, int global_index,
                            double x, double y, double z);
int rd_grid_get_global_index_from_xyz(rd_grid_type *grid, double x, double y,
                                      double z, int start_index);
bool rd_grid_get_ij_from_xy(const rd_grid_type *grid, double x, double y, int k,
                            int *i, int *j);
const char *rd_grid_get_name(const rd_grid_type *);
int rd_grid_get_active_index3(const rd_grid_type *rd_grid, int i, int j, int k);
int rd_grid_get_active_index1(const rd_grid_type *rd_grid, int global_index);
int rd_grid_get_active_fracture_index1(const rd_grid_type *rd_grid,
                                       int global_index);
bool rd_grid_cell_active3(const rd_grid_type *, int, int, int);
bool rd_grid_cell_active1(const rd_grid_type *, int);
bool rd_grid_ijk_valid(const rd_grid_type *, int, int, int);
int rd_grid_get_global_index3(const rd_grid_type *, int, int, int);
int rd_grid_get_global_index1A(const rd_grid_type *rd_grid, int active_index);
int rd_grid_get_global_index1F(const rd_grid_type *rd_grid,
                               int active_fracture_index);

const nnc_info_type *rd_grid_get_cell_nnc_info1(const rd_grid_type *grid,
                                                int global_index);
void rd_grid_add_self_nnc(rd_grid_type *grid1, int g1, int g2, int nnc_index);
rd_grid_type *rd_grid_alloc_GRDECL_kw(int nx, int ny, int nz,
                                      const rd_kw_type *zcorn_kw,
                                      const rd_kw_type *coord_kw,
                                      const rd_kw_type *actnum_kw,
                                      const rd_kw_type *mapaxes_kw);
rd_grid_type *rd_grid_alloc(const char *);
rd_grid_type *rd_grid_load_case(const char *case_input);
rd_grid_type *rd_grid_load_case__(const char *case_input, bool apply_mapaxes);
rd_grid_type *rd_grid_alloc_rectangular(int nx, int ny, int nz, double dx,
                                        double dy, double dz,
                                        const int *actnum);
rd_kw_type *rd_grid_alloc_volume_kw(const rd_grid_type *grid, bool active_size);
rd_kw_type *rd_grid_alloc_mapaxes_kw(const rd_grid_type *grid);
rd_kw_type *rd_grid_alloc_coord_kw(const rd_grid_type *grid);

bool rd_grid_exists(const char *case_input);

void rd_grid_free(rd_grid_type *);
void rd_grid_free__(void *arg);
void rd_grid_get_dims(const rd_grid_type *, int *, int *, int *, int *);
int rd_grid_get_nz(const rd_grid_type *grid);
int rd_grid_get_nx(const rd_grid_type *grid);
int rd_grid_get_ny(const rd_grid_type *grid);
int rd_grid_get_nactive(const rd_grid_type *grid);
int rd_grid_get_nactive_fracture(const rd_grid_type *grid);
int rd_grid_get_active_index(const rd_grid_type *, int, int, int);
void rd_grid_summarize(const rd_grid_type *);
void rd_grid_get_ijk1(const rd_grid_type *, int global_index, int *, int *,
                      int *);
void rd_grid_get_ijk1A(const rd_grid_type *, int active_index, int *, int *,
                       int *);
void rd_grid_get_xyz3(const rd_grid_type *, int, int, int, double *, double *,
                      double *);
void rd_grid_get_xyz1(const rd_grid_type *grid, int global_index, double *xpos,
                      double *ypos, double *zpos);
void rd_grid_get_xyz1A(const rd_grid_type *grid, int active_index, double *xpos,
                       double *ypos, double *zpos);

int rd_grid_get_global_size(const rd_grid_type *rd_grid);
bool rd_grid_compare(const rd_grid_type *g1, const rd_grid_type *g2,
                     bool include_lgr, bool include_nnc, bool verbose);
int rd_grid_get_active_size(const rd_grid_type *rd_grid);

double rd_grid_get_bottom1(const rd_grid_type *grid, int global_index);
double rd_grid_get_bottom3(const rd_grid_type *grid, int i, int j, int k);
double rd_grid_get_top1(const rd_grid_type *grid, int global_index);
double rd_grid_get_top3(const rd_grid_type *grid, int i, int j, int k);
double rd_grid_get_top1A(const rd_grid_type *grid, int active_index);
double rd_grid_get_top2(const rd_grid_type *grid, int i, int j);
double rd_grid_get_bottom2(const rd_grid_type *grid, int i, int j);
int rd_grid_locate_depth(const rd_grid_type *grid, double depth, int i, int j);

bool rd_grid_cell_invalid1(const rd_grid_type *rd_grid, int global_index);

bool rd_grid_cell_valid1(const rd_grid_type *rd_grid, int global_index);

void rd_grid_dump(const rd_grid_type *grid, FILE *stream);
void rd_grid_dump_ascii(rd_grid_type *grid, bool active_only, FILE *stream);
/* lgr related functions */
const rd_grid_type *rd_grid_get_cell_lgr1(const rd_grid_type *grid,
                                          int global_index);
int rd_grid_get_num_lgr(const rd_grid_type *main_grid);
int rd_grid_get_lgr_nr(const rd_grid_type *rd_grid);
int rd_grid_get_lgr_nr_from_name(const rd_grid_type *grid, const char *name);
rd_grid_type *rd_grid_iget_lgr(const rd_grid_type *main_grid, int lgr_index);
rd_grid_type *rd_grid_get_lgr_from_lgr_nr(const rd_grid_type *main_grid,
                                          int lgr_nr);
rd_grid_type *rd_grid_get_lgr(const rd_grid_type *main_grid,
                              const char *__lgr_name);
bool rd_grid_has_lgr(const rd_grid_type *main_grid, const char *__lgr_name);
bool rd_grid_has_lgr_nr(const rd_grid_type *main_grid, int lgr_nr);
const char *rd_grid_iget_lgr_name(const rd_grid_type *rd_grid, int lgr_index);
const char *rd_grid_get_lgr_name(const rd_grid_type *rd_grid, int lgr_nr);
const rd_grid_type *rd_grid_get_global_grid(const rd_grid_type *grid);

double rd_grid_get_property(const rd_grid_type *rd_grid,
                            const rd_kw_type *rd_kw, int i, int j, int k);
void rd_grid_grdecl_fprintf_kw(const rd_grid_type *rd_grid,
                               const rd_kw_type *rd_kw,
                               const char *special_header, FILE *stream,
                               double double_default);
bool rd_grid_test_lgr_consistency(const rd_grid_type *rd_grid);

void rd_grid_fwrite_EGRID(rd_grid_type *grid, const char *filename,
                          bool metric_output);
void rd_grid_fwrite_EGRID2(rd_grid_type *grid, const char *filename,
                           ert_rd_unit_enum output_unit);

void rd_grid_fwrite_GRID(const rd_grid_type *grid, const char *filename);
void rd_grid_fwrite_GRID2(const rd_grid_type *grid, const char *filename,
                          ert_rd_unit_enum output_unit);

void rd_grid_fprintf_grdecl(rd_grid_type *grid, FILE *stream);
void rd_grid_fprintf_grdecl2(rd_grid_type *grid, FILE *stream,
                             ert_rd_unit_enum output_unit);

int rd_grid_zcorn_index__(int nx, int ny, int i, int j, int k, int c);
int rd_grid_zcorn_index(const rd_grid_type *grid, int i, int j, int k, int c);

rd_kw_type *rd_grid_alloc_zcorn_kw(const rd_grid_type *grid);
rd_kw_type *rd_grid_alloc_actnum_kw(const rd_grid_type *grid);
rd_grid_type *rd_grid_alloc_copy(const rd_grid_type *src_grid);
bool rd_grid_dual_grid(const rd_grid_type *rd_grid);
int rd_grid_get_num_nnc(const rd_grid_type *grid);

bool rd_grid_cell_regular1(const rd_grid_type *rd_grid, int global_index);
void rd_grid_init_zcorn_data(const rd_grid_type *grid, float *zcorn);
void rd_grid_init_zcorn_data_double(const rd_grid_type *grid, double *zcorn);
int rd_grid_get_zcorn_size(const rd_grid_type *grid);

void rd_grid_init_coord_data(const rd_grid_type *grid, float *coord);
void rd_grid_init_coord_data_double(const rd_grid_type *grid, double *coord);
int rd_grid_get_coord_size(const rd_grid_type *rd_grid);

void rd_grid_init_actnum_data(const rd_grid_type *grid, int *actnum);
bool rd_grid_use_mapaxes(const rd_grid_type *grid);
void rd_grid_init_mapaxes_data_double(const rd_grid_type *grid,
                                      double *mapaxes);
void rd_grid_reset_actnum(rd_grid_type *grid, const int *actnum);
void rd_grid_compressed_kw_copy(const rd_grid_type *grid, rd_kw_type *target_kw,
                                const rd_kw_type *src_kw);
void rd_grid_global_kw_copy(const rd_grid_type *grid, rd_kw_type *target_kw,
                            const rd_kw_type *src_kw);
void rd_grid_export_cell_corners1(const rd_grid_type *grid, int global_index,
                                  double *x, double *y, double *z);

ert_rd_unit_enum rd_grid_get_unit_system(const rd_grid_type *grid);
void rd_grid_export_index(const rd_grid_type *grid, int *global_index,
                          int *index_data, bool active_only);
void rd_grid_export_data_as_int(int index_size, const int *global_index,
                                const rd_kw_type *kw, int *output);
void rd_grid_export_data_as_double(int index_size, const int *data_index,
                                   const rd_kw_type *kw, double *output);
void rd_grid_export_volume(const rd_grid_type *grid, int index_size,
                           const int *global_index, double *output);
void rd_grid_export_position(const rd_grid_type *grid, int index_size,
                             const int *global_index, double *output);
void export_corners(const rd_grid_type *grid, int index_size,
                    const int *global_index, double *output);
float rd_grid_output_scaling(const rd_grid_type *grid,
                             ert_rd_unit_enum output_unit);

UTIL_IS_INSTANCE_HEADER(rd_grid);
UTIL_SAFE_CAST_HEADER(rd_grid);

#ifdef __cplusplus
}
#endif
#endif
