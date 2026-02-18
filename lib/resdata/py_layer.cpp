#include <resdata/py_layer.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_layer, m) {
    py::class_<PyLayer>(m, "Layer")
        .def(py::init<int, int>())
        .def("__getitem__", &PyLayer::get_cell)
        .def("__setitem__", &PyLayer::set_cell)
        .def("active_cell", &PyLayer::active_cell)
        .def("update_active", &PyLayer::update_active)
        .def("bottom_barrier", &PyLayer::bottom_barrier)
        .def("left_barrier", &PyLayer::left_barrier)
        .def("get_nx", &PyLayer::get_nx)
        .def("get_ny", &PyLayer::get_ny)
        .def_property_readonly("nx", &PyLayer::nx)
        .def_property_readonly("ny", &PyLayer::ny)
        .def("cell_contact", &PyLayer::cell_contact)
        .def("add_interp_barrier", &PyLayer::add_interp_barrier)
        .def("add_polyline_barrier", &PyLayer::add_polyline_barrier)
        .def("add_fault_barrier", &PyLayer::add_fault_barrier, py::arg("fault"),
             py::arg("K"), py::arg("link_segments") = true)
        .def("add_ij_barrier", &PyLayer::add_ij_barrier)
        .def("cell_sum", &PyLayer::cell_sum)
        .def("clear_cells", &PyLayer::clear_cells)
        .def("assign", &PyLayer::assign)
        .def("update_connected", &PyLayer::update_connected, py::arg("ij"),
             py::arg("new_value"), py::arg("org_value") = py::none())
        .def("cells_equal", &PyLayer::cells_equal)
        .def("count_equal", &PyLayer::count_equal)
        .def("_address", &PyLayer::address);
}
