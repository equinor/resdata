#include <pybind11/pybind11.h>

#include "ert/ecl/ecl_kw.hpp"
#include "ert/ecl/ecl_type.hpp"

using namespace std;
namespace py = pybind11;

const py::return_value_policy ref_internal = py::return_value_policy::reference_internal;
const py::return_value_policy python_owner = py::return_value_policy::take_ownership;
const py::return_value_policy move         = py::return_value_policy::move;


class EclKW {

  ecl_kw_type * kw;

  public:
    EclKW(const string& name, int size) {
      kw = ecl_kw_alloc(name.c_str(), size, ECL_INT);
    }
    ~EclKW() {
      ecl_kw_free(kw);
    }
    string GetHeader() {
      return string(ecl_kw_get_header8(kw));
    }
};


PYBIND11_MODULE(ecl_pb11, m) {

    py::class_<EclKW>(m, "EclKW")
        .def(py::init<const string&, int>())
        .def("GetHeader", &EclKW::GetHeader);
}



