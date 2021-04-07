/*
   Copyright (C) 2014  Equinor ASA, Norway.

   The file 'ecl_grid_copy.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <stdbool.h>
#include <stdlib.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.h>

#include <ert/ecl/ecl_grid.hpp>

void test_copy_grid(const ecl_grid_type *grid) {
  ecl_grid_type *grid_copy = ecl_grid_alloc_copy(grid);
  test_assert_true(ecl_grid_compare(grid, grid_copy, true, true, true));
  ecl_grid_free(grid_copy);
}

void test_copy_grid_file(const char *filename) {
  ecl_grid_type *src_grid = ecl_grid_alloc(filename);
  ecl_grid_type *copy_grid = ecl_grid_alloc_copy(src_grid);
  test_assert_true(ecl_grid_compare(src_grid, copy_grid, true, true, true));
  ecl_grid_free(copy_grid);
  ecl_grid_free(src_grid);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    ecl_grid_type *grid = ecl_grid_alloc_rectangular(10, 11, 12, 1, 2, 3, NULL);
    test_copy_grid(grid);
    ecl_grid_free(grid);
  } else {
    test_copy_grid_file(argv[1]);
  }
  exit(0);
}
