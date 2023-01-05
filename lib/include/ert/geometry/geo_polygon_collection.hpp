#ifndef ERT_GEO_POLYGON_COLLECTION_H
#define ERT_GEO_POLYGON_COLLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <ert/util/type_macros.hpp>

#include <ert/geometry/geo_polygon.hpp>

typedef struct geo_polygon_collection_struct geo_polygon_collection_type;

geo_polygon_collection_type *geo_polygon_collection_alloc();
void geo_polygon_collection_free(geo_polygon_collection_type *polygons);
int geo_polygon_collection_size(const geo_polygon_collection_type *polygons);
geo_polygon_type *
geo_polygon_collection_create_polygon(geo_polygon_collection_type *polygons,
                                      const char *name);
bool geo_polygon_collection_has_polygon(
    const geo_polygon_collection_type *polygons, const char *name);
bool geo_polygon_collection_add_polygon(geo_polygon_collection_type *polygons,
                                        geo_polygon_type *polygon,
                                        bool polygon_owner);
geo_polygon_type *
geo_polygon_collection_iget_polygon(const geo_polygon_collection_type *polygons,
                                    int index);
geo_polygon_type *
geo_polygon_collection_get_polygon(const geo_polygon_collection_type *polygons,
                                   const char *polygon_name);

UTIL_IS_INSTANCE_HEADER(geo_polygon_collection);

#ifdef __cplusplus
}
#endif
#endif
