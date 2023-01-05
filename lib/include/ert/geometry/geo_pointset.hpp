#ifndef ERT_GEO_POINTSET_H
#define ERT_GEO_POINTSET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geo_pointset_struct geo_pointset_type;

geo_pointset_type *geo_pointset_alloc(bool external_z);
void geo_pointset_free(geo_pointset_type *pointset);
void geo_pointset_add_xyz(geo_pointset_type *pointset, double x, double y,
                          double z);
int geo_pointset_get_size(const geo_pointset_type *pointset);
void geo_pointset_iget_xy(const geo_pointset_type *pointset, int index,
                          double *x, double *y);
const double *geo_pointset_get_zcoord(const geo_pointset_type *pointset);
bool geo_pointset_equal(const geo_pointset_type *pointset1,
                        const geo_pointset_type *pointset2);
double geo_pointset_iget_z(const geo_pointset_type *pointset, int index);
void geo_pointset_iset_z(geo_pointset_type *pointset, int index, double value);
void geo_pointset_memcpy(const geo_pointset_type *src,
                         geo_pointset_type *target, bool copy_zdata);
void geo_pointset_shift_z(geo_pointset_type *pointset, double value);
void geo_pointset_assign_z(geo_pointset_type *pointset, double value);
void geo_pointset_scale_z(geo_pointset_type *pointset, double value);
void geo_pointset_imul(geo_pointset_type *pointset,
                       const geo_pointset_type *other);
void geo_pointset_iadd(geo_pointset_type *pointset,
                       const geo_pointset_type *other);
void geo_pointset_isub(geo_pointset_type *self, const geo_pointset_type *other);
void geo_pointset_isqrt(geo_pointset_type *pointset);

#ifdef __cplusplus
}
#endif
#endif
