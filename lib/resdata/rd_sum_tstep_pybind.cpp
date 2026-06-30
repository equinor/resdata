#include <cstdint>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_smspec.hpp>
#include <resdata/rd_sum_tstep.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
template <typename T> T *local_cast_cwrap(py::handle obj) {
    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    return reinterpret_cast<T *>(PyLong_AsVoidPtr(address.ptr()));
}

rd_smspec_type *smspec_from_python(py::handle obj) {
    if (obj.is_none())
        throw std::invalid_argument("smspec must not be None");
    if (PyLong_Check(obj.ptr()))
        return reinterpret_cast<rd_smspec_type *>(PyLong_AsVoidPtr(obj.ptr()));
    return local_cast_cwrap<rd_smspec_type>(obj);
}
} // namespace

PYBIND11_MODULE(_rd_sum_tstep, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for SummaryTStep";

    m.def(
        "_alloc",
        [](int report_step, int ministep, float sim_seconds,
           py::handle smspec) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_tstep_alloc_new(
                report_step, ministep, sim_seconds,
                smspec_from_python(smspec)));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        rd_sum_tstep_free(from_cwrap<rd_sum_tstep_type>(self));
    });
    m.def("_get_sim_days", [](py::handle self) {
        return rd_sum_tstep_get_sim_days(from_cwrap<rd_sum_tstep_type>(self));
    });
    m.def("_get_sim_time", [](py::handle self) {
        return static_cast<std::int64_t>(
            rd_sum_tstep_get_sim_time(from_cwrap<rd_sum_tstep_type>(self)));
    });
    m.def("_get_report", [](py::handle self) {
        return rd_sum_tstep_get_report(from_cwrap<rd_sum_tstep_type>(self));
    });
    m.def("_get_ministep", [](py::handle self) {
        return rd_sum_tstep_get_ministep(from_cwrap<rd_sum_tstep_type>(self));
    });
    m.def("_set_from_key",
          [](py::handle self, std::string key, float value) {
              rd_sum_tstep_set_from_key(from_cwrap<rd_sum_tstep_type>(self),
                                        key.c_str(), value);
          });
    m.def("_get_from_key", [](py::handle self, std::string key) {
        return rd_sum_tstep_get_from_key(from_cwrap<rd_sum_tstep_type>(self),
                                         key.c_str());
    });
    m.def("_has_key", [](py::handle self, std::string key) {
        return rd_sum_tstep_has_key(from_cwrap<rd_sum_tstep_type>(self),
                                    key.c_str());
    });
}
