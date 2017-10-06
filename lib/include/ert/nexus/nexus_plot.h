/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'nexus_plot.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef NEXUS_PLOT_H
#define NEXUS_PLOT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/type_macros.h>

typedef struct nexus_plot_struct nexus_plot_type;

nexus_plot_type *nexus_plot_alloc(char *file);

void nexus_plot_free(nexus_plot_type *nexus_plot);

UTIL_IS_INSTANCE_HEADER(nexus_plot);

UTIL_SAFE_CAST_HEADER(nexus_plot);

#ifdef __cplusplus
}
#endif
#endif
