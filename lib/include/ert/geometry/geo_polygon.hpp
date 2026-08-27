#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <optional>
#include <vector>


namespace rd {
struct Polygon {
    std::vector<double> xcoord;
    std::vector<double> ycoord;
    std::optional<std::string> name;

    Polygon(std::optional<std::string> name)
        : name(std::move(name)) {};
};
}

void geo_polygon_add_point(rd::Polygon *polygon, double x, double y);
void geo_polygon_add_point_front(rd::Polygon *polygon, double x, double y);
rd::Polygon *geo_polygon_fload_alloc_irap(const char *filename);
bool geo_polygon_contains_point(const rd::Polygon *polygon, double x,
                                double y, bool force_edge_inside = false);
void geo_polygon_reset(rd::Polygon *polygon);
void geo_polygon_close(rd::Polygon *polygoon);
size_t geo_polygon_get_size(const rd::Polygon *polygon);
void geo_polygon_iget_xy(const rd::Polygon *polygon, int index, double *x,
                         double *y);
bool geo_polygon_segment_intersects(const rd::Polygon *polygon, double x1,
                                    double y1, double x2, double y2);
const char *geo_polygon_get_name(const rd::Polygon *polygon);
void geo_polygon_set_name(rd::Polygon *polygon, const char *name);
double geo_polygon_get_length(rd::Polygon *polygon);
bool geo_polygon_equal(const rd::Polygon *polygon1,
                       const rd::Polygon *polygon2);
