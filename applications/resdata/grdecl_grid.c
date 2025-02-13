/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'grdecl_grid.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <resdata/rd_kw.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw_magic.hpp>

int main(int argc, char **argv) {
    fprintf(stderr, "** Warning: grdecl_grid is deprecated. Use OPM to parse "
                    "the deck instead\n");
    FILE *stream = util_fopen(argv[1], "r");
    rd_kw_type *gridhead_kw =
        rd_kw_fscanf_alloc_grdecl_dynamic__(stream, SPECGRID_KW, false, RD_INT);
    rd_kw_type *zcorn_kw =
        rd_kw_fscanf_alloc_grdecl_dynamic(stream, ZCORN_KW, RD_FLOAT);
    rd_kw_type *coord_kw =
        rd_kw_fscanf_alloc_grdecl_dynamic(stream, COORD_KW, RD_FLOAT);
    rd_kw_type *actnum_kw =
        rd_kw_fscanf_alloc_grdecl_dynamic(stream, ACTNUM_KW, RD_INT);

    {
        int nx = rd_kw_iget_int(gridhead_kw, SPECGRID_NX_INDEX);
        int ny = rd_kw_iget_int(gridhead_kw, SPECGRID_NY_INDEX);
        int nz = rd_kw_iget_int(gridhead_kw, SPECGRID_NZ_INDEX);
        rd_grid_type *rd_grid = rd_grid_alloc_GRDECL_kw(
            nx, ny, nz, zcorn_kw, coord_kw, actnum_kw, NULL);
        /* .... */
        rd_grid_free(rd_grid);
    }
    rd_kw_free(gridhead_kw);
    rd_kw_free(zcorn_kw);
    rd_kw_free(actnum_kw);
    rd_kw_free(coord_kw);
    fclose(stream);
}
