/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus_plot.hpp' is part of ERT - Ensemble based Reservoir Tool.

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


#include <ert/util/util.h>
#include <ert/util/type_macros.h>

#include <ert/ecl/ecl_sum.h>


typedef struct nexus_plot_struct nexus_plot_type;

nexus_plot_type* nexus_plot_alloc( char *filename );
void nexus_plot_free( nexus_plot_type* );

int nexus_plot_get_num_classes( nexus_plot_type* );
int nexus_plot_get_day( nexus_plot_type* );
int nexus_plot_get_month( nexus_plot_type* );
int nexus_plot_get_year( nexus_plot_type* );
int nexus_plot_get_nx( nexus_plot_type* );
int nexus_plot_get_ny( nexus_plot_type* );
int nexus_plot_get_nz( nexus_plot_type* );
int nexus_plot_get_ncomp( nexus_plot_type* );

ecl_sum_type *nexus_plot_alloc_ecl_sum(  const nexus_plot_type*, const char* );

UTIL_IS_INSTANCE_HEADER(nexus_plot);
UTIL_SAFE_CAST_HEADER(nexus_plot);


#ifdef __cplusplus
}
#endif
#endif // NEXUS_PLOT_H
