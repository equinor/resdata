/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus_plot_constructor.cpp' is part of ERT - Ensemble based Reservoir Tool.

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

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>


#include <ert/util/test_util.hpp>
#include <ert/nexus/nexus_plot.hpp>

void test_invalid_header1() {
    std::stringstream stream( "xxxxINVALID_HEADER" );
    test_assert_throw(nex::NexusPlot { stream }, nex::bad_header);
}

void test_invalid_header2() {
    std::stringstream stream( "xxx" );
    test_assert_throw(nex::NexusPlot { stream }, nex::bad_header);
}

void test_valid_header() {
    std::stringstream stream( "xxxxPLOT  BIN " );
    test_assert_throw(nex::NexusPlot { stream }, nex::unexpected_eof);
}

void test_spe1_header(char *argv) {
    std::stringstream ss;
    ss << argv << "/test-data/local/nexus/SPE1.plt";

    std::array< std::string, 9 > class_names = {
        "WELL    ", "WLLYR   ", "NODE    ", "CONN    ", "REGION  ", "FIELD   ",
        "CONNLIST", "TARGET  ", "FLOSTA  "
    };
    std::array< int, 9 > vars_in_class = {
        56, 52, 4, 43, 69, 58, 20, 25, 25
    };

    auto plt = nex::NexusPlot { ss.str() };
    test_assert_int_equal(plt.num_classes, 9);
    test_assert_int_equal(plt.day,         1);
    test_assert_int_equal(plt.month,       1);
    test_assert_int_equal(plt.year,        1980);
    test_assert_int_equal(plt.nx,          1);
    test_assert_int_equal(plt.ny,          1);
    test_assert_int_equal(plt.nz,          1);
    test_assert_int_equal(plt.ncomp,       2);
    for (int i = 0; i < plt.num_classes; i++)
        test_assert_std_string_equal(class_names[i], plt.class_names[i]);
    for (int i = 0; i < plt.num_classes; i++)
        test_assert_int_equal(vars_in_class[i], plt.vars_in_class[i]);
}

int main(int argc, char* argv[]) {
    test_invalid_header1();
    test_invalid_header2();
    test_valid_header();
    test_spe1_header(argv[1]);
    return 0;
}
