/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus_plot.hpp' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef NEXUS_PLOT_H
#define NEXUS_PLOT_H

#include <cinttypes>
#include <exception>
#include <fstream>
#include <string>

namespace nex {

class bad_header : public std::exception {};

class NexusPlot {
public:
    static NexusPlot load( const std::string& );
    static NexusPlot load( std::istream& );
};

}

#endif // NEXUS_PLOT_H
