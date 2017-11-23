/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus2ecl.cpp' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include <ert/util/test_work_area.h>
#include <ert/util/test_util.h>

#include <ert/util/util.h>
#include <ert/ecl/ecl_sum.h>

#include <nexus/util.hpp>


void test_create_ecl_sum(char *root_folder) {
    test_work_area_type *work_area = test_work_area_alloc("nexus_header");

    std::stringstream ss;
    ss << root_folder << "/test-data/local/nexus/SPE1.plt";
    const nex::NexusPlot plt = nex::load(ss.str());
    auto data = plt.data;

    /* Write */

    ecl_sum_type *ecl_sum = nex::ecl_summary( "ECL_CASE", false, plt );
    test_assert_true( ecl_sum_is_instance( ecl_sum ));
    ecl_sum_fwrite( ecl_sum );
    ecl_sum_free( ecl_sum );
    test_assert_true( util_file_exists( "ECL_CASE.SMSPEC"));

    /* Load */

    ecl_sum_type * ecl_sum_loaded = ecl_sum_fread_alloc_case("ECL_CASE", ":");

    auto timesteps = unique( plt, nex::get::timestep );

    /* FOPR values */
    std::vector< nex::NexusData > fopr_values;
    std::copy_if( data.begin(), data.end(), std::back_inserter( fopr_values ),
        []( const nex::NexusData& d ) {
            return nex::is::classname( "FIELD" )(d)
                && nex::is::instancename( "NETWORK" )(d)
                && nex::is::varname( "QOP" )(d);
        });
    std::sort( fopr_values.begin(), fopr_values.end(), nex::cmp::timestep );

    /* WLPT values */
    std::vector< nex::NexusData > wlpt_values;
    std::copy_if( data.begin(), data.end(), std::back_inserter( wlpt_values ),
                  []( const nex::NexusData& d ) {
                      return nex::is::classname( "WELL" )(d)
                             && nex::is::instancename( "2" )(d)
                             && nex::is::varname( "COWP" )(d);
                  });
    std::sort( wlpt_values.begin(), wlpt_values.end(), nex::cmp::timestep );

    /* Check data */

    test_assert_true( ecl_sum_has_key(ecl_sum_loaded, "FOPR"));
    test_assert_true( ecl_sum_has_key(ecl_sum_loaded, "WLPT:1"));
    test_assert_true( ecl_sum_has_key(ecl_sum_loaded, "WLPT:2"));
    test_assert_time_t_equal( ecl_sum_get_start_time( ecl_sum_loaded ),
                             util_make_date_utc( 1, 1, 1980));
    test_assert_int_equal( ecl_sum_get_data_length( ecl_sum_loaded ),
                            timesteps.size());

    float conversion = plt.header.unit_system.conversion("COWP");

    for (int t = 0; t < ecl_sum_get_data_length( ecl_sum_loaded ); t++) {
        test_assert_double_equal(
                ecl_sum_get_general_var(ecl_sum_loaded, t, "FOPR"),
                fopr_values[t].value);

        test_assert_double_equal(
                ecl_sum_get_general_var(ecl_sum_loaded, t, "WLPT:2"),
                wlpt_values[t].value * conversion);
    }

    test_work_area_free(work_area);

}

/**
 * In nexus, instances aren't present in the dataset before the first timestep
 * they appare. In the dataset SPE1_delayedWell.plt, the well instance "3" does
 * not appare before timestep number 5. In ecl, however, the well is present
 * from the beginning. Because of this, the indices where the data starts is
 * different in the nex and ecl representations.
 */
void test_ecl_timesteps(char *root_folder) {
    test_work_area_type *work_area = test_work_area_alloc("nexus_timesteps");

    std::stringstream ss;
    ss << root_folder << "/test-data/local/nexus/SPE1_delayedWell.plt";
    const nex::NexusPlot plt = nex::load(ss.str());
    auto data = plt.data;

    ecl_sum_type *ecl_sum = nex::ecl_summary( "ECL_CASE", false, plt );
    ecl_sum_fwrite( ecl_sum );
    ecl_sum_free( ecl_sum );

    ecl_sum_type * ecl_sum_loaded = ecl_sum_fread_alloc_case("ECL_CASE", ":");
    test_assert_true( ecl_sum_has_key(ecl_sum_loaded, "WLPT:3"));

    std::vector< nex::NexusData > wlpt_values;
    std::copy_if( data.begin(), data.end(), std::back_inserter( wlpt_values ),
                  []( const nex::NexusData& d ) {
                      return nex::is::classname( "WELL" )(d)
                             && nex::is::instancename( "3" )(d)
                             && nex::is::varname( "COWP" )(d);
                  });
    std::sort( wlpt_values.begin(), wlpt_values.end(), nex::cmp::timestep );

    float cowp_conversion = plt.header.unit_system.conversion( "COWP" );
    test_assert_double_equal( ecl_sum_get_general_var(ecl_sum_loaded, 4, "WLPT:3"), wlpt_values[0].value * cowp_conversion);
    test_assert_double_equal( ecl_sum_get_general_var(ecl_sum_loaded, 5, "WLPT:3"), wlpt_values[1].value * cowp_conversion);
    test_assert_double_equal( ecl_sum_get_general_var(ecl_sum_loaded, 6, "WLPT:3"), wlpt_values[2].value * cowp_conversion);
    test_assert_double_equal( ecl_sum_get_general_var(ecl_sum_loaded, 7, "WLPT:3"), wlpt_values[3].value * cowp_conversion);
    test_assert_double_equal( ecl_sum_get_general_var(ecl_sum_loaded, 8, "WLPT:3"), wlpt_values[4].value * cowp_conversion);
    test_assert_double_equal( ecl_sum_get_general_var(ecl_sum_loaded, 9, "WLPT:3"), wlpt_values[5].value * cowp_conversion);

    test_work_area_free(work_area);
}


int main(int argc, char **argv) {
    util_install_signals();
    test_create_ecl_sum(argv[1]);
    test_ecl_timesteps(argv[1]);
    exit(0);
}
