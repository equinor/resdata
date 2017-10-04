/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus_assert_null.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdio.h>
#include <string.h>

#include <ert/util/test_work_area.h>
#include <ert/util/test_util.h>

#include <ert/nexus/nexus_plot.h>

void test_file_exist() {
    test_assert_NULL(nexus_plot_alloc("file_does_not_exist"));
}


void test_invalid_header() {
  test_work_area_type * work_area = test_work_area_alloc("nexuse_header");
  FILE * stream = fopen("plot_file" , "w");
  const char * header = "xxxxINVALID_HEADER";
  fwrite( header, 1, strlen(header), stream);
  fclose( stream );

  test_assert_NULL( nexus_plot_alloc("plot_file"));

  test_work_area_free( work_area );
}




int main(int argc, char **argv) {
  test_file_exist();
  test_invalid_header();
  exit(0);
}
