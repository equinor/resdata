#pragma once
#include <optional>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <resdata/rd_kw.hpp>

template <typename T> T *from_cwrap(pybind11::handle obj);
template <typename T> T *from_cwrap(std::optional<pybind11::handle> obj);

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
pybind11::object WellState();
pybind11::object WellTimeLine();
pybind11::object ResdataKW();
pybind11::object ResdataFileView();
pybind11::object IntVector();
pybind11::object CPolyline();
pybind11::object CPolylineCollection();
pybind11::object GeoPointset();
pybind11::object GeoRegion();
pybind11::object Surface();
pybind11::object Layer();
pybind11::object FaultBlock();
pybind11::object FaultBlockLayer();
pybind11::object LookupTable();
pybind11::object RandomNumberGenerator();
pybind11::object ResdataSMSPECNode();
pybind11::object SummaryTStep();
pybind11::object ResdataGrav();
pybind11::object BoolVector();
pybind11::object PermutationVector();
pybind11::object Hash();
