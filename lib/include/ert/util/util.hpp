/*
   Copyright (C) 2018 Statoil ASA, Norway.

   This is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or1
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#ifndef UTIL_CXX
#define UTIL_CXX

#include <ert/util/util.h>

#include <vector>

namespace Ecl {

template<typename T>
T * alloc_vector_content(const std::vector<T>& src) {
  return (T*) util_alloc_copy(src.data(), src.size() * sizeof(T));
}

}

#endif
