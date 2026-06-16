/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'convert.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <filesystem>

namespace fs = std::filesystem;

void file_convert(const std::string &src_file, const std::string &target_file,
                  rd_file_enum file_type, bool fmt_src) {
    bool formatted_src;

    printf("Converting %s -> %s \n", src_file.c_str(), target_file.c_str());
    if (file_type != RD_OTHER_FILE)
        formatted_src = fmt_src;
    else {
        if (util_fmt_bit8(src_file.c_str()))
            formatted_src = true;
        else
            formatted_src = false;
    }

    ERT::FortIO target(target_file, std::ios_base::out, !formatted_src);
    ERT::FortIO src(src_file, std::ios_base::in, formatted_src);

    while (true) {
        if (src.read_at_eof())
            break;

        {
            rd_kw_type *rd_kw = rd_kw_fread_alloc(src.get());
            if (rd_kw) {
                rd_kw_fwrite(rd_kw, target.get());
                rd_kw_free(rd_kw);
            } else {
                fprintf(stderr, "Reading keyword failed \n");
                break;
            }
        }
    }
}

int main(int argc, char **argv) {
    fprintf(stderr, "** Warning: convert.x is deprecated. Use resfo instead\n");
    if (argc == 1) {
        fprintf(stderr,
                "Usage: convert.x <filename1> <filename2> <filename3> ...\n");
        exit(1);
    } else {

        std::string src_file(argv[1]);
        std::string target_file;

        int report_nr;
        rd_file_enum file_type;
        bool fmt_file;
        file_type = rd_get_file_type(src_file.c_str(), &fmt_file, &report_nr);

        if (file_type == RD_OTHER_FILE) {
            if (argc != 3) {
                fprintf(stderr,
                        "When the file can not be recognized on the name"
                        "you must give output_file as second (and "
                        "final) argument \n");
                exit(0);
            }
            target_file = argv[2];
            file_convert(src_file, target_file, file_type, fmt_file);
        } else {
            for (int i = 1; i < argc; i++) {
                src_file = std::string(argv[i]);
                file_type =
                    rd_get_file_type(src_file.c_str(), &fmt_file, &report_nr);
                if (file_type == RD_OTHER_FILE) {
                    fprintf(stderr, "File: %s - problem \n", src_file.c_str());
                    fprintf(stderr, "In a list of many files ALL must be "
                                    "recognizable by their name. \n");
                    exit(1);
                }
                fs::path src_path(src_file);
                fs::path target_case = src_path.parent_path() / src_path.stem();
                target_file =
                    rd::filename(target_case, file_type, !fmt_file, report_nr)
                        .string();
                file_convert(src_file, target_file, file_type, fmt_file);
            }
        }
        return 0;
    }
}
