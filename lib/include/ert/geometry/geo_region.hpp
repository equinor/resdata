#pragma once
#include <cstdlib>

#include <memory>

#include <ert/util/util.hpp>

#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_polygon.hpp>
#include <vector>

typedef struct geo_region_struct geo_region_type;

geo_region_type *geo_region_alloc(const geo_pointset_type *pointset,
                                  bool preselect);
void geo_region_free(geo_region_type *region);
void geo_region_reset(geo_region_type *region);
size_t geo_region_num_selected(geo_region_type *region);
const std::vector<int> geo_region_get_index_list(geo_region_type *region);

void geo_region_select_inside_polygon(geo_region_type *region,
                                      const rd::Polygon *polygon);
void geo_region_select_outside_polygon(geo_region_type *region,
                                       const rd::Polygon *polygon);
void geo_region_deselect_inside_polygon(geo_region_type *region,
                                        const rd::Polygon *polygon);
void geo_region_deselect_outside_polygon(geo_region_type *region,
                                         const rd::Polygon *polygon);

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
using geo_region_ptr =
    std::unique_ptr<geo_region_type, decltype(&geo_region_free)>;
geo_region_ptr make_geo_region(const geo_pointset_ptr &pointset,
                               bool preselect);
