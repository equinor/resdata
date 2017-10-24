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
#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>
#include <map>

typedef struct ecl_sum_struct ecl_sum_type;

namespace nex {

struct bad_header : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct read_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct unexpected_eof : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct NexusClassItem {
    std::string name;
    int timestep;
    float time;
    int num_items;
    int max_items;
    int max_perfs;
    std::map<std::string, float> variables;
};

typedef std::map<std::string, std::vector<NexusClassItem>> timestep_data;

struct NexusHeader {
    int32_t num_classes;
    int32_t day, month, year;
    int32_t nx, ny, nz;
    int32_t ncomp;
    std::vector<std::string> _class_names;
    std::map< std::string, int32_t> _vars_in_class;
    std::map< std::string, std::vector<std::string> > _var_names;
};

class NexusPlot {
public:
    NexusPlot( const std::string& );
    NexusPlot( std::istream& );

private:
    NexusHeader _header;
    std::map<int, timestep_data> _timesteps;

public:
    const NexusHeader& header() const;
    const std::map<int, const timestep_data>& timesteps() const;

    ecl_sum_type* ecl_summary( const std::string& ecl_case );

private:
    void load( std::istream& );
};

}

#endif // NEXUS_PLOT_H
