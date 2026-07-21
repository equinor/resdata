#include <cmath>
#include <cstddef>

#include <filesystem>
#include <ios>
#include <iomanip>
#include <istream>
#include <ostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>

#include <ert/util/util.hpp>

#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_surface.hpp>

#define __PI 3.14159265
struct geo_surface_struct {
    // Irap data:
    int nx, ny;
    double rot_angle; // Radians
    double origo[2];
    double cell_size[2];
    double vec1[2];
    double vec2[2];

    geo_pointset_ptr pointset{nullptr, geo_pointset_free};
};

static void geo_surface_copy_header(const geo_surface_type *src,
                                    geo_surface_type *target) {
    target->nx = src->nx;
    target->ny = src->ny;
    target->rot_angle = src->rot_angle;

    for (int i = 0; i < 2; i++) {
        target->origo[i] = src->origo[i];
        target->cell_size[i] = src->cell_size[i];
        target->vec1[i] = src->vec1[i];
        target->vec2[i] = src->vec2[i];
    }
}

static geo_surface_type *geo_surface_alloc_empty(bool internal_z) {
    geo_surface_type *surface = new geo_surface_struct();
    surface->pointset.reset(geo_pointset_alloc(internal_z));
    return surface;
}

static void
geo_surface_init_regular(geo_surface_type *surface,
                         const std::optional<std::vector<double>> &zcoord) {
    int zstride_nx = 1;
    int zstride_ny = surface->nx;

    for (int iy = 0; iy < surface->ny; iy++) {
        for (int ix = 0; ix < surface->nx; ix++) {
            double x = surface->origo[0] + ix * surface->vec1[0] +
                       iy * surface->vec2[0];
            double y = surface->origo[1] + ix * surface->vec1[1] +
                       iy * surface->vec2[1];
            if (zcoord) {
                int z_index = ix * zstride_nx + iy * zstride_ny;
                geo_pointset_add_xyz(surface->pointset.get(), x, y,
                                     (*zcoord)[z_index]);
            } else
                geo_pointset_add_xyz(surface->pointset.get(), x, y, 0);
        }
    }
}

static void geo_surface_fscanf_zcoord(std::istream &stream,
                                      std::vector<double> &zcoord) {
    for (double &z : zcoord) {
        if (!(stream >> z))
            throw std::invalid_argument(
                "Failed to read irap file: too few zcoord values");
    }
    double extra_value;
    if (stream >> extra_value)
        throw std::invalid_argument(
            "Failed to read irap file: too many zcoord values");
}

static void geo_surface_fprintf_irap_header(const geo_surface_type *surface,
                                            std::ostream &stream) {

    stream << -996 << "\n"
           << surface->ny << "\n"
           << std::setw(12) << surface->cell_size[0] << "\n"
           << std::setw(12) << surface->cell_size[1] << "\n"
           << std::setw(12) << surface->origo[0] << "\n"
           << std::setw(12)
           << surface->origo[0] + surface->cell_size[0] * (surface->nx - 1)
           << "\n"
           << std::setw(12) << surface->origo[1] << "\n"
           << std::setw(12)
           << surface->origo[1] + surface->cell_size[1] * (surface->ny - 1)
           << "\n"
           << surface->nx << "\n"
           << std::setw(12) << surface->rot_angle * 180 / __PI << "\n"
           << std::setw(12) << surface->origo[0] << "\n"
           << std::setw(12) << surface->origo[1] << "\n"
           << "0  0  0  0  0  0  0  \n";
}

static void geo_surface_fprintf_zcoord(const geo_surface_type *surface,
                                       std::ofstream &stream) {
    const auto &zcoord = geo_pointset_get_zcoord(surface->pointset.get());
    int num_columns = 6;
    for (size_t i = 0; i < zcoord.size(); i++) {
        stream << std::setw(12) << zcoord[i] << " ";

        if (((i + 1) % num_columns) == 0)
            stream << "\n";
    }
}

void geo_surface_fprintf_irap(const geo_surface_type *surface,
                              const std::string &filename) {
    std::filesystem::path path(filename);
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path());

    std::ofstream stream(filename);
    stream << std::setprecision(4) << std::fixed;
    if (!stream)
        throw std::ios_base::failure("Could not open file " + filename);

    geo_surface_fprintf_irap_header(surface, stream);
    geo_surface_fprintf_zcoord(surface, stream);
}

geo_surface_type *geo_surface_alloc_new(int nx, int ny, double xinc,
                                        double yinc, double xstart,
                                        double ystart, double angle) {
    geo_surface_ptr surface{geo_surface_alloc_empty(true), geo_surface_free};

    surface->origo[0] = xstart;
    surface->origo[1] = ystart;
    surface->rot_angle = angle * __PI / 180.0;
    surface->nx = nx;
    surface->ny = ny;

    surface->vec1[0] = xinc * cos(surface->rot_angle);
    surface->vec1[1] = xinc * sin(surface->rot_angle);

    surface->vec2[0] = -yinc * sin(surface->rot_angle);
    surface->vec2[1] = yinc * cos(surface->rot_angle);

    surface->cell_size[0] = xinc;
    surface->cell_size[1] = yinc;
    geo_surface_init_regular(surface.get(), std::nullopt);
    return surface.release();
}

static void geo_surface_fload_irap_header(geo_surface_type *surface,
                                          std::istream &stream) {
    int const996, ny, nx;
    double xinc, yinc, xstart, xend, ystart, yend, angle;

    if (!(stream >> const996 >> ny >> xinc >> yinc >> xstart >> xend >>
          ystart >> yend >> nx >> angle))
        throw std::invalid_argument("reading irap header failed");

    {
        // Some information is rewritten/not used.
        double d;
        int i;
        if (!(stream >> d >> d >> i >> i >> i >> i >> i >> i >> i))
            throw std::invalid_argument("reading irap header failed");
    }

    surface->origo[0] = xstart;
    surface->origo[1] = ystart;
    surface->rot_angle = angle * __PI / 180.0;
    surface->nx = nx;
    surface->ny = ny;

    surface->vec1[0] = xinc * cos(surface->rot_angle);
    surface->vec1[1] = xinc * sin(surface->rot_angle);

    surface->vec2[0] = -yinc * sin(surface->rot_angle);
    surface->vec2[1] = yinc * cos(surface->rot_angle);

    surface->cell_size[0] = xinc;
    surface->cell_size[1] = yinc;
}

static void geo_surface_fload_irap(geo_surface_type *surface,
                                   const std::string &filename, bool loadz) {
    std::ifstream stream(filename);
    if (!stream)
        throw std::ios_base::failure("Could not open file " + filename);
    geo_surface_fload_irap_header(surface, stream);
    {
        std::optional<std::vector<double>> zcoord = std::nullopt;
        if (loadz) {
            if (surface->nx < 0 || surface->ny < 0)
                throw std::invalid_argument("surface size was negative: " +
                                            std::to_string(surface->nx) + "x" +
                                            std::to_string(surface->ny));
            zcoord = std::make_optional<std::vector<double>>(
                static_cast<size_t>(surface->nx) *
                static_cast<size_t>(surface->ny));
            geo_surface_fscanf_zcoord(stream, *zcoord);
        }

        geo_surface_init_regular(surface, zcoord);
    }
}

bool geo_surface_equal_header(const geo_surface_type *surface1,
                              const geo_surface_type *surface2) {
    bool equal = true;

    equal = equal && (surface1->nx == surface2->nx);
    equal = equal && (surface1->ny == surface2->ny);
    equal = equal && (util_double_approx_equal(surface1->rot_angle,
                                               surface2->rot_angle));

    for (int i = 0; i < 2; i++) {
        equal = equal && (util_double_approx_equal(surface1->origo[i],
                                                   surface2->origo[i]));
        equal = equal && (util_double_approx_equal(surface1->cell_size[i],
                                                   surface2->cell_size[i]));
        equal = equal && (util_double_approx_equal(surface1->vec1[i],
                                                   surface2->vec1[i]));
        equal = equal && (util_double_approx_equal(surface1->vec2[i],
                                                   surface2->vec2[i]));
    }

    return equal;
}

geo_surface_type *geo_surface_fload_alloc_irap(const std::string &filename,
                                               bool loadz) {
    geo_surface_ptr surface{geo_surface_alloc_empty(loadz), geo_surface_free};
    geo_surface_fload_irap(surface.get(), filename, loadz);
    return surface.release();
}

geo_surface_type *geo_surface_alloc_copy(const geo_surface_type *src,
                                         bool copy_zdata) {
    geo_surface_ptr target{geo_surface_alloc_empty(true), geo_surface_free};

    geo_surface_copy_header(src, target.get());
    if (!geo_surface_equal_header(src, target.get()))
        throw std::runtime_error("headers not equal after copy");

    geo_pointset_memcpy(src->pointset.get(), target->pointset.get(),
                        copy_zdata);

    return target.release();
}

void geo_surface_free(geo_surface_type *surface) { delete surface; }

geo_pointset_type *geo_surface_get_pointset(const geo_surface_type *surface) {
    return surface->pointset.get();
}

void geo_surface_iget_xy(const geo_surface_type *surface, int index, double *x,
                         double *y) {
    const geo_pointset_type *pointset = geo_surface_get_pointset(surface);
    geo_pointset_iget_xy(pointset, index, x, y);
}

int geo_surface_get_size(const geo_surface_type *surface) {
    return geo_pointset_get_size(surface->pointset.get());
}

int geo_surface_get_nx(const geo_surface_type *surface) { return surface->nx; }

int geo_surface_get_ny(const geo_surface_type *surface) { return surface->ny; }

bool geo_surface_equal(const geo_surface_type *surface1,
                       const geo_surface_type *surface2) {
    if (geo_surface_equal_header(surface1, surface2))
        return geo_pointset_equal(surface1->pointset.get(),
                                  surface2->pointset.get());
    else
        return false;
}

double geo_surface_iget_zvalue(const geo_surface_type *surface, int index) {
    return geo_pointset_iget_z(surface->pointset.get(), index);
}

void geo_surface_iset_zvalue(geo_surface_type *surface, int index,
                             double value) {
    return geo_pointset_iset_z(surface->pointset.get(), index, value);
}

void geo_surface_assign_value(const geo_surface_type *src, double value) {
    geo_pointset_assign_z(src->pointset.get(), value);
}

void geo_surface_shift(const geo_surface_type *src, double value) {
    geo_pointset_shift_z(src->pointset.get(), value);
}

void geo_surface_scale(const geo_surface_type *src, double value) {
    geo_pointset_scale_z(src->pointset.get(), value);
}

void geo_surface_iadd(geo_surface_type *self, const geo_surface_type *other) {
    if (geo_surface_equal_header(self, other))
        geo_pointset_iadd(self->pointset.get(), other->pointset.get());
    else
        throw std::invalid_argument("tried to combine incompatible surfaces");
}

void geo_surface_isub(geo_surface_type *self, const geo_surface_type *other) {
    if (geo_surface_equal_header(self, other))
        geo_pointset_isub(self->pointset.get(), other->pointset.get());
    else
        throw std::invalid_argument("tried to combine incompatible surfaces");
}

void geo_surface_imul(geo_surface_type *self, const geo_surface_type *other) {
    if (geo_surface_equal_header(self, other))
        geo_pointset_imul(self->pointset.get(), other->pointset.get());
    else
        throw std::invalid_argument("tried to combine incompatible surfaces");
}

void geo_surface_isqrt(geo_surface_type *surface) {
    geo_pointset_isqrt(surface->pointset.get());
}

geo_surface_ptr make_geo_surface(int nx, int ny, double xinc, double yinc,
                                 double xstart, double ystart, double angle) {
    return {geo_surface_alloc_new(nx, ny, xinc, yinc, xstart, ystart, angle),
            &geo_surface_free};
}
