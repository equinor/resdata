#include <cmath>
#include <cstdlib>
#include <cstring>

#include <stdexcept>
#include <tuple>
#include <vector>
#include <string>
#include <optional>

#include <ert/geometry/geo_pointset.hpp>

struct geo_pointset_struct {
    std::vector<double> xcoord;
    std::vector<double> ycoord;
    std::optional<std::vector<double>> zcoord;
};

geo_pointset_type *geo_pointset_alloc(bool external_z) {
    auto pointset = new geo_pointset_type();
    if (external_z)
        pointset->zcoord = std::vector<double>();
    return pointset;
}

void geo_pointset_memcpy(const geo_pointset_type *src,
                         geo_pointset_type *target, bool copy_zdata) {
    if (src->zcoord.has_value() != target->zcoord.has_value())
        throw std::invalid_argument("when copying geo_pointset they must have "
                                    "equal value for external_z");

    target->xcoord.assign(src->xcoord.begin(), src->xcoord.end());
    target->ycoord.assign(src->ycoord.begin(), src->ycoord.end());
    if (copy_zdata) {
        if (!target->zcoord.has_value() || !src->zcoord.has_value()) {
            target->zcoord = src->zcoord;
        } else {
            target->zcoord->assign(src->zcoord->begin(), src->zcoord->end());
        }
    } else {
        if (src->zcoord.has_value())
            target->zcoord = std::vector(src->zcoord->size(), 0.0);
        else
            target->zcoord = std::nullopt;
    }
}

void geo_pointset_add_xyz(geo_pointset_type *pointset, double x, double y,
                          double z) {

    pointset->xcoord.push_back(x);
    pointset->ycoord.push_back(y);
    if (pointset->zcoord.has_value())
        pointset->zcoord->push_back(z);
}

void geo_pointset_free(geo_pointset_type *pointset) { delete pointset; }

int geo_pointset_get_size(const geo_pointset_type *pointset) {
    return pointset->xcoord.size();
}

const std::vector<double> &
geo_pointset_get_zcoord(const geo_pointset_type *pointset) {
    if (!pointset->zcoord)
        throw std::runtime_error("z coordinate not set");
    return *pointset->zcoord;
}

static void geo_pointset_assert_index(const geo_pointset_type *pointset,
                                      size_t index) {
    if (index >= pointset->xcoord.size())
        throw std::out_of_range(
            "invalid pointset index " + std::to_string(index) +
            ", size: " + std::to_string(pointset->xcoord.size()));
}

static void geo_pointset_assert_zindex(const geo_pointset_type *pointset,
                                       int index) {
    if ((index < 0) || (static_cast<size_t>(index) >= pointset->xcoord.size()))
        throw std::out_of_range(
            "invalid pointset index " + std::to_string(index) +
            ", size: " + std::to_string(pointset->xcoord.size()));

    if (!pointset->zcoord)
        throw std::runtime_error("z coordinate not set");
}

void geo_pointset_iget_xy(const geo_pointset_type *pointset, size_t index,
                          double *x, double *y) {
    geo_pointset_assert_index(pointset, index);
    *x = pointset->xcoord[index];
    *y = pointset->ycoord[index];
}

double geo_pointset_iget_z(const geo_pointset_type *pointset, int index) {
    geo_pointset_assert_zindex(pointset, index);
    return (*pointset->zcoord)[index];
}

void geo_pointset_iset_z(geo_pointset_type *pointset, int index, double value) {
    geo_pointset_assert_zindex(pointset, index);
    auto &zs = pointset->zcoord.value();
    zs[index] = value;
}

bool geo_pointset_equal(const geo_pointset_type *pointset1,
                        const geo_pointset_type *pointset2) {
    return std::tie(pointset1->xcoord, pointset1->ycoord, pointset1->zcoord) ==
           std::tie(pointset2->xcoord, pointset2->ycoord, pointset2->zcoord);
}

void geo_pointset_assign_z(geo_pointset_type *pointset, double value) {
    if (!pointset->zcoord.has_value())
        return;
    auto &zs = pointset->zcoord.value();
    for (double &z : zs)
        z = value;
}

void geo_pointset_shift_z(geo_pointset_type *pointset, double value) {
    if (!pointset->zcoord.has_value())
        return;
    auto &zs = pointset->zcoord.value();
    for (double &z : zs)
        z += value;
}

void geo_pointset_scale_z(geo_pointset_type *pointset, double value) {
    if (!pointset->zcoord.has_value())
        return;
    auto &zs = pointset->zcoord.value();
    for (double &z : zs)
        z *= value;
}

void geo_pointset_iadd(geo_pointset_type *self,
                       const geo_pointset_type *other) {
    if (self->xcoord.size() == other->xcoord.size()) {
        if (!self->zcoord || !other->zcoord)
            return;
        auto &self_zs = self->zcoord.value();
        auto &other_zs = other->zcoord.value();
        for (size_t index = 0; index < self_zs.size(); index++)
            self_zs[index] += other_zs[index];
    } else
        throw std::invalid_argument("size mismatch");
}

void geo_pointset_isub(geo_pointset_type *self,
                       const geo_pointset_type *other) {
    if (self->xcoord.size() == other->xcoord.size()) {
        if (!self->zcoord || !other->zcoord)
            return;
        auto &self_zs = self->zcoord.value();
        auto &other_zs = other->zcoord.value();
        for (size_t index = 0; index < self_zs.size(); index++)
            self_zs[index] -= other_zs[index];
    } else
        throw std::invalid_argument("size mismatch");
}

void geo_pointset_imul(geo_pointset_type *self,
                       const geo_pointset_type *other) {
    if (self->xcoord.size() == other->xcoord.size()) {
        if (!self->zcoord || !other->zcoord)
            return;
        auto &self_zs = self->zcoord.value();
        auto &other_zs = other->zcoord.value();
        for (size_t index = 0; index < self_zs.size(); index++)
            self_zs[index] *= other_zs[index];
    } else
        throw std::invalid_argument("size mismatch");
}

void geo_pointset_isqrt(geo_pointset_type *pointset) {
    if (!pointset->zcoord.has_value())
        return;
    for (size_t i = 0; i < pointset->zcoord->size(); i++)
        (*pointset->zcoord)[i] = sqrt((*pointset->zcoord)[i]);
}

geo_pointset_ptr make_geo_pointset(bool external_z) {
    return {geo_pointset_alloc(external_z), &geo_pointset_free};
}
