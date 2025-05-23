/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'select_test.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <ert/util/stringlist.hpp>

#include <resdata/rd_kw.hpp>

int main(int argc, char **argv) {
    fprintf(stderr,
            "** Warning: select_test.x is deprecated. Use res2df instead\n");
    const char *path = argv[1];
    const char *base = argv[2];
    stringlist_type *names = stringlist_alloc_new();

    rd_select_filelist(path, base, RD_OTHER_FILE, false, names);
    {
        int i;
        for (i = 0; i < stringlist_get_size(names); i++)
            printf("list[%02d] = %s \n", i, stringlist_iget(names, i));
    }
}
