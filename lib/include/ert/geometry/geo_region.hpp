#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/int_vector.hpp>

#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_polygon.hpp>

#ifndef ERT_GEO_REGION_H
#define ERT_GEO_REGION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geo_region_struct geo_region_type;

geo_region_type *geo_region_alloc(const geo_pointset_type *pointset,
                                  bool preselect);
void geo_region_free(geo_region_type *region);
void geo_region_free__(void *arg);
void geo_region_reset(geo_region_type *region);
const int_vector_type *geo_region_get_index_list(geo_region_type *region);

void geo_region_select_inside_polygon(geo_region_type *region,
                                      const geo_polygon_type *polygon);
void geo_region_select_outside_polygon(geo_region_type *region,
                                       const geo_polygon_type *polygon);
void geo_region_deselect_inside_polygon(geo_region_type *region,
                                        const geo_polygon_type *polygon);
void geo_region_deselect_outside_polygon(geo_region_type *region,
                                         const geo_polygon_type *polygon);

void geo_region_select_above_line(geo_region_type *region,
                                  const double xcoords[2],
                                  const double ycoords[2]);
void geo_region_select_below_line(geo_region_type *region,
                                  const double xcoords[2],
                                  const double ycoords[2]);
void geo_region_deselect_above_line(geo_region_type *region,
                                    const double xcoords[2],
                                    const double ycoords[2]);
void geo_region_deselect_below_line(geo_region_type *region,
                                    const double xcoords[2],
                                    const double ycoords[2]);

#ifdef __cplusplus
}
#endif
#endif
