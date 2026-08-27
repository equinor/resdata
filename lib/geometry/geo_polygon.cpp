#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <algorithm>
#include <array>
#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <optional>

#include <ert/util/util.hpp>

#include <ert/geometry/geo_polygon.hpp>

static bool on_edge(double x1, double y1, double x2, double y2, double x0,
                    double y0) {
    double xmin = std::min(x1, x2);
    double xmax = std::max(x1, x2);
    double ymin = std::min(y1, y2);
    double ymax = std::max(y1, y2);

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
bool rd::Polygon::contains_point(double x0, double y0,
                                 bool force_edge_inside) const {
    bool inside = false;
    double y = y0;
    double xc = 0;

    const size_t num_points = points.size();

    for (size_t i = 0; i < num_points; i++) {
        const size_t next_point = ((i + 1) % num_points);
        auto [x1, y1] = points[i];
        auto [x2, y2] = points[next_point];

        double ymin = std::min(y1, y2);
        double ymax = std::max(y1, y2);
        double xmax = std::max(x1, x2);

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

std::shared_ptr<rd::Polygon> rd::Polygon::load_irap(const std::string &sfile) {
    std::ifstream stream{sfile};
    if (!stream)
        throw std::ios_base::failure("Failed to open: " + sfile);

    auto polygon = std::make_shared<rd::Polygon>(sfile);
    double x, y, z;

    while (stream >> x >> y >> z) {
        if ((x == 999) && (y == 999) && (z == 999))
            break;
        polygon->add_point(x, y);
    }

    if ((polygon->size() > 1))
        if (polygon->points.back() == polygon->points[0])
            polygon->points.pop_back();
    return polygon;
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
                              double epsilon = 1e-6) {
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

    // Parallel
    if (fabs(denominator) < epsilon)
        return NOT_CROSSING;

    // Normal intersection
    {
        double mua = numerator_a / denominator;
        double mub = numerator_b / denominator;

        if ((mua < 0.0) || (mua > 1.0) || (mub < 0.0) || (mub > 1.0))
            return NOT_CROSSING;
        return CROSSING;
    }
}

bool rd::Polygon::segment_intersects(double x1, double y1, double x2,
                                     double y2) const {
    if (empty())
        return false;
    std::array<std::array<double, 2>, 4> lpoints{};

    lpoints[0][0] = x1;
    lpoints[1][0] = x2;
    lpoints[0][1] = y1;
    lpoints[1][1] = y2;

    for (size_t index = 0; index < points.size() - 1; index++) {

        auto [x, y] = points.at(index);
        auto [xn, yn] = points.at(index + 1);
        lpoints[2][0] = x;
        lpoints[3][0] = xn;
        lpoints[2][1] = y;
        lpoints[3][1] = yn;

        auto xline_status = xsegments(lpoints);
        if ((xline_status == CROSSING) || (xline_status == OVERLAPPING))
            return true;
    }
    return false;
}

double rd::Polygon::length() const {
    if (size() <= 1)
        return 0;
    else {
        double length = 0;
        auto [x0, y0] = points[0];

        for (size_t i = 1; i < size(); i++) {
            auto [x1, y1] = points[i];
            length += sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
            x0 = x1;
            y0 = y1;
        }
        return length;
    }
}

static bool approx_equal(const std::vector<std::tuple<double, double>> &a,
                         const std::vector<std::tuple<double, double>> &b,
                         double epsilon = 1e-8) {
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i) {
        if (std::abs(std::get<0>(a[i]) - std::get<0>(b[i])) > epsilon) {
            return false;
        }
        if (std::abs(std::get<1>(a[i]) - std::get<1>(b[i])) > epsilon) {
            return false;
        }
    }
    return true;
}
/*
  Name is ignored in the comparison.
*/
bool rd::Polygon::operator==(const rd::Polygon &other) const {
    return approx_equal(this->points, other.points);
}
