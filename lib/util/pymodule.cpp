/*
   Copyright (C) 2019  Equinor ASA, Norway.

   The file 'pymodule.cpp' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#include <Python.h>
#include <cstdarg>
#include <iostream>
#include <dlfcn.h>

struct jmp_buf;

namespace {
  PyObject *exc = nullptr;

PyMethodDef functions[] = {
    {nullptr, nullptr, 0, nullptr}};

struct PyModuleDef module = {PyModuleDef_HEAD_INIT,
                                "libecl",
                                nullptr,
                                -1,
                                functions,
                                nullptr,
                                nullptr,
                                nullptr,
                                nullptr};

} // namespace
/*
  These are non-functional stubs.
*/

extern "C" bool util_addr2line_lookup(const void *bt_addr, char **func_name,
                           char **file_name, int *line_nr) {
  return false;
}

extern "C" jmp_buf *util_abort_test_jump_buffer() { return NULL; }

extern "C" void util_abort_test_set_intercept_function(const char *function) { return; }

extern "C" void util_abort__(const char *file, const char *func, size_t line, const char *fmt, ...)
{
  char error[512];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(error, 511, fmt, ap);
  va_end(ap);

  PyErr_SetString(exc, error);
}


PyMODINIT_FUNC PyInit_libecl() {
  auto mod = PyModule_Create(&module);

  auto dict = PyDict_New();
  exc = PyErr_NewExceptionWithDoc("libecl.UtilAbort", "util_abort", PyExc_RuntimeError, dict);
  PyModule_AddObject(mod, "UtilAbort", exc);

  return mod;
}
