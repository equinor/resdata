#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <optional>
#include <vector>

namespace rd {
class Polygon {
private:
    std::vector<std::tuple<double, double>> points;
    std::optional<std::string> name;

public:
    explicit Polygon(std::optional<std::string> name)
        : name(std::move(name)) {};

    void add_point(std::tuple<double, double> p) { points.push_back(p); }
    void add_point(double x, double y) { points.emplace_back(x, y); }
    void add_point_front(double x, double y) {
        points.emplace(points.begin(), x, y);
    }
    bool empty() const { return points.empty(); }
    void close() { add_point(points.at(0)); }
    bool contains_point(double x0, double y0,
                        bool force_edge_inside = false) const;
    static std::shared_ptr<rd::Polygon> load_irap(const std::string &filename);
    void clear() { points.clear(); }
    [[nodiscard]] size_t size() const { return points.size(); }
    [[nodiscard]] std::tuple<double, double> operator[](size_t index) const {
        return points.at(index);
    }
    [[nodiscard]] bool segment_intersects(double x1, double y1, double x2,
                                          double y2) const;
    std::optional<std::string> get_name() const { return name; }
    void set_name(const std::optional<std::string> &name) { this->name = name; }
    double length() const;
    [[nodiscard]] bool operator==(const rd::Polygon &other) const;
};
} // namespace rd
