/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'nexus_plot.c' is part of ERT - Ensemble based Reservoir Tool.

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


#include <stdlib.h>
#include <stdio.h>

#include <ert/util/util.h>
#include <ert/util/type_macros.h>
#include <ert/nexus/nexus_plot.h>

#define NEXUS_PLOT_HEADER "PLOT  BIN "

#define NEXUS_PLOT_ID  884298

struct nexus_plot_struct {
    UTIL_TYPE_ID_DECLARATION;
};

UTIL_IS_INSTANCE_FUNCTION(nexus_plot, NEXUS_PLOT_ID);

UTIL_SAFE_CAST_FUNCTION(nexus_plot, NEXUS_PLOT_ID);


nexus_plot_type *nexus_plot_alloc(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        return NULL;

    char content[11];
    fseek(fp, 4, SEEK_SET);
    fread(content, 1, 10, fp);
    content[10] = '\0';

    if (!util_string_equal(content, NEXUS_PLOT_HEADER)) {
        fclose(fp);
        return NULL;
    } else {
        nexus_plot_type *plt = util_malloc(sizeof *plt);
        UTIL_TYPE_ID_INIT(plt, NEXUS_PLOT_ID);
        return plt;
    }
}


void nexus_plot_free(nexus_plot_type *nexus_plot) {
    free(nexus_plot);
}