#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/rd_version.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

bool rd_version_is_devel_version();

PYBIND11_MODULE(_rd_version, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for ResdataVersion";

    m.def("_build_time", []() {
        const char *build_time = rd_version_get_build_time();
        return build_time ? std::string(build_time) : std::string();
    });
    m.def("_git_commit", []() {
        const char *git_commit = rd_version_get_git_commit();
        return git_commit ? std::string(git_commit) : std::string();
    });
    m.def("_major_version", []() { return rd_version_get_major_version(); });
    m.def("_minor_version", []() { return rd_version_get_minor_version(); });
    m.def("_micro_version", []() {
        const char *micro_version = rd_version_get_micro_version();
        return micro_version ? std::string(micro_version) : std::string();
    });
    m.def("_is_devel", []() { return rd_version_is_devel_version(); });
}
