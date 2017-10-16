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


namespace {
    const std::vector< std::string > spe1_classnames {{
        "WELL    ", "NODE    ", "CONN    ", "REGION  ", "FIELD   ", "CONNLIST"
    }};
    const std::array< int, 9 > spe1_vars_in_class {{
        56, 52, 4, 43, 69, 58, 20, 25, 25
    }};
    const std::array< int, 10 > spe1_timesteps {{
        33, 46, 59, 72, 85, 89, 91, 92, 94, 97
    }};
    const std::array< std::vector< std::string >, 9 > spe1_varnames {{
        std::vector< std::string > {{
            "COP ", "CGP ", "CWP ", "CGI ", "CWI ", "QOP ", "QGP ", "QWP ",
            "QGI ", "QWI ", "BHP ", "WPH ", "WKH ", "WPAV", "THP ", "COWP",
            "QOWP", "GOR ", "WCUT", "WOR ", "QGLG", "CGLG", "DRDN", "DRMX",
            "CROP", "CRGP", "CRWP", "CROI", "CRGI", "CRWI", "ROP ", "RGP ",
            "RWP ", "ROI ", "RGI ", "RWI ", "ONTM", "ALQ ", "API ", "QCDP",
            "CCDP", "YCDP", "ACTV", "STAT", "Q1P ", "Q1I ", "C1P ", "C1I ",
            "X1P ", "Y1P ", "Q2P ", "Q2I ", "C2P ", "C2I ", "X2P ", "Y2P "
        }},
        std::vector< std::string > {{
            "PNOD", "PDAT", "TNOD", "ACTV"
        }},
        std::vector< std::string > {{
            "QGAS", "QOIL", "QWTR", "CGAS", "COIL", "CWTR", "CBFG", "CBFO",
            "CBFW", "QGIS", "QOIS", "QWIS", "P_IN", "POUT", "T_IN", "TOUT",
            "ACTV", "STAT", "CSTR", "ITRG", "ONTM", "ALQ ", "SETM", "SETA",
            "POWM", "POWA", "SPDM", "SPDA", "API ", "DELP", "QTOT", "GVF ",
            "EFF ", "POSN", "WCUT", "GOR ", "WOR ", "Q1  ", "Q2  ", "X1  ",
            "X2  ", "Y1  ", "Y2  "
        }},
        std::vector< std::string > {{
            "COP ", "CGP ", "CWP ", "COI ", "CGI ", "CWI ", "PAVT", "PAVH",
            "OIP ", "GIP ", "WIP ", "QOP ", "QGP ", "QWP ", "QOI ", "QGI ",
            "QWI ", "OIN ", "GIN ", "WIN ", "SO  ", "SG  ", "SW  ", "OREC",
            "FGIP", "CIP ", "PAVE", "PAVD", "ROIP", "RGIP", "RWIP", "MRO ",
            "MRG ", "MRW ", "NFLX", "PV  ", "HCPV", "TAVT", "TAVH", "CROP",
            "CRGP", "CRWP", "CROI", "CRGI", "CRWI", "ROP ", "RGP ", "RWP ",
            "ROI ", "RGI ", "RWI ", "QCDP", "CCDP", "YCDP", "API ", "GOR ",
            "WCUT", "WOR ", "Z1  ", "Z2  ", "MC1 ", "MC2 ", "MC3 ", "C1P ",
            "C2P ", "C3P ", "C1I ", "C2I ", "C3I "
        }},
        std::vector< std::string > {{
            "COP ", "CGP ", "CWP ", "CGI ", "CWI ", "QOP ", "QGP ", "QWP ",
            "QGI ", "QWI ", "COWP", "QOWP", "GOR ", "OREC", "GREC", "PAVT",
            "PAVH", "QGLG", "CGLG", "WCUT", "NFLX", "CROP", "CRGP", "CRWP",
            "CROI", "CRGI", "CRWI", "ROP ", "RGP ", "RWP ", "ROI ", "RGI ",
            "RWI ", "OIP ", "GIP ", "WIP ", "QCDP", "CCDP", "YCDP", "WLLS",
            "PRDW", "GLFW", "WINJ", "GINJ", "ACTW", "API ", "Q1P ", "Q1I ",
            "C1P ", "C1I ", "X1P ", "Y1P ", "Q2P ", "Q2I ", "C2P ", "C2I ",
            "X2P ", "Y2P "
        }},
        std::vector< std::string > {{
            "QOP ", "QGP ", "QWP ", "QOI ", "QGI ", "QWI ", "COP ", "CGP ",
            "CWP ", "COI ", "CGI ", "CWI ", "API ", "WCUT", "GOR ", "WOR ",
            "Q1P ", "Q1I ", "Q2P ", "Q2I "
        }}
    }};
}

void test_spe1_header(const nex::NexusPlot& spe1) {
    test_assert_int_equal(spe1.header.num_classes, 9);
    test_assert_int_equal(spe1.header.day,         1);
    test_assert_int_equal(spe1.header.month,       1);
    test_assert_int_equal(spe1.header.year,        1980);
    test_assert_int_equal(spe1.header.nx,          1);
    test_assert_int_equal(spe1.header.ny,          1);
    test_assert_int_equal(spe1.header.nz,          1);
    test_assert_int_equal(spe1.header.ncomp,       2);
}

void test_spe1_classes(const nex::NexusPlot& spe1) {
    auto classes = spe1.classnames();
    for (const auto& c : classes) {
        auto f = std::find( spe1_classnames.begin(), spe1_classnames.end(), c );
        test_assert_true( f != spe1_classnames.end() );
    }
}

void test_spe1_timesteps(const nex::NexusPlot& spe1) {
    auto data = spe1.data;
    std::vector< int32_t > timesteps;
    std::transform(data.begin(), data.end(), std::back_inserter( timesteps ),
                   nex::get::timestep);
    std::sort( timesteps.begin(), timesteps.end() );
    timesteps.erase( std::unique( timesteps.begin(), timesteps.end() ),
                timesteps.end() );

    for (size_t i = 0; i < timesteps.size(); i++) {
        test_assert_int_equal( timesteps[i], spe1_timesteps[i] );
    }
}

void test_spe1_well_comulative_gas_injected(const nex::NexusPlot& spe1) {
    std::vector< float > cgi_all_timesteps {{
        2.197554E+07, 2.221688E+07, 2.221765E+07, 2.221765E+07, 2.221765E+07,
        2.221765E+07, 2.221765E+07, 2.221765E+07, 2.221765E+07, 2.221765E+07
    }};

    auto data = spe1.data;
    auto pred_well_1_cgi = [](const nex::NexusData& d) {
        return nex::is::classname("WELL    ")(d)
            && nex::is::instance_name("1       ")(d)
            && nex::is::varname("CGI ")(d);
    };
    std::vector< nex::NexusData > well_1_cgi;
    std::copy_if( data.begin(), data.end(), std::back_inserter( well_1_cgi ),
                  pred_well_1_cgi );

    test_assert_int_equal( well_1_cgi.size(), cgi_all_timesteps.size() );
    for (size_t i = 0; i < cgi_all_timesteps.size(); i++) {
        test_assert_std_string_equal( nex::get::varname_str(well_1_cgi[i]),
                                      std::string { "CGI " } );
        test_assert_float_equal( well_1_cgi[i].value, cgi_all_timesteps[i] );
    }
}

void test_spe1_region_rgs00002_ts97_all_variables(const nex::NexusPlot& spe1) {
    std::vector< float > rgs00002_ts97_values {{
        2.523329E+07, 4.136424E+07, 1.447312E+00, 0.000000E+00, 2.221765E+07,
        0.000000E+00, 4.455184E+03, 4.461495E+03, 2.589262E+05, 3.418254E+05,
        6.335829E+04, 4.616032E+03, 1.070334E+04, 3.910403E-04, 0.000000E+00,
        0.000000E+00, 0.000000E+00, 0.000000E+00, 0.000000E+00, 0.000000E+00,
        7.575924E-01, 1.221403E-01, 1.202673E-01, 8.951881E+00, 6.995434E+04,
        1.175401E+03, 4.461297E+03, 4.467622E+03, 4.101614E+05, 6.612690E+04,
        6.511288E+04, 3.290200E+05, 5.733295E+04, 5.478092E+02, 0.000000E+00,
        5.414012E+05, 4.762883E+05, 0.000000E+00, 0.000000E+00, 3.920510E+07,
        2.185938E+07, 1.494259E+00, 0.000000E+00, 9.035565E+06, 0.000000E+00,
        6.939682E+03, 9.079640E+03, 4.040660E-04, 0.000000E+00, 0.000000E+00,
        0.000000E+00, 1.599658E-02, 2.989084E+02, 2.231668E+00, 3.776983E+01,
        2.318732E+00, 8.471349E-08, 8.471351E-08, 7.952136E-01, 2.047864E-01,
        7.678800E+10, 1.977474E+10, 2.212642E+10, 7.506947E+09, 2.390970E+09,
        5.054768E+02, 0.000000E+00, 1.286641E+09, 0.000000E+00
    }};

    auto data = spe1.data;
    auto pred_rgs00002_ts97 = [](const nex::NexusData& d){
        return nex::is::instance_name("RGS00002")(d)
            && nex::is::timestep( 97 )(d);
    };
    std::vector< nex::NexusData > rgs00002_ts97;
    std::copy_if( data.begin(), data.end(), std::back_inserter( rgs00002_ts97 ),
                  pred_rgs00002_ts97 );

    // Test the test
    test_assert_int_equal( rgs00002_ts97_values.size(),
                           spe1_varnames[3].size() );
    test_assert_int_equal( rgs00002_ts97_values.size(),
                           rgs00002_ts97.size() );
    for (size_t i = 0; i < rgs00002_ts97.size(); i++) {
        test_assert_std_string_equal( nex::get::classname_str(rgs00002_ts97[i]),
                                      std::string { "REGION  " } );
        test_assert_std_string_equal( nex::get::varname_str(rgs00002_ts97[i]),
                                      spe1_varnames[3][i] );
        test_assert_float_equal( rgs00002_ts97[i].value,
                                 rgs00002_ts97_values[i] );

    }
}

void test_spe1_class_varnames(const nex::NexusPlot& plt) {
    auto data = plt.data;

    for (size_t i = 0; i < spe1_classnames.size(); i++) {
        auto varnames = plt.varnames( spe1_classnames[i] );
        auto spe1_varnames_sorted = spe1_varnames[i];
        std::sort( spe1_varnames_sorted.begin(), spe1_varnames_sorted.end() );

        test_assert_int_equal( varnames.size(),
                               spe1_varnames_sorted.size() );
        for (size_t k = 0; k < varnames.size(); k++) {
            test_assert_std_string_equal( varnames[k],
                                          spe1_varnames_sorted[k] );
        }
    }
}


int main(int argc, char* argv[]) {
    std::stringstream ss;
    ss << argv[1] << "/test-data/local/nexus/SPE1.plt";
    const auto spe1 = nex::load(ss.str());

    test_spe1_header(spe1);
    test_spe1_classes(spe1);
    test_spe1_timesteps(spe1);
    test_spe1_well_comulative_gas_injected(spe1);
    test_spe1_region_rgs00002_ts97_all_variables(spe1);
    test_spe1_class_varnames(spe1);
    return 0;
}
