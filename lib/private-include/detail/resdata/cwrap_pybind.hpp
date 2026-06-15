#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <resdata/rd_kw.hpp>

template <typename T> T *from_cwrap(pybind11::handle obj);
rd_kw_type *from_cwrap_opt_kw(pybind11::handle obj);
