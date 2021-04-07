/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'ecl_sum_test.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/ecl/ecl_sum.hpp>

int main(int argc, char **argv) {
  const char *case1 = argv[1];
  const char *case2 = argv[2];
  const char *equal_string = argv[3];
  bool equal;
  ecl_sum_type *ecl_sum1 = ecl_sum_fread_alloc_case(case1, ":");
  ecl_sum_type *ecl_sum2 = ecl_sum_fread_alloc_case(case2, ":");

  test_assert_true(ecl_sum_is_instance(ecl_sum1));
  test_assert_true(ecl_sum_is_instance(ecl_sum2));
  test_assert_true(ecl_sum_report_step_equal(ecl_sum1, ecl_sum1));
  test_assert_true(util_sscanf_bool(equal_string, &equal));

  test_assert_true(ecl_sum_report_step_equal(ecl_sum1, ecl_sum1));
  test_assert_true(ecl_sum_report_step_equal(ecl_sum2, ecl_sum2));
  test_assert_bool_equal(equal, ecl_sum_report_step_equal(ecl_sum1, ecl_sum2));

  ecl_sum_free(ecl_sum1);
  ecl_sum_free(ecl_sum2);
  exit(0);
}
