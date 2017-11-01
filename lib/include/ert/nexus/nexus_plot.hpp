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

#include <algorithm>
#include <array>
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
    std::array< char, 8 > instancename;
    std::array< char, 4 > varname;
    float value;

    constexpr bool operator==(const NexusData& rhs) const noexcept {
        return this->timestep      == rhs.timestep &&
               this->time          == rhs.time &&
               this->max_perfs     == rhs.max_perfs &&
               this->classname     == rhs.classname &&
               this->instancename == rhs.instancename &&
               this->varname       == rhs.varname &&
               this->value         == rhs.value;
    }
    constexpr bool operator!=(const NexusData& rhs) const noexcept {
        return ! (*this == rhs);
    }
};

namespace get {

    std::array< char, 8 > classname( const NexusData& nd ) {
        return nd.classname;
    }
    std::array< char, 8 > instancename( const NexusData& nd ) {
        return nd.instancename;
    }
    std::array< char, 4 > varname( const NexusData& nd ) {
        return nd.varname;
    }
    std::string classname_str( const NexusData& nd ) {
        auto end = std::find( nd.classname.begin(), nd.classname.end(), ' ' );
        return std::string(nd.classname.begin(), end);
    }
    std::string instancename_str( const NexusData& nd ) {
        auto end = std::find( nd.instancename.begin(), nd.instancename.end(), ' ' );
        return std::string(nd.instancename.begin(), end);
    }
    std::string varname_str( const NexusData& nd ) {
        auto end = std::find( nd.varname.begin(), nd.varname.end(), ' ' );
        return std::string(nd.varname.begin(), end);
    }
    int32_t timestep( const NexusData& nd ) {
        return nd.timestep;
    }
    float time( const NexusData& nd ) {
        return nd.time;
    }
    int32_t max_perfs( const NexusData& nd ) {
        return nd.max_perfs;
    }
    float value( const NexusData& nd ) {
        return nd.value;
    }
}

namespace is {

    struct timestep {
        int32_t value;
        timestep( int32_t x ) : value( x ) {}
        bool operator()( const NexusData& nd ) const {
            return this->value == nd.timestep;
        }
    };
    struct time {
        float value;
        time( float x ) : value( x ) {}
        bool operator()( const NexusData& nd ) const {
            return this->value == nd.time;
        }
    };
    struct max_perfs {
        int32_t value;
        max_perfs( int32_t x ) : value( x ) {}
        bool operator()( const NexusData& nd ) const {
            return this->value == nd.max_perfs;
        }
    };
    struct classname {
        std::array< char, 8 > value {{ ' ' }};
        classname( std::array< char, 8 > x ) : value( x ) {}
        classname( const std::string& x ) {
            if (x.size() > value.max_size())
                throw std::runtime_error("Could not make predicated, gived classname was larger than 8 bytes.");
            std::copy(x.begin(), x.end(), value.begin());
            std::replace(value.begin(), value.end(), '\0', ' ');
        }
        classname( const char (&x)[9] ) {
            std::copy(std::begin(x), std::end(x), value.begin());
            std::replace(value.begin(), value.end(), '\0', ' ');
        }
        bool operator()( const NexusData& nd ) const {
            return this->value == nd.classname;
        }
    };
    struct instancename {
        std::array< char, 8 > value {{ ' ' }};
        instancename( std::array< char, 8 > x ) : value( x ) {}
        instancename( const std::string& x ) {
            if (x.size() > value.max_size())
                throw std::runtime_error("Could not make predicated, gived instancename was larger than 8 bytes.");
            std::copy(x.begin(), x.end(), value.begin());
            std::replace(value.begin(), value.end(), '\0', ' ');
        }
        instancename( const char (&x)[9] ) {
            std::copy(std::begin(x), std::end(x), value.begin());
            std::replace(value.begin(), value.end(), '\0', ' ');
        }
        bool operator()( const NexusData& nd ) const {
            return this->value == nd.instancename;
        }
    };
    struct varname {
        std::array< char, 4 > value {{ ' ' }};
        varname( std::array< char, 4 > x ) : value( x ) {}
        varname( const std::string& x ) {
            if (x.size() > value.max_size())
                throw std::runtime_error("Could not make predicated, gived varname was larger than 8 bytes.");
            std::copy(x.begin(), x.end(), value.begin());
            std::replace(value.begin(), value.end(), '\0', ' ');
        }
        varname( const char (&x)[9] ) {
            std::copy(std::begin(x), std::end(x), value.begin());
            std::replace(value.begin(), value.end(), '\0', ' ');
        }
        bool operator()( const NexusData& nd ) const {
            return this->value == nd.varname;
        }
    };
    struct value {
        float val;
        value( float x ) : val( x ) {}
        bool operator()( const NexusData& nd ) const {
            return this->val == nd.value;
        }
    };
}

namespace cmp {
    bool timestep( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.timestep < rhs.timestep;
    }
    bool time( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.time < rhs.time;
    }
    bool max_perfs( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.max_perfs < rhs.max_perfs;
    }
    bool classname( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.classname < rhs.classname;
    }
    bool instancename( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.instancename < rhs.instancename;
    }
    bool varname( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.varname < rhs.varname;
    }
    bool value( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.value < rhs.value;
    }
}

namespace equal {
    bool timestep( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.timestep == rhs.timestep;
    }
    bool time( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.time == rhs.time;
    }
    bool max_perfs( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.max_perfs == rhs.max_perfs;
    }
    bool classname( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.classname == rhs.classname;
    }
    bool instancename( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.instancename == rhs.instancename;
    }
    bool varname( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.varname == rhs.varname;
    }
    bool value( const NexusData& lhs, const NexusData& rhs ) {
        return lhs.value == rhs.value;
    }
}

struct NexusPlot {
    NexusHeader header;
    std::vector< NexusData > data;

    template< typename T >
    std::vector< T > get_unique( T (&f)(const NexusData&) ) const {
        std::vector< T > vec;
        vec.reserve( this->data.size() );
        std::transform( this->data.begin(), this->data.end(),
                        std::back_inserter( vec ),
                        f);
        auto last = std::unique(vec.begin(), vec.end());
        std::sort(vec.begin(), last);
        last = std::unique(vec.begin(), last);
        vec.erase(last, vec.end());
        return vec;
    }

    std::vector< std::string > varnames(char (&classname)[9]) const {
        std::array< char, 8> cn {{ ' ' }};
        std::copy( std::begin(classname), std::end(classname),
                   std::begin( cn ) );
        std::replace(cn.begin(), cn.end(), '\0', ' ');
        return this->varnames( cn );
    }
    std::vector< std::string > varnames(std::string classname) const {
        std::array< char, 8> cn {{ ' ' }};
        if (classname.size() > cn.max_size())
            throw std::runtime_error("Could not get varnames, given classname was larger than 8 bytes");
        std::copy( std::begin(classname), std::end(classname),
                   std::begin( cn ) );
        std::replace(cn.begin(), cn.end(), '\0', ' ');
        return this->varnames( cn );
    }
    std::vector< std::string > varnames(std::array< char, 8 > classname) const {
        std::vector< NexusData > of_class;
        std::copy_if( this->data.begin(), this->data.end(),
                      std::back_inserter( of_class ),
                      nex::is::classname(classname) );

        std::vector< std::string > vec;
        vec.reserve( of_class.size() );
        std::transform( of_class.begin(), of_class.end(),
                        std::back_inserter( vec ),
                        nex::get::varname_str);
        std::transform ( vec.begin(), vec.end(), vec.begin(),
            []( std::string& str ){
                auto end = std::find( str.begin(), str.end(), ' ' );
                str.erase( end, str.end() );
                return str;
            });
        auto last = std::unique(vec.begin(), vec.end());
        std::sort(vec.begin(), last);
        last = std::unique(vec.begin(), last);
        vec.erase(last, vec.end());
        return vec;
    }
};

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
