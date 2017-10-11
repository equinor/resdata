/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus_plot.cpp' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/nexus/nexus_plot.hpp>


const std::string NEXUS_PLOT_TYPE_HEADER = "PLOT  BIN ";

nex::NexusPlot nex::NexusPlot::load(const std::string& filename) {
    std::ifstream stream(filename, std::ios::binary);
    return NexusPlot::load(stream);
}

nex::NexusPlot nex::NexusPlot::load(std::istream& stream) {
    stream.ignore(4); // skip 4 bytes
    std::string type_header(10,'\0');
    stream.read(&type_header[0], 10);

    if (type_header.compare(NEXUS_PLOT_TYPE_HEADER) != 0)
        throw nex::bad_header();

    return NexusPlot {};
}
