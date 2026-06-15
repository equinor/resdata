#pragma once
#include <pybind11/pybind11.h>

template <typename T> T *from_cwrap(pybind11::handle obj) {
    if (obj.is_none())
        return nullptr;

    pybind11::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());

    return reinterpret_cast<T *>(pointer);
}
