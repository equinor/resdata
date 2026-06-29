#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <resdata/rd_kw.hpp>

template <typename T> T *from_cwrap(pybind11::handle obj);
rd_kw_type *from_cwrap_opt_kw(pybind11::handle obj);

/// Sets up custom exception translators
inline void register_exceptions(pybind11::module &m) {
    pybind11::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p)
                std::rethrow_exception(p);
        } catch (const std::ios_base::failure &e) {
            PyErr_SetString(PyExc_OSError, e.what());
        }
    });
}

pybind11::object WellConnection();
pybind11::object WellSegment();
