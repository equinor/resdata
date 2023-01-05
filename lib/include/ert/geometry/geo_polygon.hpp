#ifndef ERT_GEO_POLYGON_H
#define ERT_GEO_POLYGON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <ert/util/type_macros.hpp>

typedef struct geo_polygon_struct geo_polygon_type;

geo_polygon_type *geo_polygon_alloc(const char *name);
void geo_polygon_free(geo_polygon_type *polygon);
void geo_polygon_free__(void *arg);
void geo_polygon_add_point(geo_polygon_type *polygon, double x, double y);
void geo_polygon_add_point_front(geo_polygon_type *polygon, double x, double y);
geo_polygon_type *geo_polygon_fload_alloc_irap(const char *filename);
bool geo_polygon_contains_point(const geo_polygon_type *polygon, double x,
                                double y);
bool geo_polygon_contains_point__(const geo_polygon_type *polygon, double x,
                                  double y, bool force_edge_inside);
void geo_polygon_reset(geo_polygon_type *polygon);
void geo_polygon_fprintf(const geo_polygon_type *polygon, FILE *stream);
void geo_polygon_shift(geo_polygon_type *polygon, double x0, double y0);
void geo_polygon_close(geo_polygon_type *polygoon);
int geo_polygon_get_size(const geo_polygon_type *polygon);
void geo_polygon_iget_xy(const geo_polygon_type *polygon, int index, double *x,
                         double *y);
bool geo_polygon_segment_intersects(const geo_polygon_type *polygon, double x1,
                                    double y1, double x2, double y2);
const char *geo_polygon_get_name(const geo_polygon_type *polygon);
void geo_polygon_set_name(geo_polygon_type *polygon, const char *name);
double geo_polygon_get_length(geo_polygon_type *polygon);
bool geo_polygon_equal(const geo_polygon_type *polygon1,
                       const geo_polygon_type *polygon2);

UTIL_IS_INSTANCE_HEADER(geo_polygon);

#ifdef __cplusplus
}
#endif
#endif
