/*
   Copyright (C) 2017  Statoil ASA, Norway.
   The file 'nex2ecl' is part of ERT - Ensemble based Reservoir Tool.

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

#include <string>

#include <iostream>

#include <ert/nexus/nexus_plot.hpp>


void create_ecl_sum(char *file_path) {
    std::string filename(file_path);
    auto ending = filename.rfind(".plt");
    if (ending == std::string::npos) {
        filename += ".plt";
    }

    nex::NexusPlot plt = nex::load(filename);

    std::string basename(filename);
    basename.erase(basename.find_last_of('.'), basename.size());

    ecl_sum_type *ecl_sum = nex::ecl_summary(basename, plt);
    ecl_sum_fwrite(ecl_sum);
    ecl_sum_free(ecl_sum);
}


int main(int argc, char **argv) {
    create_ecl_sum(argv[1]);
    exit(0);
}
