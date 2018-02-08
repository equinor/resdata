/*
   Copyright (C) 2018  Statoil ASA, Norway.

   The file 'ecl_util_filenames.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdbool.h>

#include <ert/util/test_util.h>
#include <ert/util/time_t_vector.h>
#include <ert/util/util.h>

#include <ert/ecl/ecl_util.h>


void test_filename_report_nr() {
  test_assert_int_equal(78, ecl_util_filename_report_nr("Path/with/mixedCASE/case.x0078"));
  test_assert_int_equal(78, ecl_util_filename_report_nr("Case.X0078"));
  test_assert_int_equal(ECL_EGRID_FILE, ecl_util_get_file_type("path/WITH/xase/MyGrid.EGrid", NULL, NULL));

}

int main(int argc , char ** argv) {
  test_filename_report_nr();
}
