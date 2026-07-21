#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <array>
#include <fstream>
#include <ios>
#include <string>
#include <utility>
#include <vector>
#include <optional>

#include <ert/util/util.hpp>
#include <ert/util/type_macros.hpp>

#include <ert/geometry/geo_polygon.hpp>

#define GEO_POLYGON_TYPE_ID 9951322

struct geo_polygon_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::vector<double> xcoord;
    std::vector<double> ycoord;
    std::optional<std::string> name;

    geo_polygon_struct(std::optional<std::string> name)
        : name(std::move(name)) {};
};

static UTIL_SAFE_CAST_FUNCTION(geo_polygon, GEO_POLYGON_TYPE_ID);
UTIL_IS_INSTANCE_FUNCTION(geo_polygon, GEO_POLYGON_TYPE_ID);

geo_polygon_type *geo_polygon_alloc(const char *name) {
    geo_polygon_struct *polygon = nullptr;
    if (name)
        polygon = new geo_polygon_struct(name);
    else
        polygon = new geo_polygon_struct(std::nullopt);

    UTIL_TYPE_ID_INIT(polygon, GEO_POLYGON_TYPE_ID);
    return polygon;
}

void geo_polygon_free(geo_polygon_type *polygon) { delete polygon; }

void geo_polygon_free__(void *arg) {
    geo_polygon_type *polygon = geo_polygon_safe_cast(arg);
    geo_polygon_free(polygon);
}

void geo_polygon_add_point(geo_polygon_type *polygon, double x, double y) {
    polygon->xcoord.push_back(x);
    polygon->ycoord.push_back(y);
}

void geo_polygon_add_point_front(geo_polygon_type *polygon, double x,
                                 double y) {
    polygon->xcoord.insert(polygon->xcoord.begin(), x);
    polygon->ycoord.insert(polygon->ycoord.begin(), y);
}

void geo_polygon_close(geo_polygon_type *polygon) {
    double x = polygon->xcoord.at(0);
    double y = polygon->ycoord.at(0);
    geo_polygon_add_point(polygon, x, y);
}

static bool on_edge(double x1, double y1, double x2, double y2, double x0,
                    double y0) {
    double xmin = util_double_min(x1, x2);
    double xmax = util_double_max(x1, x2);
    double ymin = util_double_min(y1, y2);
    double ymax = util_double_max(y1, y2);

    /* Vertical line */
    if (x1 == x2)
        return (x0 == x1) && (ymin <= y0) && (y0 <= ymax);

    /* Horizontal line */
    if (y1 == y2)
        return (xmin <= x0) && (x0 <= xmax) && (y0 == y1);

    /* General slope */
    double a = (y2 - y1) / (x2 - x1);
    double yc = a * (x0 - x1) + y1;

    return (yc == y0) && (xmin <= x0) && (x0 <= xmax) && (ymin <= y0) &&
           (y0 <= ymax);
}

/*
  If the bool force_edge_inside is set to true, points exactly on the
  edge will be identified as inside. If the force_edge_inside variable
  is set to false the behaviour on the edges is undefined.
*/
bool geo_polygon_contains_point(const geo_polygon_type *polygon, double x0,
                                double y0, bool force_edge_inside) {
    bool inside = false;
    double y = y0;
    double xc = 0;

    const size_t num_points = polygon->xcoord.size();

    for (size_t i = 0; i < num_points; i++) {
        const size_t next_point = ((i + 1) % num_points);
        double x1 = polygon->xcoord[i];
        double y1 = polygon->ycoord[i];
        double x2 = polygon->xcoord[next_point];
        double y2 = polygon->ycoord[next_point];

        double ymin = util_double_min(y1, y2);
        double ymax = util_double_max(y1, y2);
        double xmax = util_double_max(x1, x2);

        if (force_edge_inside) {
            if (on_edge(x1, y1, x2, y2, x0, y0)) {
                inside = true;
                break;
            }
        }

        if ((x1 == x2) && (y1 == y2))
            continue;

        if ((y0 > ymin) && (y <= ymax)) {

            if (x0 <= xmax) {
                if (y1 != y2)
                    xc = (y0 - y1) * (x2 - x1) / (y2 - y1) + x1;

                if ((x1 == x2) || (x0 <= xc))
                    inside = !inside;
            }
        }
    }

    return inside;
}

/*
  The irap format is a polygon which closes on itself by construction,
  and the list of numbers is terminated with (999,999,999). This is
  supported as follows:

    - Reading will stop at (999,999,999) - all points after this
      triplet will be ignored.

    - The polyline will by construction close on itself, i.e. P0 ==
      PN. Iff the last point is identical to the first it will not be
      included.
*/

geo_polygon_type *geo_polygon_fload_alloc_irap(const char *filename) {
    std::string sfile{filename};
    std::ifstream stream{sfile};
    if (!stream)
        throw std::ios_base::failure("Failed to open: " + sfile);

    geo_polygon_ptr polygon{geo_polygon_alloc(filename), geo_polygon_free};
    double x, y, z;

    while (stream >> x >> y >> z) {
        if ((x == 999) && (y == 999) && (z == 999))
            break;

        geo_polygon_add_point(polygon.get(), x, y);
    }

    if ((polygon->xcoord.size() > 1)) {
        if ((polygon->xcoord.at(polygon->xcoord.size() - 1) ==
             polygon->xcoord.at(0)) &&
            (polygon->ycoord.at(polygon->ycoord.size() - 1) ==
             polygon->ycoord.at(0))) {

            polygon->xcoord.pop_back();
            polygon->ycoord.pop_back();
        }
    }
    return polygon.release();
}

void geo_polygon_reset(geo_polygon_type *polygon) {
    polygon->xcoord.resize(0);
    polygon->ycoord.resize(0);
}

size_t geo_polygon_get_size(const geo_polygon_type *polygon) {
    return polygon->xcoord.size();
}

void geo_polygon_iget_xy(const geo_polygon_type *polygon, int index, double *x,
                         double *y) {
    *x = polygon->xcoord.at(index);
    *y = polygon->ycoord.at(index);
}

enum XLinesStatus {
    CROSSING = 0,
    PARALLELL = 1,
    OVERLAPPING = 2,
    DEGENERATE = 3,
    NOT_CROSSING = 4
};

static bool between(double v1, double v2, double v) {
    return (((v > v1) && (v < v2)) || ((v < v1) && (v > v2)));
}

static bool interval_overlap(double a1, double a2, double b1, double b2) {
    if (between(a1, a2, b1) || between(a1, a2, b2))
        return true;

    if ((a1 == b1) && (a2 == b2))
        return true;

    return false;
}

static XLinesStatus xsegments(const std::array<std::array<double, 2>, 4> points,
                              double *x0, double *y0, double epsilon = 1e-6) {
    double x1 = points[0][0];
    double x2 = points[1][0];
    double x3 = points[2][0];
    double x4 = points[3][0];

    double y1 = points[0][1];
    double y2 = points[1][1];
    double y3 = points[2][1];
    double y4 = points[3][1];

    double denominator = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);
    double numerator_a = (x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3);
    double numerator_b = (x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3);

    // coincident?
    if ((fabs(numerator_a) < epsilon) && (fabs(numerator_b) < epsilon) &&
        (fabs(denominator) < epsilon)) {

        if (interval_overlap(x1, x2, x3, x4) &&
            interval_overlap(y1, y2, y3, y4))
            return OVERLAPPING;
        else
            return NOT_CROSSING;
    }

    // Parallell
    if (fabs(denominator) < epsilon)
        return NOT_CROSSING;

    // Normal intersection
    {
        double mua = numerator_a / denominator;
        double mub = numerator_b / denominator;

        if ((mua < 0.0) || (mua > 1.0) || (mub < 0.0) || (mub > 1.0))
            return NOT_CROSSING;

        *x0 = x1 + mua * (x2 - x1);
        *y0 = y1 + mua * (y2 - y1);

        return CROSSING;
    }
}

bool geo_polygon_segment_intersects(const geo_polygon_type *polygon, double x1,
                                    double y1, double x2, double y2) {
    if (polygon->xcoord.empty())
        return false;
    std::array<std::array<double, 2>, 4> points{};

    points[0][0] = x1;
    points[1][0] = x2;
    points[0][1] = y1;
    points[1][1] = y2;

    for (size_t index = 0; index < polygon->xcoord.size() - 1; index++) {
        double xc, yc;

        points[2][0] = polygon->xcoord.at(index);
        points[3][0] = polygon->xcoord.at(index + 1);
        points[2][1] = polygon->ycoord.at(index);
        points[3][1] = polygon->ycoord.at(index + 1);

        {
            auto xline_status = xsegments(points, &xc, &yc);
            if ((xline_status == CROSSING) || (xline_status == OVERLAPPING))
                return true;
        }
    }
    return false;
}

const char *geo_polygon_get_name(const geo_polygon_type *polygon) {
    return polygon->name.has_value() ? (*polygon->name).c_str() : nullptr;
}

void geo_polygon_set_name(geo_polygon_type *polygon, const char *name) {
    if (name)
        polygon->name = name;
    else
        polygon->name = std::nullopt;
}

double geo_polygon_get_length(geo_polygon_type *polygon) {
    if (polygon->xcoord.size() == 1)
        return 0;
    else {
        double length = 0;
        double x0 = polygon->xcoord.at(0);
        double y0 = polygon->ycoord.at(0);

        for (size_t i = 1; i < polygon->xcoord.size(); i++) {
            double x1 = polygon->xcoord.at(i);
            double y1 = polygon->ycoord.at(i);

            length += sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
            x0 = x1;
            y0 = y1;
        }
        return length;
    }
}

static bool approx_equal(const std::vector<double> &a,
                         const std::vector<double> &b, double epsilon = 1e-8) {
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i) {
        if (std::abs(a[i] - b[i]) > epsilon) {
            return false;
        }
    }
    return true;
}
/*
  Name is ignored in the comparison.
*/
bool geo_polygon_equal(const geo_polygon_type *polygon1,
                       const geo_polygon_type *polygon2) {
    bool equal = polygon1->xcoord == polygon2->xcoord &&
                 polygon1->ycoord == polygon2->ycoord;

    if (!equal) {
        equal = approx_equal(polygon1->xcoord, polygon2->xcoord) &&
                approx_equal(polygon1->ycoord, polygon2->ycoord);
    }

    return equal;
}

geo_polygon_ptr make_geo_polygon(std::string name) {
    return {geo_polygon_alloc(name.c_str()), &geo_polygon_free};
}
