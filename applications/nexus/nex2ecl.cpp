/*
   Copyright (C) 2017  Statoil ASA, Norway.
   The file 'nex2ecl' is part of ERT - Ensemble based Reservoir Tool.

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

#include <cctype>
#include <getopt.h>
#include <iostream>
#include <string>

#include <ert/util/util.h>

#include "nexus/util.hpp"

namespace {

static const constexpr char *usage =
R"(nex2ecl [options]... <nexus_plt_file>
Try 'nex2ecl --help' for more information.)";

static const constexpr char *help =
R"(Usage:
  nex2ecl [options]... <nexus_plt_file>
  nex2ecl -h | --help
Options:
  -f --format                Format the output.
  -h --help                  Show this screen.
  -o --output <file>|<path>  Write output to files prefixed by <file> or place
                             output at <path>.
)";

static struct option long_options[] = {
    {"format", no_argument,       nullptr, 'f'},
    {"help",   no_argument,       nullptr, 'h'},
    {"output", required_argument, nullptr, 'o'}
};

}

void create_ecl_sum( std::string input_path,
                     std::string output_path,
                     bool format_output ) {
    auto ending = input_path.rfind(".plt");
    if (ending == std::string::npos) {
        input_path += ".plt";
    }

    std::string ecl_case_name {};
    {
        char *plt_basename;
        util_alloc_file_components( input_path.c_str(),
                                    NULL,
                                    &plt_basename,
                                    NULL );
        util_strupr( plt_basename );

        if ( output_path.empty() ) {
            ecl_case_name = std::string( plt_basename );
        } else if ( util_is_directory(output_path.c_str()) ) {
            ecl_case_name = output_path + std::string( plt_basename );
        } else {
            char *path = 0, *basename = 0;
            util_alloc_file_components(output_path.c_str(), &path, &basename, NULL);

            if ( !basename ) {
                std::cerr << "Failed to parse output path "
                          << output_path << "." << std::endl;
                exit(1);
            }

            if ( path && !util_is_directory(path) ) {
                std::cerr << path << " does not exists." << std::endl;
                exit(1);
            }

            util_strupr( basename );

            if ( path ) {
                ecl_case_name += std::string( path );
                ecl_case_name += UTIL_PATH_SEP_CHAR;
            }
            ecl_case_name += std::string( basename );

            if ( path ) free( path );
            if ( basename ) free( basename );
        }

        free( plt_basename );
    }

    nex::NexusPlot plt = nex::load( input_path );
    ecl_sum_type *ecl_sum = nex::ecl_summary( ecl_case_name.c_str(),
                                              format_output,
                                              plt );
    ecl_sum_fwrite(ecl_sum);
    ecl_sum_free(ecl_sum);
}

int main( int argc, char **argv ) {
    if (argc == 1) {
        std::cout << usage << std::endl;
        exit(1);
    }

    bool format_output = false;
    std::string output_path {};
    std::string input_path {};

    /*
     * Parse options
     */

    int c;
    for (;;) {
        c = getopt_long (argc, argv, "fho:", long_options, 0);
        if (c == -1) // No more options
            break;

        switch (c) {
        case 0:
            break;
        case 'f':
            format_output = true;
            break;
        case 'h':
            std::cout << help;
            exit(0);
        case 'o':
            output_path = std::string( optarg );
            break;
        case '?':
            exit(1);
        default:
            exit(1);
        }
    }
    if (optind != argc - 1) {
        std::cout << usage << std::endl;
        exit(1);
    }

    input_path = std::string( argv[optind] );

    try {
        create_ecl_sum( input_path, output_path, format_output );
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}
