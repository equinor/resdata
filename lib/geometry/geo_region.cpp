#include <cstdlib>
#include <vector>

#include <ert/util/util.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/type_macros.hpp>

#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_region.hpp>
#include <ert/geometry/geo_polygon.hpp>

struct geo_region_struct {
    bool preselect;
    bool index_valid;
    std::vector<bool> active_mask;
    int_vector_ptr index_list{nullptr, int_vector_free};
    const geo_pointset_type *pointset;

    [[nodiscard]] size_t size() const { return active_mask.size(); }
};

geo_region_type *geo_region_alloc(const geo_pointset_type *pointset,
                                  bool preselect) {
    auto region = new geo_region_type();

    region->pointset = pointset;
    region->preselect = preselect;
    region->index_list.reset(int_vector_alloc(0, 0));
    region->active_mask.resize(geo_pointset_get_size(pointset));
    geo_region_reset(region);

    return region;
}

static void geo_region_invalidate_index_list(geo_region_type *region) {
    region->index_valid = false;
}

static void geo_region_assert_index_list(geo_region_type *region) {
    if (!region->index_valid) {
        int_vector_reset(region->index_list.get());
        for (size_t i = 0; i < region->size(); i++)
            if (region->active_mask[i])
                int_vector_append(region->index_list.get(),
                                  static_cast<int>(i));

        region->index_valid = true;
    }
}

void geo_region_reset(geo_region_type *region) {
    for (size_t i = 0; i < region->size(); i++)
        region->active_mask[i] = region->preselect;
    geo_region_invalidate_index_list(region);
}

void geo_region_free(geo_region_type *region) { delete region; }

static void geo_region_polygon_select__(geo_region_type *region,
                                        const geo_polygon_type *polygon,
                                        bool select_inside, bool select) {

    for (size_t i = 0; i < region->size(); i++) {
        double x, y;
        bool is_inside;
        geo_pointset_iget_xy(region->pointset, i, &x, &y);

        is_inside = geo_polygon_contains_point(polygon, x, y);
        if (is_inside == select_inside)
            region->active_mask[i] = select;
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

    for (size_t i = 0; i < region->size(); i++) {
        bool above;
        double x, y;
        double px, py;

        geo_pointset_iget_xy(region->pointset, i, &x, &y);
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
            region->active_mask[i] = select;
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
    return region->index_list.get();
}

/** Note that the geo_region is borrowing the pointset and
 * the lifetime of the pointset has to outlive the geo_region */
geo_region_ptr make_geo_region(const geo_pointset_ptr &pointset,
                               bool preselect) {
    return {geo_region_alloc(pointset.get(), preselect), &geo_region_free};
}
