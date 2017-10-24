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
#include <sstream>
#include <iostream>
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
#include <ert/ecl/ecl_sum.h>


const std::string NEXUS_PLOT_TYPE_HEADER = "PLOT  BIN ";

namespace {

template< int N >
std::string read_str(std::istream& stream) {
    std::array< char, N> buf;
    stream.read( buf.data(), N );
    return std::string( buf.data(), N );
}

struct hinfo {
    int32_t num_classes;
    int32_t day, month, year;
    int32_t nx, ny, nz;
    int32_t ncomp;
    std::vector<std::string> class_names;
    std::vector<int32_t> vars_in_class;
    std::vector< std::vector<std::string> > var_names;
};

hinfo headerinfo( std::istream& stream ) {
    stream.seekg(4 + 10 + 562 + 264, std::ios::beg);

    std::array< int32_t, 8 > buf {};
    stream.read( (char*)buf.data(), buf.max_size() * 4 );
    if ( !stream.good() ) throw nex::unexpected_eof("");

    auto ntoh = []( int32_t x ) { return ntohl( x ); };
    std::transform( buf.begin(), buf.end(), buf.begin(), ntoh );
    auto negative = []( int32_t x ) { return x < 0; };
    if ( std::any_of( buf.begin(), buf.end(), negative ) )
        throw nex::bad_header("Negative value, corrupted file");

    hinfo h = {
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
        {},
        std::vector< int32_t >( buf[0], 0 ),
        std::vector< std::vector<std::string> >( buf[0],
            std::vector<std::string>()),
    };

    stream.seekg(8, std::ios::cur);
    std::array< char, 8 > class_name;
    for (int i = 0; i < h.num_classes; i++) {
        stream.read(class_name.data(), 8);
        h.class_names.push_back(std::string( class_name.data(), 8 ));
    }

    stream.seekg(8, std::ios::cur);
    stream.read((char*) h.vars_in_class.data(), h.num_classes * 4);
    std::transform( h.vars_in_class.begin(),
                    h.vars_in_class.end(),
                    h.vars_in_class.begin(),
                    ntoh );
    if (std::any_of( h.vars_in_class.begin(), h.vars_in_class.end(), negative))
        throw nex::bad_header("Negative value, corrupted file");

    stream.seekg(8, std::ios::cur);
    for (int i = 0; i < h.num_classes; ++i) {
        stream.seekg(4, std::ios::cur);
        std::vector< char > var_names( h.vars_in_class[i] * 4, 0 );
        stream.read( var_names.data(), h.vars_in_class[i] * 4 );
        for (int k = 0; k < h.vars_in_class[i]; ++k)
            h.var_names[i].push_back( std::string( var_names.data() + k*4 ,4 ));
        stream.seekg(8, std::ios::cur);
    }
    stream.seekg(4, std::ios::cur);

    return h;
}

}

nex::NexusPlot::NexusPlot( const std::string& filename ) {
    std::ifstream stream(filename, std::ios::binary);
    if ( !stream.good() )
        throw nex::read_error("Could not open file " + filename);
    this->load(stream);
}

nex::NexusPlot::NexusPlot( std::istream& stream ) {
    this->load(stream);
}

void nex::NexusPlot::load(std::istream& stream) {
    struct stream_guard {
        stream_guard( std::istream& stream ) :
            mask( stream.exceptions() ),
            s( stream ) {}
        ~stream_guard() { this->s.exceptions( this->mask ); }
        std::ios::iostate mask;
        std::istream& s;
    } g { stream };
    stream.exceptions( std::ios::goodbit );

    stream.seekg(4, std::ios::beg); // skip 4 bytes
    auto type_header = read_str<10>(stream);

    if (type_header.compare(NEXUS_PLOT_TYPE_HEADER) != 0 || !stream.good())
        throw nex::bad_header("Could not verify file type");

    auto header        = headerinfo( stream );
    this->num_classes   = header.num_classes;
    this->day           = header.day;
    this->month         = header.month;
    this->year          = header.year;
    this->nx            = header.nx;
    this->ny            = header.ny;
    this->nz            = header.nz;
    this->ncomp         = header.ncomp;
    this->class_names   = header.class_names;
    this->vars_in_class = header.vars_in_class;
    this->var_names     = header.var_names;
}

//char* ecl_key(char* key) {
//    std::string ecl_keys = "FOPT";
//    return ecl_keys.c_str();
//}
//
//const char* ecl_unit(const char key) {
//    const char *ecl_units = "Barrels";
//    return ecl_units;
//}

ecl_sum_type *nex::ecl_summary(const std::string &ecl_case, const NexusPlot &plt) {
    bool unified = true;
    bool fmt_output = false;
    const char* key_join_string = ":";
    time_t sim_start = 0;
    bool time_in_days = true;

    ecl_sum_type * ecl_sum = ecl_sum_alloc_writer( ecl_case.c_str(),
        fmt_output,
        unified,
        key_join_string,
        sim_start,
        time_in_days,
        this->nx, this->ny, this->nz);

    // Data container
    auto data = plt.data;



//    nex::NexusData classes = ;
//    for (const auto& cl : classes) {
//
//        nex::NexusData varnames = ;
//        for (const auto& varname : varnames) {
//
//            nex::NexusData wells = ;
//            for (const auto& well : wells) {
//
//                //Get relevant data struct from plt
//                nex::NexusData current_struct = std::copy_if(data.classname(cl).var_name(varname).instance_name(well), plt);
//
//                //Copy values from struct
//                current_values = copy_if(plt.begin(), plt.end(), std::back_inserter(classes),
//                                     [](x) { x.classname != classes.back(); });
//
//                //Add variable to ecl_sum
//                //ecl_sum_add_var( ecl_sum , keyword, wgname, size, unit, default_value )
//                current_handler = ecl_sum_add_var(ecl_sum, ecl_key(varname), data.instance_name(well), sizeof(timesteps), ecl_unit(varname), 0.0);
//
//                //Initialize variable
//                //ecl_sum_init_var( ecl_sum , node , keyword , wgname , num , unit )
//                ecl_sum_init_var(ecl_sum, current_handler, ecl_key(varname), data.instance_name(well), sizeof(timesteps), ecl_unit(varname));
//
//                //Iterate through all timesteps
//                nex::NexusData timesteps = ;
//                for (const auto &timestep : timesteps) {
//                    //Create timestep
//                    ecl_sum_tstep_type *tstep = ecl_sum_add_tstep(ecl_sum, timestep.timestep, (int) timestep.time);
//
//                    //Add values to ecl_sum for current timestep
//                    ecl_sum_tstep_iset(tstep, current_handler, timestep.value);
//                }
//            }
//        }
//    }


    // Get all WOPR
    auto pred_well_1_qop = [](const nex::NexusData& d) {
        return nex::is::classname("WELL    ")
            && nex::is::instance_name("1       ")
            && nex::is::varname("QOP ");
    };
    std::vector< nex::NexusData > well_1_qop;
    std::copy_if( data.begin(), data.end(), std::back_inserter( well_1_qop ),
                  pred_well_1_qop );

    const auto& fst = well_1_qop.front();
    std::string wgname = nex::get::instance_name_str(fst);
    std::string ecl_key = std::string("WOPR:") + wgname;

    auto *smspec_node = ecl_sum_add_var(ecl_sum,
                                        "WOPR",
                                        wgname.c_str(),
                                        -1,
                                        "Barrels",
                                        -1.0);
    ecl_sum_init_var(ecl_sum ,
                     smspec_node ,
                     "WOPR",
                     wgname.c_str(),
                     -1,
                     "Barrels");

    auto* tstep = ecl_sum_add_tstep( ecl_sum, fst.timestep, fst.time );
    ecl_sum_tstep_set_from_node( tstep , smspec_node, fst.time );





    /**TODO:
     *
     * define string classes    %List of all classnames
     * define string keywords   %List of keywords for each classname
     * define ecl_key[key]      %Map from nexus keyword to eclipse keyword
     * define ecl_key[key]      %Unit for keywords
     *
     *
     * **/

    return ecl_sum;
}
