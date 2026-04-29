#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fmt/format.h>
#include <pybind11/numpy.h>
#include <cstdint>

#include <resdata/rd_grid.hpp>

namespace py = pybind11;
using fmt::format;

namespace {
template <typename T> T *from_cwrap(py::handle obj) {
    if (obj.is_none())
        return nullptr;

    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());

    return reinterpret_cast<T *>(pointer);
}

PYBIND11_MODULE(_grid, m) {
    m.doc() = "pybind11 bindings between rd_grid.py and rd_grid.cpp";

    m.def(
        "_fread_alloc",
        [](std::string filename, bool apply_mapaxes) {
            return reinterpret_cast<std::uintptr_t>(
                rd_grid_load_case__(filename.c_str(), apply_mapaxes));
        },
        py::return_value_policy::reference);

    m.def(
        "_grdecl_create",
        [](int nx, int ny, int nz, py::handle zcorn, py::handle coord,
           py::handle actnum, py::handle mapaxes) {
            return reinterpret_cast<std::uintptr_t>(rd_grid_alloc_GRDECL_kw(
                nx, ny, nz, from_cwrap<rd_kw_type>(zcorn),
                from_cwrap<rd_kw_type>(coord), from_cwrap<rd_kw_type>(actnum),
                from_cwrap<rd_kw_type>(mapaxes)));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_rectangular",
        [](int nx, int ny, int nz, double dx, double dy, double dz,
           std::optional<std::vector<int>> actnum) {
            if (actnum.has_value())
                return reinterpret_cast<std::uintptr_t>(
                    rd_grid_alloc_rectangular(nx, ny, nz, dx, dy, dz,
                                              actnum->data()));
            else
                return reinterpret_cast<std::uintptr_t>(
                    rd_grid_alloc_rectangular(nx, ny, nz, dx, dy, dz, nullptr));
        },
        py::return_value_policy::reference);
    m.def(
        "_get_numbered_lgr",
        [](py::handle self, int lgr_nr) {
            return reinterpret_cast<std::uintptr_t>(rd_grid_get_lgr_from_lgr_nr(
                from_cwrap<rd_grid_type>(self), lgr_nr));
        },
        py::return_value_policy::reference);
    m.def(
        "_get_named_lgr",
        [](py::handle self, std::string lgr_name) {
            return reinterpret_cast<std::uintptr_t>(rd_grid_get_lgr(
                from_cwrap<rd_grid_type>(self), lgr_name.c_str()));
        },
        py::return_value_policy::reference);
    m.def(
        "_get_cell_lgr",
        [](py::handle self, int index) {
            return reinterpret_cast<std::uintptr_t>(
                rd_grid_get_cell_lgr1(from_cwrap<rd_grid_type>(self), index));
        },
        py::return_value_policy::reference);
    m.def("_num_coarse_groups", [](py::handle self) {
        return rd_grid_get_num_coarse_groups(from_cwrap<rd_grid_type>(self));
    });
    m.def("_in_coarse_group1", [](py::handle self, int index) {
        return rd_grid_cell_in_coarse_group1(from_cwrap<rd_grid_type>(self),
                                             index);
    });
    m.def("_free", [](py::handle self) {
        rd_grid_free(from_cwrap<rd_grid_type>(self));
    });
    m.def("_get_nx", [](py::handle self) {
        return rd_grid_get_nx(from_cwrap<rd_grid_type>(self));
    });
    m.def("_get_ny", [](py::handle self) {
        return rd_grid_get_ny(from_cwrap<rd_grid_type>(self));
    });
    m.def("_get_nz", [](py::handle self) {
        return rd_grid_get_nz(from_cwrap<rd_grid_type>(self));
    });
    m.def("_get_global_size", [](py::handle self) {
        return rd_grid_get_global_size(from_cwrap<rd_grid_type>(self));
    });
    m.def("_get_active", [](py::handle self) {
        return rd_grid_get_active_size(from_cwrap<rd_grid_type>(self));
    });
    m.def("_get_active_fracture", [](py::handle self) {
        return rd_grid_get_nactive_fracture(from_cwrap<rd_grid_type>(self));
    });
    m.def("_get_name", [](py::handle self) {
        return rd_grid_get_name(from_cwrap<rd_grid_type>(self));
    });
    m.def("_ijk_valid", [](py::handle self, int i, int j, int k) {
        return rd_grid_ijk_valid(from_cwrap<rd_grid_type>(self), i, j, k);
    });
    m.def("_get_global_index3", [](py::handle self, int i, int j, int k) {
        return rd_grid_get_global_index3(from_cwrap<rd_grid_type>(self), i, j,
                                         k);
    });
    m.def("_get_active_index1", [](py::handle self, int index) {
        return rd_grid_get_active_index1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_get_active_fracture_index1", [](py::handle self, int index) {
        return rd_grid_get_active_fracture_index1(
            from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_get_global_index1A", [](py::handle self, int index) {
        return rd_grid_get_global_index1A(from_cwrap<rd_grid_type>(self),
                                          index);
    });
    m.def("_get_global_index1F", [](py::handle self, int index) {
        return rd_grid_get_global_index1F(from_cwrap<rd_grid_type>(self),
                                          index);
    });
    m.def("_get_ijk1", [](py::handle self, int index) {
        int i = 0;
        int j = 0;
        int k = 0;
        rd_grid_get_ijk1(from_cwrap<rd_grid_type>(self), index, &i, &j, &k);
        return std::make_tuple(i, j, k);
    });
    m.def("_get_xyz1", [](py::handle self, int index) {
        double x = 0;
        double y = 0;
        double z = 0;
        rd_grid_get_xyz1(from_cwrap<rd_grid_type>(self), index, &x, &y, &z);
        return std::make_tuple(x, y, z);
    });
    m.def("_get_cell_corner_xyz1",
          [](py::handle self, int index, int corner_nr) {
              double x = 0;
              double y = 0;
              double z = 0;
              rd_grid_get_cell_corner_xyz1(from_cwrap<rd_grid_type>(self),
                                           index, corner_nr, &x, &y, &z);
              return std::make_tuple(x, y, z);
          });
    m.def("_get_corner_xyz", [](py::handle self, int i, int j, int k) {
        double x = 0;
        double y = 0;
        double z = 0;
        rd_grid_get_corner_xyz(from_cwrap<rd_grid_type>(self), i, j, k, &x, &y,
                               &z);
        return std::make_tuple(x, y, z);
    });
    m.def("_get_ij_xy", [](py::handle self, double x, double y, int k) {
        int i = 0;
        int j = 0;
        bool ok = rd_grid_get_ij_from_xy(from_cwrap<rd_grid_type>(self), x, y,
                                         k, &i, &j);
        if (!ok)
            throw std::domain_error(fmt::format(
                "Could not find the point:({:g},{:g}) in layer:{:d}", x, y, k));
        return std::make_tuple(i, j);
    });
    m.def("_get_ijk_xyz",
          [](py::handle self, double x, double y, double z, int start_index) {
              return rd_grid_get_global_index_from_xyz(
                  from_cwrap<rd_grid_type>(self), x, y, z, start_index);
          });
    m.def("_cell_contains",
          [](py::handle self, int index, double x, double y, double z) {
              return rd_grid_cell_contains_xyz1(from_cwrap<rd_grid_type>(self),
                                                index, x, y, z);
          });
    m.def("_cell_regular", [](py::handle self, int index) {
        return rd_grid_cell_regular1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_num_lgr", [](py::handle self) {
        return rd_grid_get_num_lgr(from_cwrap<rd_grid_type>(self));
    });
    m.def("_has_numbered_lgr", [](py::handle self, int lgr_nr) {
        return rd_grid_has_lgr_nr(from_cwrap<rd_grid_type>(self), lgr_nr);
    });
    m.def("_has_named_lgr", [](py::handle self, std::string lgr_name) {
        return rd_grid_has_lgr(from_cwrap<rd_grid_type>(self),
                               lgr_name.c_str());
    });
    m.def("_grid_value",
          [](py::handle self, py::handle kw, int i, int j, int k) {
              return rd_grid_get_property(from_cwrap<rd_grid_type>(self),
                                          from_cwrap<rd_kw_type>(kw), i, j, k);
          });
    m.def("_get_cell_volume", [](py::handle self, int index) {
        return rd_grid_get_cell_volume1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_get_cell_thickness", [](py::handle self, int index) {
        return rd_grid_get_cell_thickness1(from_cwrap<rd_grid_type>(self),
                                           index);
    });
    m.def("_get_cell_dx", [](py::handle self, int index) {
        return rd_grid_get_cell_dx1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_get_cell_dy", [](py::handle self, int index) {
        return rd_grid_get_cell_dy1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_get_depth", [](py::handle self, int index) {
        return rd_grid_get_cdepth1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_fwrite_grdecl", [](py::handle self, py::handle kw,
                               std::optional<std::string> header,
                               py::handle file, double default_value) {
        if (header.has_value())
            rd_grid_grdecl_fprintf_kw(
                from_cwrap<rd_grid_type>(self), from_cwrap<rd_kw_type>(kw),
                header->c_str(), from_cwrap<FILE>(file), default_value);
        else
            rd_grid_grdecl_fprintf_kw(from_cwrap<rd_grid_type>(self),
                                      from_cwrap<rd_kw_type>(kw), nullptr,
                                      from_cwrap<FILE>(file), default_value);
    });
    m.def("_load_column",
          [](py::handle self, py::handle kw, int i, int j, py::handle column) {
              return rd_grid_get_column_property(
                  from_cwrap<rd_grid_type>(self), from_cwrap<rd_kw_type>(kw), i,
                  j, from_cwrap<double_vector_type>(column));
          });
    m.def("_get_top", [](py::handle self, int i, int j) {
        return rd_grid_get_top2(from_cwrap<rd_grid_type>(self), i, j);
    });
    m.def("_get_top1A", [](py::handle self, int index) {
        return rd_grid_get_top1A(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_get_bottom", [](py::handle self, int i, int j) {
        return rd_grid_get_bottom2(from_cwrap<rd_grid_type>(self), i, j);
    });
    m.def("_locate_depth", [](py::handle self, double depth, int i, int j) {
        return rd_grid_locate_depth(from_cwrap<rd_grid_type>(self), depth, i,
                                    j);
    });
    m.def("_invalid_cell", [](py::handle self, int index) {
        return rd_grid_cell_invalid1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_valid_cell", [](py::handle self, int index) {
        return rd_grid_cell_valid1(from_cwrap<rd_grid_type>(self), index);
    });
    m.def("_get_distance", [](py::handle self, int index1, int index2) {
        double dx = 0;
        double dy = 0;
        double dz = 0;
        rd_grid_get_distance(from_cwrap<rd_grid_type>(self), index1, index2,
                             &dx, &dy, &dz);
        return std::make_tuple(dx, dy, dz);
    });
    m.def("_fprintf_grdecl2",
          [](py::handle self, py::handle file, int rd_unit) {
              rd_grid_fprintf_grdecl2(from_cwrap<rd_grid_type>(self),
                                      from_cwrap<FILE>(file),
                                      static_cast<ert_rd_unit_enum>(rd_unit));
          });
    m.def("_fwrite_GRID2", [](py::handle self, std::string filename,
                              int rd_unit) {
        rd_grid_fwrite_GRID2(from_cwrap<rd_grid_type>(self), filename.c_str(),
                             static_cast<ert_rd_unit_enum>(rd_unit));
    });
    m.def("_fwrite_EGRID2", [](py::handle self, std::string filename,
                               int rd_unit) {
        rd_grid_fwrite_EGRID2(from_cwrap<rd_grid_type>(self), filename.c_str(),
                              static_cast<ert_rd_unit_enum>(rd_unit));
    });
    m.def("_equal", [](py::handle self, py::handle other, bool include_lgr,
                       bool include_nnc, bool verbose) {
        return rd_grid_compare(from_cwrap<rd_grid_type>(self),
                               from_cwrap<rd_grid_type>(other), include_lgr,
                               include_nnc, verbose);
    });
    m.def("_dual_grid", [](py::handle self) {
        return rd_grid_dual_grid(from_cwrap<rd_grid_type>(self));
    });
    m.def(
        "_init_actnum",
        [](py::handle self) {
            auto rd_grid = from_cwrap<rd_grid_type>(self);
            int size = rd_grid_get_global_size(rd_grid);
            int_vector_type *actnum = int_vector_alloc(size, 0);
            rd_grid_init_actnum_data(rd_grid, int_vector_get_ptr(actnum));

            return reinterpret_cast<std::uintptr_t>(actnum);
        },
        py::return_value_policy::reference);
    m.def(
        "_init_actnum_kw",
        [](py::handle self) {
            auto rd_grid = from_cwrap<rd_grid_type>(self);
            int size = rd_grid_get_global_size(rd_grid);
            rd_kw_type *actnum = rd_kw_alloc("ACTNUM", size, RD_INT);
            if (!actnum)
                throw std::runtime_error(
                    fmt::format("Could not allocate ACTNUM of size {}", size));
            rd_grid_init_actnum_data(rd_grid, rd_kw_get_int_ptr(actnum));

            return reinterpret_cast<std::uintptr_t>(actnum);
        },
        py::return_value_policy::reference);
    m.def("_compressed_kw_copy",
          [](py::handle self, py::handle kw_copy, py::handle kw) {
              rd_grid_compressed_kw_copy(from_cwrap<rd_grid_type>(self),
                                         from_cwrap<rd_kw_type>(kw_copy),
                                         from_cwrap<rd_kw_type>(kw));
          });
    m.def("_global_kw_copy",
          [](py::handle self, py::handle kw_copy, py::handle kw) {
              rd_grid_global_kw_copy(from_cwrap<rd_grid_type>(self),
                                     from_cwrap<rd_kw_type>(kw_copy),
                                     from_cwrap<rd_kw_type>(kw));
          });
    m.def(
        "_create_volume_keyword",
        [](py::handle self, bool active_size) {
            return reinterpret_cast<std::uintptr_t>(rd_grid_alloc_volume_kw(
                from_cwrap<rd_grid_type>(self), active_size));
        },
        py::return_value_policy::reference);
    m.def("_use_mapaxes", [](py::handle self) {
        return rd_grid_use_mapaxes(from_cwrap<rd_grid_type>(self));
    });
    m.def(
        "_export_coord",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_grid_alloc_coord_kw(from_cwrap<rd_grid_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_export_zcorn",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_grid_alloc_zcorn_kw(from_cwrap<rd_grid_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_export_actnum",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_grid_alloc_actnum_kw(from_cwrap<rd_grid_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_export_mapaxes",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_grid_alloc_mapaxes_kw(from_cwrap<rd_grid_type>(self)));
        },
        py::return_value_policy::reference);
    m.def("_get_unit_system", [](py::handle self) {
        return static_cast<int>(
            rd_grid_get_unit_system(from_cwrap<rd_grid_type>(self)));
    });
    m.def("_export_index_frame", [](py::handle self, bool active_only) {
        auto rd_grid = from_cwrap<rd_grid_type>(self);
        ptrdiff_t size = rd_grid_get_global_size(rd_grid);
        if (active_only)
            size = rd_grid_get_active_size(rd_grid);

        py::array_t<int> indx(size);
        py::array_t<int> data(std::vector<ptrdiff_t>{size, 4});
        rd_grid_export_index(rd_grid, indx.mutable_data(), data.mutable_data(),
                             active_only);
        return std::make_tuple(indx, data);
    });
    m.def("_export_data_as_int", [](ptrdiff_t size, py::array_t<int32_t> indx,
                                    py::handle kw, py::array_t<int32_t> data) {
        rd_grid_export_data_as_int(size, indx.mutable_data(),
                                   from_cwrap<rd_kw_type>(kw),
                                   data.mutable_data());
    });
    m.def("_export_data_as_double",
          [](ptrdiff_t size, py::array_t<int32_t> indx, py::handle kw,
             py::array_t<double> data) {
              rd_grid_export_data_as_double(size, indx.mutable_data(),
                                            from_cwrap<rd_kw_type>(kw),
                                            data.mutable_data());
          });
    m.def("_export_volume", [](py::handle self, py::array_t<int32_t> index) {
        auto rd_grid = from_cwrap<rd_grid_type>(self);

        py::array_t<double> data(index.size());
        auto data_ptr = data.mutable_data();
        std::fill(data_ptr, data_ptr + data.size(), 0.0);
        rd_grid_export_volume(rd_grid, index.size(), index.mutable_data(),
                              data_ptr);
        return data;
    });
    m.def("_export_position", [](py::handle self, py::array_t<int32_t> index) {
        auto rd_grid = from_cwrap<rd_grid_type>(self);

        std::vector<ptrdiff_t> shape = {index.size(), 3};
        py::array_t<double> data(shape);
        auto data_ptr = data.mutable_data();
        std::fill(data_ptr, data_ptr + data.size(), 0.0);
        rd_grid_export_position(rd_grid, index.size(), index.mutable_data(),
                                data_ptr);
        return data;
    });
    m.def("_export_corners", [](py::handle self, py::array_t<int32_t> index) {
        auto rd_grid = from_cwrap<rd_grid_type>(self);

        std::vector<ptrdiff_t> shape = {index.size(), 24};
        py::array_t<double> data(shape);
        auto data_ptr = data.mutable_data();
        std::fill(data_ptr, data_ptr + data.size(), 0.0);
        export_corners(rd_grid, index.size(), index.mutable_data(), data_ptr);
        return data;
    });
}
} // namespace
