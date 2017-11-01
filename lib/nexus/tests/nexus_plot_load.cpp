/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus_plot_load.cpp' is part of ERT - Ensemble based Reservoir Tool.

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

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>


#include <ert/util/test_util.hpp>
#include <ert/nexus/nexus_plot.hpp>


void test_invalid_header1() {
    std::stringstream stream( "xxxxINVALID_HEADER" );
    test_assert_throw(nex::load(stream), nex::bad_header);
}

void test_invalid_header2() {
    std::stringstream stream( "xxx" );
    test_assert_throw(nex::load(stream), nex::bad_header);
}

void test_valid_header() {
    std::stringstream stream( "xxxxPLOT  BIN " );
    test_assert_throw(nex::load(stream), nex::unexpected_eof);
}

int main(int argc, char* argv[]) {
    test_invalid_header1();
    test_invalid_header2();
    test_valid_header();
    return 0;
}
