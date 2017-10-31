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

#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <set>


#include <ert/util/build_config.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#elif defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#elif defined(HAVE_WINSOCK2_H)
#include <winsock2.h>
#endif

#include <ert/nexus/nexus_plot.hpp>

using namespace nex;


namespace {

uint32_t ntoh(uint32_t ui) { return ntohl(ui); }

float interpret_float(int32_t i) {
    float f;
    std::memcpy( &f, &i, sizeof( float ) );
    return f;
}

template< int N >
using str = std::array< char, N >;

template< int N >
str< N > read_str(std::istream& stream) {
    std::array< char, N> buf;
    stream.read( buf.data(), N );
    return buf;
}

NexusHeader read_header( std::istream& stream ) {
    stream.seekg(4, std::ios::beg); // skip 4 bytes

    static constexpr const char th0[11] = "PLOT  BIN ";
    auto th1 = read_str<10>(stream);
    auto ok = std::equal(std::begin(th1), std::end(th1), std::begin(th0));

    if (!ok || !stream.good())
        throw bad_header("Could not verify file type");

    // skip unknown blob
    stream.seekg(562 + 264, std::ios::cur);

    std::array< int32_t, 8 > buf {};
    stream.read( (char*)buf.data(), buf.max_size() * 4 );
    if ( !stream.good() ) throw unexpected_eof("");

    std::transform( buf.begin(), buf.end(), buf.begin(), ntoh );
    auto negative = []( int32_t x ) { return x < 0; };
    if ( std::any_of( buf.begin(), buf.end(), negative ) )
        throw bad_header("Negative value, corrupted file");

    NexusHeader h = {
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
    };

    return h;
}

std::map< str<8>, std::vector<str<4>> >
read_varnames( std::istream& stream, int32_t num_classes )  {
    std::vector< str<8> > classnames;
    std::vector< int32_t > vars_in_class( num_classes );
    std::map< str<8>, std::vector<str<4>> > varnames;

    stream.seekg(8, std::ios::cur);
    str< 8 > classname;
    for (int i = 0; i < num_classes; i++) {
        stream.read( classname.data(), 8 );
        classnames.push_back(classname);
    }

    stream.seekg(8, std::ios::cur);
    stream.read((char*) vars_in_class.data(), num_classes * 4);
    std::transform( vars_in_class.begin(),
                    vars_in_class.end(),
                    vars_in_class.begin(),
                    ntoh );
    auto negative = []( int32_t x ) { return x < 0; };
    if (std::any_of( vars_in_class.begin(), vars_in_class.end(), negative))
        throw bad_header("Negative value, corrupted file");

    stream.seekg(8, std::ios::cur);
    for (int i = 0; i < num_classes; ++i) {
        varnames[classnames[i]] = {};
        stream.seekg(4, std::ios::cur); // skip time varname
        std::vector<char> buf( vars_in_class[i] * 4, 0 );
        stream.read( buf.data(), vars_in_class[i] * 4 );
        str<4> varname;
        for (int k = 0; k < vars_in_class[i]; ++k) {
            memcpy(varname.data(), buf.data() + k*4, 4);
            varnames[classnames[i]].push_back(varname);
        }
        stream.seekg(8, std::ios::cur);
    }

    if (stream.eof() || !stream.good())
        throw unexpected_eof("");

    return varnames;
}

void read_vars(std::istream& stream,
               std::vector< NexusData >& data,
               int32_t timestep,
               float time,
               int32_t max_perfs,
               str< 8 > classname,
               str< 8 > instancename,
               const std::vector< str< 4 > >& varnames) {

    std::vector< int32_t > values( varnames.size() );
    stream.read( (char*)values.data(), values.size() * sizeof(int32_t) );
    std::transform( values.begin(), values.end(), values.begin(), ntoh );

    for (size_t k = 0; k < varnames.size(); k++) {
        data.push_back( NexusData {
            timestep,
            time,
            max_perfs,
            classname,
            instancename,
            varnames[k],
            interpret_float( values[k] )
        });
    }
}

}

NexusPlot nex::load( const std::string& filename ) {
    std::ifstream stream(filename, std::ios::binary);
    if ( !stream.good() )
        throw read_error("Could not open file " + filename);
    return load(stream);
}

NexusPlot nex::load(std::istream& stream) {
    struct stream_guard {
        stream_guard(std::istream &stream) :
                mask(stream.exceptions()),
                s(stream) {}

        ~stream_guard() { this->s.exceptions(this->mask); }

        std::ios::iostate mask;
        std::istream &s;
    } g{stream};
    stream.exceptions(std::ios::goodbit);

    auto header = read_header(stream);
    auto varnames = read_varnames(stream, header.num_classes);

    /*
     * Load data
     */

    NexusPlot plt { header, {} };
    auto& data = plt.data;

    while( true ) {
        auto classname = read_str<8>(stream);

        static constexpr const char stop[9] = "STOP    ";

        if( std::equal(std::begin(classname), std::end(classname),
                       std::begin(stop)) )
            return plt;

        stream.seekg(8, std::ios::cur);

        std::array< int32_t, 5 > buf {};
        stream.read( (char*)buf.data(), buf.max_size() * 4 );
        if ( !stream.good() ) throw unexpected_eof("");
        std::transform( buf.begin(), buf.end(), buf.begin(), ntoh );

        int32_t timestep  = (int32_t) interpret_float(buf[0]);
        float   time      = interpret_float(buf[1]);
        int32_t num_items = (int32_t) interpret_float(buf[2]);
        // int32_t max_items = (int32_t) interpret_float(buf[3]);
        int32_t max_perfs = (int32_t) interpret_float(buf[4]);

        for (int i = 0; i < num_items; i++) {
            stream.seekg(8, std::ios::cur);
            str<8> instancename;
            stream.read(instancename.data(), 8);
            stream.seekg(64, std::ios::cur);
            read_vars(stream,
                      data,
                      timestep,
                      time,
                      max_perfs,
                      classname,
                      instancename,
                      varnames[classname]);
        }
        stream.seekg(8, std::ios::cur);
    }
}

ecl_sum_type* nex::ecl_summary(const std::string& ecl_case, const NexusPlot& plt) {
    bool fmt_output = false;
    bool unified = true;
    const char* key_join_string = ":";
    std::time_t sim_start = util_make_date_utc( plt.header.day,
                                           plt.header.month,
                                           plt.header.year );
    bool time_in_days = true;

    ecl_sum_type * ecl_sum = ecl_sum_alloc_writer( ecl_case.c_str(),
        fmt_output,
        unified,
        key_join_string,
        sim_start,
        time_in_days,
        plt.header.nx, plt.header.ny, plt.header.nz);

    auto data = plt.data;


    /*

    1) Lag header
    2) Lag timesteps
    3) Set verdier

     */

    std::map< std::string, std::string > kw_nex2ecl {
        {"QOP ", "FOPR" },
        {"QWP ", "FWPR" },
        {"QGP ", "FGPR" },
        {"GOR ", "FGOR" },
        {"WCUT", "FWCT" },
        {"MULT", "FPR"  },
        {"MULT", "FPR"  },
        {"COP ", "FOPT" },
        {"CWP ", "FWPT" },
        {"CGP ", "FGPT" },
        {"QWI ", "FWIR" },
        {"QGI ", "FGIR" },
        {"CWI ", "FWIT" },
        {"CGI ", "FGIT" },
        {"QPP ", "FCPR" },
        {"CPP ", "FCPC" }
    };


    std::map< std::string, std::string > well_kw_nex2ecl {
        {"QOP ", "WOPR" },
        {"QWP ", "WWPR" },
        {"QGP ", "WGPR" },
        {"GOR ", "WGOR" },
        {"WCUT", "WWCT" },
        {"MULT", "WPR"  },
        {"MULT", "WPR"  },
        {"COP ", "WOPT" },
        {"CWP ", "WWPT" },
        {"CGP ", "WGPT" },
        {"QWI ", "WWIR" },
        {"QGI ", "WGIR" },
        {"CWI ", "WWIT" },
        {"CGI ", "WGIT" },
        {"QPP ", "WCPR" },
        {"CPP ", "WCPC" }
    };

    //Field header
    auto field_class_vars = plt.varnames( "FIELD" );
    std::vector< std::string > field_vars;
    std::vector< smspec_node_type* > smspecs;
    for ( const auto& var : field_class_vars ) {
        auto it = kw_nex2ecl.find( var );
        if ( it != kw_nex2ecl.end() ) {
            const auto& ecl_kw = it->second;
            auto* node = ecl_sum_add_var( ecl_sum, ecl_kw.c_str(), NULL, -1,
                                          "x", 0.0);
            smspecs.push_back( node );
            field_vars.push_back( var );
        } else {
            std::cerr << "Warning: could not convert nexus variable " <<
                      var << " to ecl keyword." << std::endl;
        }
    }

    //Well header
    auto well_class_vars = plt.varnames( "WELL" );
    std::vector< std::string > well_vars;
    std::vector< smspec_node_type* > well_smspecs;
    for ( const auto& var : well_class_vars ) {
        auto it = well_kw_nex2ecl.find( var );
        if ( it != well_kw_nex2ecl.end() ) {
            const auto& well_ecl_kw = it->second;
            auto* well_node = ecl_sum_add_var( ecl_sum, well_ecl_kw.c_str(), "2", -1,
                                          "x", 0.0);
            well_smspecs.push_back( well_node );
            well_vars.push_back( var );
        } else {
            std::cerr << "Warning: could not convert nexus variable " <<
                      var << " to ecl keyword." << std::endl;
        }
    }


    //timesteps
    auto nex_timesteps = plt.get_unique(get::timestep);
    auto nex_times = plt.get_unique(get::time);
    std::vector< ecl_sum_tstep_type* > timesteps;
    for (size_t i = 0; i < nex_timesteps.size(); i++) {
        auto* ts = ecl_sum_add_tstep(ecl_sum, i + 1, nex_times[i] * 86400.f );
        timesteps.push_back( ts );
    }



    /*
    * Write Field
    */
    std::vector< NexusData > field;
    std::copy_if( data.begin(), data.end(), std::back_inserter( field ),
                  []( const NexusData& nd ) {
                      return is::classname( "FIELD" )(nd)
                             && is::instancename( "NETWORK" )(nd);
                  });
    std::sort( field.begin(), field.end(), cmp::timestep );

    for (size_t i = 0; i < field_vars.size(); i++) {
        std::vector< NexusData > values;
        std::copy_if( field.begin(), field.end(), std::back_inserter( values ),
                      is::varname( field_vars[i] ));

        for (size_t k = 0; k < timesteps.size(); k++) {
            ecl_sum_tstep_set_from_node( timesteps[k], smspecs[i],
                                         values[k].value );
            std::cout << "values: " << values[k].value << std::endl;
        }
    }


    /*
     * Write Well
     */
    std::vector< NexusData > well;
    std::copy_if( data.begin(), data.end(), std::back_inserter( well ),
                  []( const NexusData& nd ) {
                      return is::classname( "WELL" )(nd)
                             && is::instancename( "2" )(nd);
                  });
    std::sort( well.begin(), well.end(), cmp::timestep );

    for (size_t i = 0; i < well_vars.size(); i++) {
        std::vector< NexusData > values;
        std::copy_if( well.begin(), well.end(), std::back_inserter( values ),
                      is::varname( well_vars[i] ));

        for (size_t k = 0; k < timesteps.size(); k++) {
            ecl_sum_tstep_set_from_node( timesteps[k], well_smspecs[i],
                                         values[k].value );
        }
    }


    return ecl_sum;
}
