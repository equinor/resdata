#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/fault_block.hpp>
#include <resdata/fault_block_layer.hpp>
#include <resdata/layer.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
/* Block ids and storage indices are size_t internally; validate here at the
   binding edge so that negative python values do not wrap around. */
size_t index_to_size_t(int index, const char *name) {
    if (index < 0)
        throw std::out_of_range(std::string(name) +
                                " must be non-negative, got " +
                                std::to_string(index));
    return static_cast<size_t>(index);
}

/* Attaches a "_parent_layer_ref" attribute to the FaultBlock
   so that FaultBlock.get_parent_layer() can get implemented correctly.
   This causes a cycle from parent->blocks (std::shared_ptr<FaultBlock>)->
   FaultBlock._parent_layer_ref (__dict__ of python object) -> parent.
   However, when __dict__ is GC'd the cycle is broken and there is no
   memory leak.*/
py::object fault_block_reference(std::shared_ptr<FaultBlock> block,
                                 py::handle parent) {
    if (!block)
        return py::none();
    py::object obj = py::cast(block);
    obj.attr("_parent_layer_ref") = parent;
    return obj;
}
} // namespace

PYBIND11_MODULE(_fault_block_layer, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for FaultBlockLayer";

    m.def(
        "_alloc",
        [](py::handle grid, size_t k) {
            return reinterpret_cast<std::uintptr_t>(
                fault_block_layer_alloc(from_cwrap<rd_grid_type>(grid), k));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        fault_block_layer_free(from_cwrap<fault_block_layer_type>(self));
    });
    m.def("_size", [](py::handle self) {
        return fault_block_layer_get_size(
            from_cwrap<fault_block_layer_type>(self));
    });
    m.def("_iget_block", [](py::handle self, int storage_index) {
        return fault_block_reference(
            fault_block_layer_iget_block(
                from_cwrap<fault_block_layer_type>(self),
                index_to_size_t(storage_index, "storage_index")),
            self);
    });
    m.def("_add_block", [](py::handle self, int block_id) {
        return fault_block_reference(
            fault_block_layer_add_block(
                from_cwrap<fault_block_layer_type>(self),
                index_to_size_t(block_id, "block_id")),
            self);
    });
    m.def("_get_block", [](py::handle self, int block_id) {
        return fault_block_reference(
            fault_block_layer_get_block(
                from_cwrap<fault_block_layer_type>(self),
                index_to_size_t(block_id, "block_id")),
            self);
    });
    m.def("_del_block", [](py::handle self, int block_id) {
        fault_block_layer_del_block(from_cwrap<fault_block_layer_type>(self),
                                    index_to_size_t(block_id, "block_id"));
    });
    /* A negative block id can never be present; __contains__ must answer
       false rather than raise. */
    m.def("_has_block", [](py::handle self, int block_id) {
        if (block_id < 0)
            return false;
        return fault_block_layer_has_block(
            from_cwrap<fault_block_layer_type>(self),
            static_cast<size_t>(block_id));
    });
    m.def("_scan_keyword", [](py::handle self, py::handle fault_block_kw) {
        return fault_block_layer_scan_kw(
            from_cwrap<fault_block_layer_type>(self),
            from_cwrap<rd_kw_type>(fault_block_kw));
    });
    m.def("_load_keyword", [](py::handle self, py::handle fault_block_kw) {
        return fault_block_layer_load_kw(
            from_cwrap<fault_block_layer_type>(self),
            from_cwrap<rd_kw_type>(fault_block_kw));
    });
    m.def("_getK", [](py::handle self) {
        return fault_block_layer_get_k(
            from_cwrap<fault_block_layer_type>(self));
    });
    m.def("_get_next_id", [](py::handle self) {
        return fault_block_layer_get_next_id(
            from_cwrap<fault_block_layer_type>(self));
    });
    m.def("_scan_layer", [](py::handle self, py::handle layer) {
        fault_block_layer_scan_layer(from_cwrap<fault_block_layer_type>(self),
                                     from_cwrap<layer_type>(layer));
    });
    m.def("_insert_block_content", [](py::handle self, FaultBlock &block) {
        fault_block_layer_insert_block_content(
            from_cwrap<fault_block_layer_type>(self), block);
    });
    m.def("_export_kw", [](py::handle self, py::handle kw) {
        return fault_block_layer_export(
            from_cwrap<fault_block_layer_type>(self),
            from_cwrap<rd_kw_type>(kw));
    });
    m.def("_get_layer", [](py::handle self) {
        return Layer().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(fault_block_layer_get_layer(
                from_cwrap<fault_block_layer_type>(self))),
            self);
    });
}
