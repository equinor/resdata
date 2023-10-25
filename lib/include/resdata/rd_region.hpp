#ifndef ERT_RD_REGION_H
#define ERT_RD_REGION_H
#include <stdbool.h>

#include <ert/util/int_vector.hpp>

#include <ert/geometry/geo_polygon.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/layer.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SELECT_ALL = 0,
    DESELECT_ALL = 1,
    SELECT_FROM_IJK = 2,
    DESELECT_FROM_IJK = 3,
    SELECT_FROM_I = 4,
    DSELECT_FROM_I = 5,
    SELECT_FROM_J = 6,
    DSELECT_FROM_J = 7,
    SELECT_FROM_K = 8,
    DSELECT_FROM_K = 9,
    SELECT_EQUAL = 10,
    DESELECT_EQUAL = 11,
    SELECT_IN_INTERVAL = 12,
    DESELECT_IN_INTERVAL = 13,
    INVERT_SELECTION = 14
} rd_region_select_cmd;

typedef struct rd_region_struct rd_region_type;

void rd_region_unlock(rd_region_type *region);
void rd_region_lock(rd_region_type *region);
void rd_region_reset(rd_region_type *rd_region);
rd_region_type *rd_region_alloc_copy(const rd_region_type *rd_region);
rd_region_type *rd_region_alloc(const rd_grid_type *rd_grid, bool preselect);
void rd_region_free(rd_region_type *region);
void rd_region_free__(void *__region);

const int_vector_type *rd_region_get_active_list(rd_region_type *region);
const int_vector_type *rd_region_get_global_list(rd_region_type *region);
const int_vector_type *rd_region_get_global_active_list(rd_region_type *region);

bool rd_region_contains_ijk(const rd_region_type *rd_region, int i, int j,
                            int k);
bool rd_region_contains_global(const rd_region_type *rd_region,
                               int global_index);
bool rd_region_contains_active(const rd_region_type *rd_region,
                               int active_index);

void rd_region_select_true(rd_region_type *region, const rd_kw_type *rd_kw);

void rd_region_invert_selection(rd_region_type *region);
void rd_region_select_all(rd_region_type *region);
void rd_region_deselect_all(rd_region_type *region);
void rd_region_deselect_true(rd_region_type *region, const rd_kw_type *rd_kw);
void rd_region_select_false(rd_region_type *region, const rd_kw_type *rd_kw);

void rd_region_select_in_interval(rd_region_type *region,
                                  const rd_kw_type *rd_kw, float min_value,
                                  float max_value);
void rd_region_deselect_in_interval(rd_region_type *region,
                                    const rd_kw_type *rd_kw, float min_value,
                                    float max_value);

void rd_region_select_equal(rd_region_type *region, const rd_kw_type *rd_kw,
                            int value);
void rd_region_deselect_equal(rd_region_type *region, const rd_kw_type *rd_kw,
                              int value);

void rd_region_select_inactive_cells(rd_region_type *region);
void rd_region_deselect_inactive_cells(rd_region_type *region);
void rd_region_select_active_cells(rd_region_type *region);
void rd_region_deselect_active_cells(rd_region_type *region);

void rd_region_select_from_ijkbox(rd_region_type *region, int i1, int i2,
                                  int j1, int j2, int k1, int k2);
void rd_region_deselect_from_ijkbox(rd_region_type *region, int i1, int i2,
                                    int j1, int j2, int k1, int k2);

void rd_region_select_i1i2(rd_region_type *region, int i1, int i2);
void rd_region_deselect_i1i2(rd_region_type *region, int i1, int i2);
void rd_region_select_j1j2(rd_region_type *region, int j1, int j2);
void rd_region_deselect_j1j2(rd_region_type *region, int j1, int i2);
void rd_region_select_k1k2(rd_region_type *region, int k1, int k2);
void rd_region_deselect_k1k2(rd_region_type *region, int k1, int i2);

void rd_region_select_shallow_cells(rd_region_type *region, double depth_limit);
void rd_region_deselect_shallow_cells(rd_region_type *region,
                                      double depth_limit);
void rd_region_select_deep_cells(rd_region_type *region, double depth_limit);
void rd_region_deselect_deep_cells(rd_region_type *region, double depth_limit);

void rd_region_select_thin_cells(rd_region_type *rd_region, double dz_limit);
void rd_region_deselect_thin_cells(rd_region_type *rd_region, double dz_limit);
void rd_region_select_thick_cells(rd_region_type *rd_region, double dz_limit);
void rd_region_deselect_thick_cells(rd_region_type *rd_region, double dz_limit);

void rd_region_select_small_cells(rd_region_type *rd_region,
                                  double volum_limit);
void rd_region_deselect_small_cells(rd_region_type *rd_region,
                                    double volum_limit);
void rd_region_select_large_cells(rd_region_type *rd_region,
                                  double volum_limit);
void rd_region_deselect_large_cells(rd_region_type *rd_region,
                                    double volum_limit);

void rd_region_select_global_index(rd_region_type *rd_region, int global_index);
void rd_region_deselect_global_index(rd_region_type *rd_region,
                                     int global_index);

void rd_region_select_active_index(rd_region_type *rd_region, int active_index);
void rd_region_deselect_active_index(rd_region_type *rd_region,
                                     int active_index);

void rd_region_intersection(rd_region_type *region,
                            const rd_region_type *new_region);
void rd_region_union(rd_region_type *region, const rd_region_type *new_region);
void rd_region_subtract(rd_region_type *region,
                        const rd_region_type *new_region);
void rd_region_xor(rd_region_type *region, const rd_region_type *new_region);

void rd_region_select_smaller(rd_region_type *rd_region,
                              const rd_kw_type *rd_kw, float limit);
void rd_region_deselect_smaller(rd_region_type *rd_region,
                                const rd_kw_type *rd_kw, float limit);
void rd_region_select_larger(rd_region_type *rd_region, const rd_kw_type *rd_kw,
                             float limit);
void rd_region_deselect_larger(rd_region_type *rd_region,
                               const rd_kw_type *rd_kw, float limit);

void rd_region_cmp_select_less(rd_region_type *rd_region, const rd_kw_type *kw1,
                               const rd_kw_type *kw2);
void rd_region_cmp_deselect_less(rd_region_type *rd_region,
                                 const rd_kw_type *kw1, const rd_kw_type *kw2);
void rd_region_cmp_select_more(rd_region_type *rd_region, const rd_kw_type *kw1,
                               const rd_kw_type *kw2);
void rd_region_cmp_deselect_more(rd_region_type *rd_region,
                                 const rd_kw_type *kw1, const rd_kw_type *kw2);

void rd_region_select_in_cylinder(rd_region_type *region, double x0, double y0,
                                  double R);
void rd_region_deselect_in_cylinder(rd_region_type *region, double x0,
                                    double y0, double R);
void rd_region_select_in_zcylinder(rd_region_type *region, double x0, double y0,
                                   double R, double z1, double z2);
void rd_region_deselect_in_zcylinder(rd_region_type *region, double x0,
                                     double y0, double R, double z1, double z2);

void rd_region_select_above_plane(rd_region_type *region, const double n[3],
                                  const double p[3]);
void rd_region_select_below_plane(rd_region_type *region, const double n[3],
                                  const double p[3]);
void rd_region_deselect_above_plane(rd_region_type *region, const double n[3],
                                    const double p[3]);
void rd_region_deselect_below_plane(rd_region_type *region, const double n[3],
                                    const double p[3]);

void rd_region_select_inside_polygon(rd_region_type *region,
                                     const geo_polygon_type *polygon);
void rd_region_deselect_inside_polygon(rd_region_type *region,
                                       const geo_polygon_type *polygon);
void rd_region_select_outside_polygon(rd_region_type *region,
                                      const geo_polygon_type *polygon);
void rd_region_deselect_outside_polygon(rd_region_type *region,
                                        const geo_polygon_type *polygon);

void rd_region_select_from_layer(rd_region_type *region,
                                 const layer_type *layer, int k,
                                 int layer_value);
void rd_region_deselect_from_layer(rd_region_type *region,
                                   const layer_type *layer, int k,
                                   int layer_value);
void rd_region_deselect_false(rd_region_type *region, const rd_kw_type *rd_kw);

void rd_region_set_kw_int(rd_region_type *rd_region, rd_kw_type *rd_kw,
                          int value, bool force_active);
void rd_region_set_kw_float(rd_region_type *rd_region, rd_kw_type *rd_kw,
                            float value, bool force_active);
void rd_region_set_kw_double(rd_region_type *rd_region, rd_kw_type *rd_kw,
                             double value, bool force_active);
void rd_region_kw_copy(rd_region_type *rd_region, rd_kw_type *rd_kw,
                       const rd_kw_type *src_kw, bool force_active);
int rd_region_get_kw_size(rd_region_type *rd_region, const rd_kw_type *rd_kw,
                          bool force_active);

void rd_region_kw_iadd(rd_region_type *rd_region, rd_kw_type *rd_kw,
                       const rd_kw_type *delta_kw, bool force_active);
void rd_region_kw_idiv(rd_region_type *rd_region, rd_kw_type *rd_kw,
                       const rd_kw_type *div_kw, bool force_active);
void rd_region_kw_imul(rd_region_type *rd_region, rd_kw_type *rd_kw,
                       const rd_kw_type *mul_kw, bool force_active);
void rd_region_kw_isub(rd_region_type *rd_region, rd_kw_type *rd_kw,
                       const rd_kw_type *delta_kw, bool force_active);

bool rd_region_equal(const rd_region_type *region1,
                     const rd_region_type *region2);

void rd_region_scale_kw_float(rd_region_type *rd_region, rd_kw_type *rd_kw,
                              float value, bool force_active);
void rd_region_scale_kw_double(rd_region_type *rd_region, rd_kw_type *rd_kw,
                               double value, bool force_active);
void rd_region_scale_kw_int(rd_region_type *rd_region, rd_kw_type *rd_kw,
                            int value, bool force_active);
void rd_region_shift_kw_int(rd_region_type *rd_region, rd_kw_type *rd_kw,
                            int value, bool force_active);
void rd_region_shift_kw_double(rd_region_type *rd_region, rd_kw_type *rd_kw,
                               double value, bool force_active);
void rd_region_shift_kw_float(rd_region_type *rd_region, rd_kw_type *rd_kw,
                              float value, bool force_active);

const int_vector_type *rd_region_get_kw_index_list(rd_region_type *rd_region,
                                                   const rd_kw_type *rd_kw,
                                                   bool force_active);

void rd_region_set_name(rd_region_type *region, const char *name);
const char *rd_region_get_name(const rd_region_type *region);

int rd_region_get_active_size_cpp(rd_region_type *region);
int rd_region_get_global_size_cpp(rd_region_type *region);
const int *rd_region_get_active_list_cpp(rd_region_type *region);
const int *rd_region_get_global_list_cpp(rd_region_type *region);

double rd_region_sum_kw_double(rd_region_type *rd_region,
                               const rd_kw_type *rd_kw, bool force_active);
int rd_region_sum_kw_int(rd_region_type *rd_region, const rd_kw_type *rd_kw,
                         bool force_active);
float rd_region_sum_kw_float(rd_region_type *rd_region, const rd_kw_type *rd_kw,
                             bool force_active);
int rd_region_sum_kw_bool(rd_region_type *rd_region, const rd_kw_type *rd_kw,
                          bool force_active);

UTIL_IS_INSTANCE_HEADER(rd_region);
UTIL_SAFE_CAST_HEADER(rd_region);

#ifdef __cplusplus
}
#endif
#endif
