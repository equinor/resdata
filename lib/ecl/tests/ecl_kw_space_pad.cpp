/*
  Copyright (C) 2019  Equinor ASA, Norway.

  The file 'ecl_grid_unit_system.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

#include <ert/ecl/ecl_kw.hpp>



int main(int argc, char **argv) {
  ecl::util::TestArea ta("grid_unit_system");

  // 1. Write a ecl_kw instance with string data - uninitialized.
  {
    ecl_kw_type * ecl_kw = ecl_kw_alloc("SPACE", 1, ECL_CHAR);
    fortio_type * f = fortio_open_writer("file", false, true);
    ecl_kw_fwrite(ecl_kw, f);
    fortio_fclose(f);
    ecl_kw_free( ecl_kw );
  }

  // 2. Open file with normal fopen() and verify that the data section consists of only spaces.
  {
    FILE * stream = util_fopen("file", "r");
    char buffer[8];
    size_t offset = 4 + 16 + 4 + 4;
    fseek(stream, offset, SEEK_SET);
    fread(buffer, 1, 8, stream);
    for (int i=0; i < 8; i++)
      test_assert_int_equal(buffer[i], ' ');

    fclose(stream);
  }
}
