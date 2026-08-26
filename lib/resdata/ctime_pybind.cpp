#include <ctime>
#include <string>

#include <pybind11/pybind11.h>

#include <ert/util/util.hpp>

namespace py = pybind11;

namespace {

PYBIND11_MODULE(_ctime, m) {
    m.doc() = "pybind11 bindings for the timezone/date helpers used by "
              "resdata.util.util.ctime";

    m.def("_timezone", []() -> std::string { return util_get_timezone(); });
    m.def("_timegm",
          [](int sec, int min, int hour, int mday, int month,
             int year) -> std::time_t {
              return util_make_datetime_utc(sec, min, hour, mday, month, year);
          });
}
} // namespace
