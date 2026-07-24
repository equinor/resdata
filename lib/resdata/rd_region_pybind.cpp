#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/layer.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_region.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
const double *plane_vector_data(const std::vector<double> &values,
                                const char *name) {
    if (values.size() != 3)
        throw py::value_error(std::string(name) +
                              " must contain exactly 3 values");
    return values.data();
}
} // namespace

namespace {
PYBIND11_MODULE(_rd_region, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for ResdataRegion";

    m.def(
        "_alloc",
        [](py::handle grid, bool preselect) {
            return reinterpret_cast<std::uintptr_t>(
                rd_region_alloc(from_cwrap<rd_grid_type>(grid), preselect));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_copy",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_region_alloc_copy(from_cwrap<rd_region_type>(self)));
        },
        py::return_value_policy::reference);

    m.def("_set_kw_int", [](py::handle self, py::handle kw, int value,
                            bool force_active) {
        rd_region_set_kw_int(from_cwrap<rd_region_type>(self),
                             from_cwrap<rd_kw_type>(kw), value, force_active);
    });
    m.def("_set_kw_float", [](py::handle self, py::handle kw, float value,
                              bool force_active) {
        rd_region_set_kw_float(from_cwrap<rd_region_type>(self),
                               from_cwrap<rd_kw_type>(kw), value, force_active);
    });
    m.def("_set_kw_double",
          [](py::handle self, py::handle kw, double value, bool force_active) {
              rd_region_set_kw_double(from_cwrap<rd_region_type>(self),
                                      from_cwrap<rd_kw_type>(kw), value,
                                      force_active);
          });
    m.def("_shift_kw_int", [](py::handle self, py::handle kw, int value,
                              bool force_active) {
        rd_region_shift_kw_int(from_cwrap<rd_region_type>(self),
                               from_cwrap<rd_kw_type>(kw), value, force_active);
    });
    m.def("_shift_kw_float",
          [](py::handle self, py::handle kw, float value, bool force_active) {
              rd_region_shift_kw_float(from_cwrap<rd_region_type>(self),
                                       from_cwrap<rd_kw_type>(kw), value,
                                       force_active);
          });
    m.def("_shift_kw_double",
          [](py::handle self, py::handle kw, double value, bool force_active) {
              rd_region_shift_kw_double(from_cwrap<rd_region_type>(self),
                                        from_cwrap<rd_kw_type>(kw), value,
                                        force_active);
          });
    m.def("_scale_kw_int", [](py::handle self, py::handle kw, int value,
                              bool force_active) {
        rd_region_scale_kw_int(from_cwrap<rd_region_type>(self),
                               from_cwrap<rd_kw_type>(kw), value, force_active);
    });
    m.def("_scale_kw_float",
          [](py::handle self, py::handle kw, float value, bool force_active) {
              rd_region_scale_kw_float(from_cwrap<rd_region_type>(self),
                                       from_cwrap<rd_kw_type>(kw), value,
                                       force_active);
          });
    m.def("_scale_kw_double",
          [](py::handle self, py::handle kw, double value, bool force_active) {
              rd_region_scale_kw_double(from_cwrap<rd_region_type>(self),
                                        from_cwrap<rd_kw_type>(kw), value,
                                        force_active);
          });
    m.def("_sum_kw_int", [](py::handle self, py::handle kw, bool force_active) {
        return rd_region_sum_kw_int(from_cwrap<rd_region_type>(self),
                                    from_cwrap<rd_kw_type>(kw), force_active);
    });
    m.def("_sum_kw_float", [](py::handle self, py::handle kw,
                              bool force_active) {
        return rd_region_sum_kw_float(from_cwrap<rd_region_type>(self),
                                      from_cwrap<rd_kw_type>(kw), force_active);
    });
    m.def("_sum_kw_double",
          [](py::handle self, py::handle kw, bool force_active) {
              return rd_region_sum_kw_double(from_cwrap<rd_region_type>(self),
                                             from_cwrap<rd_kw_type>(kw),
                                             force_active);
          });
    m.def("_sum_kw_bool", [](py::handle self, py::handle kw,
                             bool force_active) {
        return rd_region_sum_kw_int(from_cwrap<rd_region_type>(self),
                                    from_cwrap<rd_kw_type>(kw), force_active);
    });

    m.def("_free", [](py::handle self) {
        rd_region_free(from_cwrap<rd_region_type>(self));
    });
    m.def("_reset", [](py::handle self) {
        rd_region_reset(from_cwrap<rd_region_type>(self));
    });
    m.def("_select_all", [](py::handle self) {
        rd_region_select_all(from_cwrap<rd_region_type>(self));
    });
    m.def("_deselect_all", [](py::handle self) {
        rd_region_deselect_all(from_cwrap<rd_region_type>(self));
    });
    m.def("_select_equal", [](py::handle self, py::handle kw, int value) {
        rd_region_select_equal(from_cwrap<rd_region_type>(self),
                               from_cwrap<rd_kw_type>(kw), value);
    });
    m.def("_deselect_equal", [](py::handle self, py::handle kw, int value) {
        rd_region_deselect_equal(from_cwrap<rd_region_type>(self),
                                 from_cwrap<rd_kw_type>(kw), value);
    });
    m.def("_select_less", [](py::handle self, py::handle kw, float limit) {
        rd_region_select_smaller(from_cwrap<rd_region_type>(self),
                                 from_cwrap<rd_kw_type>(kw), limit);
    });
    m.def("_deselect_less", [](py::handle self, py::handle kw, float limit) {
        rd_region_deselect_smaller(from_cwrap<rd_region_type>(self),
                                   from_cwrap<rd_kw_type>(kw), limit);
    });
    m.def("_select_more", [](py::handle self, py::handle kw, float limit) {
        rd_region_select_larger(from_cwrap<rd_region_type>(self),
                                from_cwrap<rd_kw_type>(kw), limit);
    });
    m.def("_deselect_more", [](py::handle self, py::handle kw, float limit) {
        rd_region_deselect_larger(from_cwrap<rd_region_type>(self),
                                  from_cwrap<rd_kw_type>(kw), limit);
    });
    m.def("_select_in_interval",
          [](py::handle self, py::handle kw, float min_value, float max_value) {
              rd_region_select_in_interval(from_cwrap<rd_region_type>(self),
                                           from_cwrap<rd_kw_type>(kw),
                                           min_value, max_value);
          });
    m.def("_deselect_in_interval",
          [](py::handle self, py::handle kw, float min_value, float max_value) {
              rd_region_deselect_in_interval(from_cwrap<rd_region_type>(self),
                                             from_cwrap<rd_kw_type>(kw),
                                             min_value, max_value);
          });
    m.def("_invert_selection", [](py::handle self) {
        rd_region_invert_selection(from_cwrap<rd_region_type>(self));
    });

    m.def("_select_box",
          [](py::handle self, int i1, int i2, int j1, int j2, int k1, int k2) {
              rd_region_select_from_ijkbox(from_cwrap<rd_region_type>(self), i1,
                                           i2, j1, j2, k1, k2);
          });
    m.def("_deselect_box",
          [](py::handle self, int i1, int i2, int j1, int j2, int k1, int k2) {
              rd_region_deselect_from_ijkbox(from_cwrap<rd_region_type>(self),
                                             i1, i2, j1, j2, k1, k2);
          });
    m.def("_imul_kw", [](py::handle self, py::handle kw, py::handle other,
                         bool force_active) {
        rd_region_kw_imul(from_cwrap<rd_region_type>(self),
                          from_cwrap<rd_kw_type>(kw),
                          from_cwrap<rd_kw_type>(other), force_active);
    });
    m.def("_idiv_kw", [](py::handle self, py::handle kw, py::handle other,
                         bool force_active) {
        rd_region_kw_idiv(from_cwrap<rd_region_type>(self),
                          from_cwrap<rd_kw_type>(kw),
                          from_cwrap<rd_kw_type>(other), force_active);
    });
    m.def("_iadd_kw", [](py::handle self, py::handle kw, py::handle other,
                         bool force_active) {
        rd_region_kw_iadd(from_cwrap<rd_region_type>(self),
                          from_cwrap<rd_kw_type>(kw),
                          from_cwrap<rd_kw_type>(other), force_active);
    });
    m.def("_isub_kw", [](py::handle self, py::handle kw, py::handle other,
                         bool force_active) {
        rd_region_kw_isub(from_cwrap<rd_region_type>(self),
                          from_cwrap<rd_kw_type>(kw),
                          from_cwrap<rd_kw_type>(other), force_active);
    });
    m.def("_copy_kw", [](py::handle self, py::handle kw, py::handle src_kw,
                         bool force_active) {
        rd_region_kw_copy(from_cwrap<rd_region_type>(self),
                          from_cwrap<rd_kw_type>(kw),
                          from_cwrap<rd_kw_type>(src_kw), force_active);
    });
    m.def("_intersect", [](py::handle self, py::handle other) {
        rd_region_intersection(from_cwrap<rd_region_type>(self),
                               from_cwrap<rd_region_type>(other));
    });
    m.def("_combine", [](py::handle self, py::handle other) {
        rd_region_union(from_cwrap<rd_region_type>(self),
                        from_cwrap<rd_region_type>(other));
    });
    m.def("_subtract", [](py::handle self, py::handle other) {
        rd_region_subtract(from_cwrap<rd_region_type>(self),
                           from_cwrap<rd_region_type>(other));
    });
    m.def("_get_kw_index_list",
          [](py::handle self, py::handle kw, bool force_active) {
              return IntVector().attr("createCReference")(
                  reinterpret_cast<std::uintptr_t>(rd_region_get_kw_index_list(
                      from_cwrap<rd_region_type>(self),
                      from_cwrap<rd_kw_type>(kw), force_active)),
                  self);
          });
    m.def("_get_active_list", [](py::handle self) {
        return IntVector().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(
                rd_region_get_active_list(from_cwrap<rd_region_type>(self))),
            self);
    });
    m.def("_get_global_list", [](py::handle self) {
        return IntVector().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(
                rd_region_get_global_list(from_cwrap<rd_region_type>(self))),
            self);
    });
    m.def("_select_cmp_less",
          [](py::handle self, py::handle kw1, py::handle kw2) {
              rd_region_cmp_select_less(from_cwrap<rd_region_type>(self),
                                        from_cwrap<rd_kw_type>(kw1),
                                        from_cwrap<rd_kw_type>(kw2));
          });
    m.def("_select_cmp_more",
          [](py::handle self, py::handle kw1, py::handle kw2) {
              rd_region_cmp_select_more(from_cwrap<rd_region_type>(self),
                                        from_cwrap<rd_kw_type>(kw1),
                                        from_cwrap<rd_kw_type>(kw2));
          });
    m.def("_deselect_cmp_less",
          [](py::handle self, py::handle kw1, py::handle kw2) {
              rd_region_cmp_deselect_less(from_cwrap<rd_region_type>(self),
                                          from_cwrap<rd_kw_type>(kw1),
                                          from_cwrap<rd_kw_type>(kw2));
          });
    m.def("_deselect_cmp_more",
          [](py::handle self, py::handle kw1, py::handle kw2) {
              rd_region_cmp_deselect_more(from_cwrap<rd_region_type>(self),
                                          from_cwrap<rd_kw_type>(kw1),
                                          from_cwrap<rd_kw_type>(kw2));
          });
    m.def("_select_islice", [](py::handle self, int i1, int i2) {
        rd_region_select_i1i2(from_cwrap<rd_region_type>(self), i1, i2);
    });
    m.def("_deselect_islice", [](py::handle self, int i1, int i2) {
        rd_region_deselect_i1i2(from_cwrap<rd_region_type>(self), i1, i2);
    });
    m.def("_select_jslice", [](py::handle self, int j1, int j2) {
        rd_region_select_j1j2(from_cwrap<rd_region_type>(self), j1, j2);
    });
    m.def("_deselect_jslice", [](py::handle self, int j1, int j2) {
        rd_region_deselect_j1j2(from_cwrap<rd_region_type>(self), j1, j2);
    });
    m.def("_select_kslice", [](py::handle self, int k1, int k2) {
        rd_region_select_k1k2(from_cwrap<rd_region_type>(self), k1, k2);
    });
    m.def("_deselect_kslice", [](py::handle self, int k1, int k2) {
        rd_region_deselect_k1k2(from_cwrap<rd_region_type>(self), k1, k2);
    });
    m.def("_select_deep_cells", [](py::handle self, double depth) {
        rd_region_select_deep_cells(from_cwrap<rd_region_type>(self), depth);
    });
    m.def("_deselect_deep_cells", [](py::handle self, double depth) {
        rd_region_deselect_deep_cells(from_cwrap<rd_region_type>(self), depth);
    });
    m.def("_select_shallow_cells", [](py::handle self, double depth) {
        rd_region_select_shallow_cells(from_cwrap<rd_region_type>(self), depth);
    });
    m.def("_deselect_shallow_cells", [](py::handle self, double depth) {
        rd_region_deselect_shallow_cells(from_cwrap<rd_region_type>(self),
                                         depth);
    });
    m.def("_select_small", [](py::handle self, double limit) {
        rd_region_select_small_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_deselect_small", [](py::handle self, double limit) {
        rd_region_deselect_small_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_select_large", [](py::handle self, double limit) {
        rd_region_select_large_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_deselect_large", [](py::handle self, double limit) {
        rd_region_deselect_large_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_select_thin", [](py::handle self, double limit) {
        rd_region_select_thin_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_deselect_thin", [](py::handle self, double limit) {
        rd_region_deselect_thin_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_select_thick", [](py::handle self, double limit) {
        rd_region_select_thick_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_deselect_thick", [](py::handle self, double limit) {
        rd_region_deselect_thick_cells(from_cwrap<rd_region_type>(self), limit);
    });
    m.def("_select_active", [](py::handle self) {
        rd_region_select_active_cells(from_cwrap<rd_region_type>(self));
    });
    m.def("_select_inactive", [](py::handle self) {
        rd_region_select_inactive_cells(from_cwrap<rd_region_type>(self));
    });
    m.def("_deselect_active", [](py::handle self) {
        rd_region_deselect_active_cells(from_cwrap<rd_region_type>(self));
    });
    m.def("_deselect_inactive", [](py::handle self) {
        rd_region_deselect_inactive_cells(from_cwrap<rd_region_type>(self));
    });
    m.def("_select_above_plane",
          [](py::handle self, std::vector<double> n, std::vector<double> p) {
              rd_region_select_above_plane(from_cwrap<rd_region_type>(self),
                                           plane_vector_data(n, "n"),
                                           plane_vector_data(p, "p"));
          });
    m.def("_select_below_plane",
          [](py::handle self, std::vector<double> n, std::vector<double> p) {
              rd_region_select_below_plane(from_cwrap<rd_region_type>(self),
                                           plane_vector_data(n, "n"),
                                           plane_vector_data(p, "p"));
          });
    m.def("_deselect_above_plane",
          [](py::handle self, std::vector<double> n, std::vector<double> p) {
              rd_region_deselect_above_plane(from_cwrap<rd_region_type>(self),
                                             plane_vector_data(n, "n"),
                                             plane_vector_data(p, "p"));
          });
    m.def("_deselect_below_plane",
          [](py::handle self, std::vector<double> n, std::vector<double> p) {
              rd_region_deselect_below_plane(from_cwrap<rd_region_type>(self),
                                             plane_vector_data(n, "n"),
                                             plane_vector_data(p, "p"));
          });
    m.def("_select_inside_polygon", [](py::handle self, py::handle polygon) {
        rd_region_select_inside_polygon(from_cwrap<rd_region_type>(self),
                                        from_cwrap<geo_polygon_type>(polygon));
    });
    m.def("_select_outside_polygon", [](py::handle self, py::handle polygon) {
        rd_region_select_outside_polygon(from_cwrap<rd_region_type>(self),
                                         from_cwrap<geo_polygon_type>(polygon));
    });
    m.def("_deselect_inside_polygon", [](py::handle self, py::handle polygon) {
        rd_region_deselect_inside_polygon(
            from_cwrap<rd_region_type>(self),
            from_cwrap<geo_polygon_type>(polygon));
    });
    m.def("_deselect_outside_polygon", [](py::handle self, py::handle polygon) {
        rd_region_deselect_outside_polygon(
            from_cwrap<rd_region_type>(self),
            from_cwrap<geo_polygon_type>(polygon));
    });
    m.def("_set_name", [](py::handle self, std::optional<std::string> name) {
        rd_region_set_name(from_cwrap<rd_region_type>(self),
                           name ? name->c_str() : nullptr);
    });
    m.def("_get_name", [](py::handle self) -> py::object {
        const char *name = rd_region_get_name(from_cwrap<rd_region_type>(self));
        if (!name)
            return py::none();
        return py::str(name);
    });
    m.def("_contains_ijk", [](py::handle self, int i, int j, int k) {
        return rd_region_contains_ijk(from_cwrap<rd_region_type>(self), i, j,
                                      k);
    });
    m.def("_contains_global", [](py::handle self, int global_index) {
        return rd_region_contains_global(from_cwrap<rd_region_type>(self),
                                         global_index);
    });
    m.def("_contains_active", [](py::handle self, int active_index) {
        return rd_region_contains_active(from_cwrap<rd_region_type>(self),
                                         active_index);
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return rd_region_equal(from_cwrap<rd_region_type>(self),
                               from_cwrap<rd_region_type>(other));
    });
    m.def("_select_true", [](py::handle self, py::handle kw) {
        rd_region_select_true(from_cwrap<rd_region_type>(self),
                              from_cwrap<rd_kw_type>(kw));
    });
    m.def("_select_false", [](py::handle self, py::handle kw) {
        rd_region_select_false(from_cwrap<rd_region_type>(self),
                               from_cwrap<rd_kw_type>(kw));
    });
    m.def("_select_from_layer", [](py::handle self, py::handle layer, int k,
                                   int value) {
        rd_region_select_from_layer(from_cwrap<rd_region_type>(self),
                                    from_cwrap<layer_type>(layer), k, value);
    });
}
} // namespace
