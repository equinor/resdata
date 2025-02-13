/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'kw_list.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdbool.h>
#include <string.h>

#include <resdata/rd_kw.hpp>
#include <resdata/fortio.h>
#include <resdata/rd_util.hpp>
#include <resdata/rd_endian_flip.hpp>

void kw_list(const char *filename) {
    fortio_type *fortio;
    rd_kw_type *rd_kw = rd_kw_alloc_empty();
    bool fmt_file;
    if (rd_fmt_file(filename, &fmt_file)) {

        printf("---------------------------------------------------------------"
               "--\n");
        printf("%s: \n", filename);
        fortio = fortio_open_reader(filename, fmt_file, RD_ENDIAN_FLIP);
        while (rd_kw_fread_realloc(rd_kw, fortio))
            rd_kw_summarize(rd_kw);
        printf("---------------------------------------------------------------"
               "--\n");

        rd_kw_free(rd_kw);
        fortio_fclose(fortio);
    } else
        fprintf(stderr,
                "Could not determine formatted/unformatted status of:%s - "
                "skipping \n",
                filename);
}

int main(int argc, char **argv) {
    fprintf(stderr,
            "** Warning: kw_list.x deprecated. Use resfo in python instead\n");
    int i;
    for (i = 1; i < argc; i++)
        kw_list(argv[i]);
    return 0;
}
