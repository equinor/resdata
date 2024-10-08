#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>
#include <ert/util/type_macros.hpp>

#include <ert/geometry/geo_util.hpp>
#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_region.hpp>
#include <ert/geometry/geo_polygon.hpp>

#define GEO_REGION_TYPE_ID 4431973

struct geo_region_struct {
    UTIL_TYPE_ID_DECLARATION;
    bool preselect;
    bool index_valid;
    bool *active_mask;
    int_vector_type *index_list;
    const geo_pointset_type *pointset;
    int pointset_size;
};

static UTIL_SAFE_CAST_FUNCTION(geo_region, GEO_REGION_TYPE_ID)

    geo_region_type *geo_region_alloc(const geo_pointset_type *pointset,
                                      bool preselect) {
    geo_region_type *region = (geo_region_type *)util_malloc(sizeof *region);
    UTIL_TYPE_ID_INIT(region, GEO_REGION_TYPE_ID);

    region->pointset = pointset;
    region->pointset_size = geo_pointset_get_size(pointset);
    region->preselect = preselect;
    region->index_list = int_vector_alloc(0, 0);
    region->active_mask =
        (bool *)util_calloc(region->pointset_size, sizeof *region->active_mask);
    geo_region_reset(region);

    return region;
}

static void geo_region_invalidate_index_list(geo_region_type *region) {
    region->index_valid = false;
}

static void geo_region_assert_index_list(geo_region_type *region) {
    if (!region->index_valid) {
        int i;
        int_vector_reset(region->index_list);
        for (i = 0; i < region->pointset_size; i++)
            if (region->active_mask[i])
                int_vector_append(region->index_list, i);

        region->index_valid = true;
    }
}

void geo_region_reset(geo_region_type *region) {
    int i;
    for (i = 0; i < region->pointset_size; i++)
        region->active_mask[i] = region->preselect;
    geo_region_invalidate_index_list(region);
}

void geo_region_free(geo_region_type *region) {
    int_vector_free(region->index_list);
    free(region->active_mask);
    free(region);
}

static void geo_region_polygon_select__(geo_region_type *region,
                                        const geo_polygon_type *polygon,
                                        bool select_inside, bool select) {

    int index;
    for (index = 0; index < region->pointset_size; index++) {

        double x, y;
        bool is_inside;
        geo_pointset_iget_xy(region->pointset, index, &x, &y);

        is_inside = geo_polygon_contains_point(polygon, x, y);
        if (is_inside == select_inside)
            region->active_mask[index] = select;
    }
    geo_region_invalidate_index_list(region);
}

void geo_region_select_inside_polygon(geo_region_type *region,
                                      const geo_polygon_type *polygon) {
    geo_region_polygon_select__(region, polygon, true, true);
}

void geo_region_select_outside_polygon(geo_region_type *region,
                                       const geo_polygon_type *polygon) {
    geo_region_polygon_select__(region, polygon, false, true);
}

void geo_region_deselect_inside_polygon(geo_region_type *region,
                                        const geo_polygon_type *polygon) {
    geo_region_polygon_select__(region, polygon, true, false);
}

void geo_region_deselect_outside_polygon(geo_region_type *region,
                                         const geo_polygon_type *polygon) {
    geo_region_polygon_select__(region, polygon, false, false);
}

static void geo_region_select_line__(geo_region_type *region,
                                     const double xcoords[2],
                                     const double ycoords[2], bool select_above,
                                     bool select) {
    double vx = xcoords[1] - xcoords[0]; // Vector from point 1 to point 2
    double vy = ycoords[1] - ycoords[0];
    int index;

    for (index = 0; index < region->pointset_size; index++) {
        bool above;
        double x, y;
        double px, py;

        geo_pointset_iget_xy(region->pointset, index, &x, &y);
        px = x - xcoords[0]; // Vector from point on line to (x,y)
        py = y - ycoords[0];

        // We are interested in the dot product between the vector p, and
        // the vector rot90(v) = [vy , -vx]
        {
            double distance = px * vy - vx * py;
            if (distance >= 0)
                above = true;
            else
                above = false;
        }

        if (above == select_above)
            region->active_mask[index] = select;
    }
    geo_region_invalidate_index_list(region);
}

/*
  Functions to select and deselect all points which are above a
  line. The concept 'above' is defined as follows:

    1. We define the oriented line going from (xcoords[0] ,
       ycoords[0]) -> (xcoords[1] , ycoords[1]).

    2. It is a right hand system where 'above' means that the distance
       to the line is positive.

*/

void geo_region_select_above_line(geo_region_type *region,
                                  const double xcoords[2],
                                  const double ycoords[2]) {
    geo_region_select_line__(region, xcoords, ycoords, true, true);
}

void geo_region_select_below_line(geo_region_type *region,
                                  const double xcoords[2],
                                  const double ycoords[2]) {
    geo_region_select_line__(region, xcoords, ycoords, false, true);
}

void geo_region_deselect_above_line(geo_region_type *region,
                                    const double xcoords[2],
                                    const double ycoords[2]) {
    geo_region_select_line__(region, xcoords, ycoords, true, false);
}

void geo_region_deselect_below_line(geo_region_type *region,
                                    const double xcoords[2],
                                    const double ycoords[2]) {
    geo_region_select_line__(region, xcoords, ycoords, false, false);
}

const int_vector_type *geo_region_get_index_list(geo_region_type *region) {
    geo_region_assert_index_list(region);
    return region->index_list;
}
