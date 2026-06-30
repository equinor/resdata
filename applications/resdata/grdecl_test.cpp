/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'grdecl_test.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <cstdio>
#include <memory>

#include <ert/util/util.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_grdecl.hpp>

int main(int argc, char **argv) {
    fprintf(stderr,
            "** Warning: grdecl_test.x is deprecated. Use res2df instead\n");
    std::unique_ptr<FILE, decltype(&fclose)> stream(util_fopen(argv[1], "r"),
                                                    &fclose);
    {
        while (true) {
            clock_t begin = clock();
            rd_kw_ptr grdecl_kw(
                rd_kw_fscanf_alloc_grdecl(stream.get(), nullptr, RD_FLOAT),
                &rd_kw_free);
            clock_t end = clock();
            double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

            if (grdecl_kw) {
                printf("Loaded %s - %d elements : %g \n",
                       rd_kw_get_header(grdecl_kw.get()),
                       rd_kw_get_size(grdecl_kw.get()), time_spent);
            } else
                break;
        }
    }
}
