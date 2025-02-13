/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'grid_info.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>

int main(int argc, char **argv) {
    fprintf(
        stderr,
        "** Warning: grid_info is deprecated. Use res2df and pandas instead\n");
    if (argc < 2) {
        fprintf(stderr, "%s: filename \n", argv[0]);
        exit(1);
    }

    {
        rd_grid_type *rd_grid;
        const char *grid_file = argv[1];

        rd_grid = rd_grid_alloc(grid_file);
        rd_grid_summarize(rd_grid);
        if (argc >= 3) {
            rd_grid_type *grid2 = rd_grid_alloc(argv[2]);

            if (rd_grid_compare(rd_grid, grid2, true, false, false))
                printf("\nThe grids %s %s are IDENTICAL.\n", argv[1], argv[2]);
            else {
                printf("\n");
                rd_grid_summarize(grid2);
                printf("\nThe grids %s %s are DIFFERENT.\n", argv[1], argv[2]);
            }
            rd_grid_free(grid2);
        }
        rd_grid_free(rd_grid);
    }
}
