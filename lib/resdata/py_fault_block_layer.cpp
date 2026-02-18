#include <resdata/py_layer.hpp>
#include <resdata/fault_block_layer.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_fault_block_layer, m) {
    m.def("fault_block_get_layer", [](const py::object &self){
      auto address = py::cast<std::uintptr_t>(self.attr("_address")());
      auto fb_layer = reinterpret_cast<fault_block_layer_type *>(address);
      return PyLayer(fault_block_layer_get_layer(fb_layer));
    });
}
