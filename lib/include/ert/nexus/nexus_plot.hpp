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
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <ert/ecl/ecl_sum.h>


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

struct NexusHeader {
    int32_t num_classes;
    int32_t day, month, year;
    int32_t nx, ny, nz;
    int32_t ncomp;
};

struct NexusData {
    int32_t timestep;
    float time;
    int32_t max_perfs;
    std::array< char, 8 > classname;
    std::array< char, 8 > instance_name;
    std::array< char, 4 > varname;
    float value;

    std::string classname_str()     {
        return std::string(classname.begin(), classname.end());
    }
    std::string instance_name_str() { 
        return std::string(instance_name.begin(), instance_name.end());
    }
    std::string varname_str()       {
        return std::string(varname.begin(), varname.end());
    }

    constexpr bool operator==(const NexusData& rhs) const noexcept {
        return this->timestep      == rhs.timestep &&
               this->time          == rhs.time &&
               this->max_perfs     == rhs.max_perfs &&
               this->classname     == rhs.classname &&
               this->instance_name == rhs.instance_name &&
               this->varname       == rhs.varname &&
               this->value         == rhs.value;
    }
    constexpr bool operator!=(const NexusData& rhs) const noexcept {
        return ! (*this == rhs);
    }
    // bool operator< (const NexusData& rhs) { }
    // bool operator<=(const NexusData& rhs) { }
    // bool operator> (const NexusData& rhs) { }
    // bool operator>=(const NexusData& rhs) { }
};

struct NexusPlot {
    NexusHeader header;
    std::vector< NexusData > data;
};

NexusHeader read_header( std::istream& stream );
NexusPlot load( const std::string& );
NexusPlot load( std::istream& );

struct ecl_sum_deleter {
    void operator()( ecl_sum_type* e ) {
        ecl_sum_free( e );
    }
};

ecl_sum_type* ecl_summary( const std::string&, const NexusPlot& );

}

#endif // NEXUS_PLOT_H
