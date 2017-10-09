/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus2ecl.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/ecl/ecl_sum.h>

#include <ert/nexus/nexus_plot.h>


void test_create_ecl_sum() {
  test_work_area_type *work_area = test_work_area_alloc("nexus_header");
  FILE *stream = fopen("valid_file", "w");
  const char *header = "xxxxPLOT  BIN ";
  fwrite(header, 1, strlen(header), stream);
  fclose(stream);

  nexus_plot_type *plt = nexus_plot_alloc("valid_file");
  test_assert_true(nexus_plot_is_instance(plt));

  ecl_sum_type * ecl_sum = nexus_plot_alloc_ecl_sum( plt , "ECL_CASE");
  test_assert_true( ecl_sum_is_instance( ecl_sum ));
  ecl_sum_fwrite( ecl_sum );
  ecl_sum_free( ecl_sum );
  test_assert_true( util_file_exists( "ECL_CASE.SMSPEC"));
  nexus_plot_free(plt);
  test_work_area_free(work_area);
}



int main(int argc, char **argv) {
    util_install_signals();
    test_create_ecl_sum();
    exit(0);
}
