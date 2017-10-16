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

#include <cstdlib>
#include <cinttypes>
#include <iostream>
#include <stdexcept>

#include <ert/util/util.h>
#include <ert/util/type_macros.h>
#include <ert/nexus/nexus_plot.h>
#include <ert/nexus/nexus_plot.hpp>

#define NEXUS_PLOT_ID  884298

struct nexus_plot_struct {
    UTIL_TYPE_ID_DECLARATION;
    nex::NexusPlot* data;
};

UTIL_IS_INSTANCE_FUNCTION(nexus_plot, NEXUS_PLOT_ID);
UTIL_SAFE_CAST_FUNCTION(nexus_plot, NEXUS_PLOT_ID);

nexus_plot_type* nexus_plot_alloc( char *filename ) {
    try {
        auto nexus_plt = new nex::NexusPlot(filename);
        nexus_plot_type *plt = (nexus_plot_type*) malloc( sizeof *plt );
        UTIL_TYPE_ID_INIT( plt, NEXUS_PLOT_ID );
        plt->data = nexus_plt;
        return plt;
    }
    catch ( std::runtime_error& e ) {
        return NULL;
    }
}
void nexus_plot_free( nexus_plot_type *plt ) {
    delete plt->data;
    delete plt;
}

int nexus_plot_get_num_classes( nexus_plot_type *plt ) {
    return int(plt->data->num_classes);
}

int nexus_plot_get_day( nexus_plot_type *plt ) {
    return int(plt->data->day);
}

int nexus_plot_get_month( nexus_plot_type *plt ) {
    return int(plt->data->month);
}

int nexus_plot_get_year( nexus_plot_type *plt ) {
    return int(plt->data->year);
}

int nexus_plot_get_nx( nexus_plot_type *plt ) {
    return int(plt->data->nx);
}

int nexus_plot_get_ny( nexus_plot_type *plt ) {
    return int(plt->data->ny);
}

int nexus_plot_get_nz( nexus_plot_type *plt ) {
    return int(plt->data->nz);
}

int nexus_plot_get_ncomp( nexus_plot_type *plt ) {
    return int(plt->data->ncomp);
}

ecl_sum_type *nexus_plot_alloc_ecl_sum( const nexus_plot_type *plt,
        const char* ecl_case ) {
    return plt->data->ecl_summary(ecl_case);
}
