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
#include <iostream>
#include <map>
#include <sstream>


#include <ert/util/build_config.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#elif defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#elif defined(HAVE_WINSOCK2_H)
#include <winsock2.h>
#endif

#include <ert/nexus/nexus_plot.hpp>


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
        throw nex::bad_header("Negative value, corrupted file");

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
        throw nex::unexpected_eof("");

    return varnames;
}

void read_vars(std::istream& stream,
               std::vector< nex::NexusData >& data,
               int32_t timestep,
               float time,
               int32_t max_perfs,
               str< 8 > classname,
               str< 8 > instance_name,
               const std::vector< str< 4 > >& varnames) {

    std::vector< int32_t > values( varnames.size() );
    stream.read( (char*)values.data(), values.size() * sizeof(int32_t) );
    std::transform( values.begin(), values.end(), values.begin(), ntoh );

    for (size_t k = 0; k < varnames.size(); k++) {
        data.push_back( nex::NexusData {
            timestep,
            time,
            max_perfs,
            classname,
            instance_name,
            varnames[k],
            interpret_float( values[k] )
        });
    }
}

}

nex::NexusHeader nex::read_header( std::istream& stream ) {
    stream.seekg(4, std::ios::beg); // skip 4 bytes

    static constexpr const char th0[11] = "PLOT  BIN ";
    auto th1 = read_str<10>(stream);
    auto ok = std::equal(std::begin(th1), std::end(th1), std::begin(th0));

    if (!ok || !stream.good())
        throw nex::bad_header("Could not verify file type");

    // skip unknown blob
    stream.seekg(562 + 264, std::ios::cur);

    std::array< int32_t, 8 > buf {};
    stream.read( (char*)buf.data(), buf.max_size() * 4 );
    if ( !stream.good() ) throw nex::unexpected_eof("");

    std::transform( buf.begin(), buf.end(), buf.begin(), ntoh );
    auto negative = []( int32_t x ) { return x < 0; };
    if ( std::any_of( buf.begin(), buf.end(), negative ) )
        throw nex::bad_header("Negative value, corrupted file");

    nex::NexusHeader h = {
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
    };

    return h;
}

nex::NexusPlot nex::load( const std::string& filename ) {
    std::ifstream stream(filename, std::ios::binary);
    if ( !stream.good() )
        throw nex::read_error("Could not open file " + filename);
    return nex::load(stream);
}

nex::NexusPlot nex::load(std::istream& stream) {
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

    nex::NexusPlot plt { header, {} };
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
        if ( !stream.good() ) throw nex::unexpected_eof("");
        std::transform( buf.begin(), buf.end(), buf.begin(), ntoh );

        int32_t timestep  = (int32_t) interpret_float(buf[0]);
        float   time      = interpret_float(buf[1]);
        int32_t num_items = (int32_t) interpret_float(buf[2]);
        // int32_t max_items = (int32_t) interpret_float(buf[3]);
        int32_t max_perfs = (int32_t) interpret_float(buf[4]);

        for (int i = 0; i < num_items; i++) {
            str<8> instance_name;
            stream.read(instance_name.data(), 8);
            stream.seekg(72, std::ios::cur);
            read_vars(stream,
                      data,
                      timestep,
                      time,
                      max_perfs,
                      classname,
                      instance_name,
                      varnames[classname]);
        }
        stream.seekg(8, std::ios::cur);
    }
}

char* ecl_key(char* key) {
    std::string ecl_keys = "FOPT";
    return ecl_keys.c_str();
}

const char* ecl_unit(const char key) {
    const char *ecl_units = "Barrels";
    return ecl_units;
}


ecl_sum_type *nex::ecl_summary(const std::string &ecl_case, const NexusPlot &plt) {
    bool unified = true;
    bool fmt_output = false;
    const char *key_join_string = ":";
    time_t sim_start = 0;
    bool time_in_days = true;


    ecl_sum_type *ecl_sum = ecl_sum_alloc_writer(ecl_case.c_str(),
                                                 fmt_output,
                                                 unified,
                                                 key_join_string,
                                                 sim_start,
                                                 time_in_days,
                                                 plt.header.nx, plt.header.ny, plt.header.nz);

    // Data container
    auto &data = plt.data;

    //Jens API for getting classes
    auto unique_cls = std::unique(data.begin(), data.end(),
                                  [](const nex::NexusData &a, const nex::NexusData &b) {
                                      return a.classname == b.classname;
                                  });

    //Iterate through all
    for(auto clitr = data.begin(); clitr != unique_cls; ++clitr ) {
        const auto& cl = *clitr;

        //Jens API for getting variables

        auto unique_varname = std::unique(data.begin(), .end(),
                                          [](const nex::NexusData &a, const nex::NexusData &b) {
                                              return a.varname == b.varname;
                                          });

        //Iterate through all
        for (const auto &varname : unique_varname) {

            //Jens API for getting timesteps
            std::vector<nex::NexusData> timesteps;
            std::copy_if(data.begin(), data.end(), std::back_inserter(timesteps),
                         [](const nex::NexusData &nd) {
                             return nd.classname == cl.classname && nd.varname == varname.varname
                         });


            //Add variable to ecl_sum
            //ecl_sum_add_var( ecl_sum , keyword , wgname, size, unit, default_value )
            current_handler = ecl_sum_add_var(ecl_sum, ecl_key(varname.varname_str()), NULL, sizeof(timesteps), ecl_unit(varname.varname_str()), 0.0);


            //Initialize variable
            //ecl_sum_init_var( ecl_sum , node , keyword , wgname , num , unit )
            ecl_sum_init_var(ecl_sum, current_handler, ecl_key(cl.varname_str()), cl.instance_name_str().c_str(),
                             sizeof(timesteps), ecl_unit(varname));


            //Iterate through all timesteps
            for (const auto &timestep : timesteps) {
                //Create timestep
                ecl_sum_tstep_type *tstep = ecl_sum_add_tstep(ecl_sum, timestep.timestep, (int) timestep.time);

                //Add values to ecl_sum for current timestep
                ecl_sum_tstep_iset(tstep, current_handler, timestep.value);
            }

        }
    }

    /**TODO:
     *
     * define ecl_key[key]      %Map from nexus keyword to eclipse keyword
     * define ecl_key[key]      %Unit for keywords
     *
     * **/

    ecl_sum_fwrite( ecl_sum );

    return ecl_sum;
}
