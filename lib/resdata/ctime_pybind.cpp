#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/util.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_ctime, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for CTime";

    m.def("_timezone", []() {
        char *timezone = util_get_timezone();
        return timezone ? std::string(timezone) : std::string();
    });
    m.def("_timegm",
          [](int sec, int min, int hour, int mday, int month, int year) {
              return static_cast<std::int64_t>(
                  util_make_datetime_utc(sec, min, hour, mday, month, year));
          });
}
