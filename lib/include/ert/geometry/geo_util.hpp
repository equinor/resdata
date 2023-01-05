#ifndef ERT_GEO_UTIL_H
#define ERT_GEO_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

typedef enum {
    GEO_UTIL_LINES_CROSSING = 0,
    GEO_UTIL_LINES_PARALLELL = 1,
    GEO_UTIL_LINES_OVERLAPPING = 2,
    GEO_UTIL_LINES_DEGENERATE = 3,
    GEO_UTIL_NOT_CROSSING = 4
} geo_util_xlines_status_enum;

bool geo_util_inside_polygon__(const double *xlist, const double *ylist,
                               int num_points, double x0, double y0,
                               bool force_edge_inside);
bool geo_util_inside_polygon(const double *xlist, const double *ylist,
                             int num_points, double x0, double y0);
geo_util_xlines_status_enum geo_util_xlines(const double **points, double *x0,
                                            double *y0);
geo_util_xlines_status_enum geo_util_xsegments(const double **points,
                                               double *x0, double *y0);

#ifdef __cplusplus
}
#endif

#endif
