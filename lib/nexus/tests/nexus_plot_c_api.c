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

void test_spe1_header(const char *argv) {
    char filename[256];
    strcpy(filename, argv);
    strcat(filename, "/test-data/local/nexus/SPE1.plt");

    nexus_plot_type *plt = nexus_plot_alloc( filename );

    // char *class_names[9] = {
    //     "WELL    ", "WLLYR   ", "NODE    ", "CONN    ", "REGION  ", "FIELD   ",
    //     "CONNLIST", "TARGET  ", "FLOSTA  "
    // };
    // int vars_in_class[9] = {
    //     56, 52, 4, 43, 69, 58, 20, 25, 25
    // };

    test_assert_int_equal(nexus_plot_get_num_classes(plt), 9);
    test_assert_int_equal(nexus_plot_get_day(plt),         1);
    test_assert_int_equal(nexus_plot_get_month(plt),       1);
    test_assert_int_equal(nexus_plot_get_year(plt),        1980);
    test_assert_int_equal(nexus_plot_get_nx(plt),          1);
    test_assert_int_equal(nexus_plot_get_ny(plt),          1);
    test_assert_int_equal(nexus_plot_get_nz(plt),          1);
    test_assert_int_equal(nexus_plot_get_ncomp(plt),       2);
    // for (int i = 0; i < plt->num_classes; i++)
    //     test_assert_string_equal(class_names[i], plt->class_names[i]);
    // for (int i = 0; i < plt->num_classes; i++)
    //     test_assert_int_equal(vars_in_class[i], plt->vars_in_class[i]);
}

int main(int argc, char const *argv[]) {
    util_install_signals();
    test_file_exist();
    test_spe1_header(argv[1]);
    exit(0);
}
