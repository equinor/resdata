/*
   Copyright (C) 2018  Statoil ASA, Norway.

   This file is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ECL_GRID_HPP
#define ECL_GRID_HPP

#include <ert/ecl/ecl_grid.h>

namespace ecl {

ecl_grid_type * ecl_grid_alloc_GRDECL_data(int nx,
                                           int ny,
                                           int nz,
                                           const double * zcorn,
                                           const double * coord,
                                           const int * actnum,
                                           bool apply_mapaxes,
                                           const float * mapaxes);

}
#endif

