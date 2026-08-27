#pragma once
#include <cstddef>
#include <memory>
#include <string>

#include <ert/util/type_macros.hpp>
#include <ert/geometry/geo_polygon.hpp>

typedef struct geo_polygon_collection_struct geo_polygon_collection_type;

geo_polygon_collection_type *geo_polygon_collection_alloc();
void geo_polygon_collection_free(geo_polygon_collection_type *polygons);
size_t geo_polygon_collection_size(const geo_polygon_collection_type *polygons);
std::shared_ptr<rd::Polygon>
geo_polygon_collection_create_polygon(geo_polygon_collection_type *polygons,
                                      const char *name);
bool geo_polygon_collection_has_polygon(
    const geo_polygon_collection_type *polygons, const char *name);
bool geo_polygon_collection_add_polygon(geo_polygon_collection_type *polygons,
                                        std::shared_ptr<rd::Polygon> polygon);
std::shared_ptr<rd::Polygon>
geo_polygon_collection_iget_polygon(const geo_polygon_collection_type *polygons,
                                    size_t index);
std::shared_ptr<rd::Polygon>
geo_polygon_collection_get_polygon(const geo_polygon_collection_type *polygons,
                                   const std::string &polygon_name);

UTIL_IS_INSTANCE_HEADER(geo_polygon_collection);
