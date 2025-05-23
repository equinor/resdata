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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/fortio.h>
#include <resdata/rd_util.hpp>
#include <resdata/rd_endian_flip.hpp>

void file_convert(const char *src_file, const char *target_file,
                  rd_file_enum file_type, bool fmt_src) {
    fortio_type *src, *target;
    bool formatted_src;

    printf("Converting %s -> %s \n", src_file, target_file);
    if (file_type != RD_OTHER_FILE)
        formatted_src = fmt_src;
    else {
        if (util_fmt_bit8(src_file))
            formatted_src = true;
        else
            formatted_src = false;
    }

    target = fortio_open_writer(target_file, !formatted_src, RD_ENDIAN_FLIP);
    src = fortio_open_reader(src_file, formatted_src, RD_ENDIAN_FLIP);

    while (true) {
        if (fortio_read_at_eof(src))
            break;

        {
            rd_kw_type *rd_kw = rd_kw_fread_alloc(src);
            if (rd_kw) {
                rd_kw_fwrite(rd_kw, target);
                rd_kw_free(rd_kw);
            } else {
                fprintf(stderr, "Reading keyword failed \n");
                break;
            }
        }
    }

    fortio_fclose(src);
    fortio_fclose(target);
}

int main(int argc, char **argv) {
    fprintf(stderr, "** Warning: convert.x is deprecated. Use resfo instead\n");
    if (argc == 1) {
        fprintf(stderr,
                "Usage: convert.x <filename1> <filename2> <filename3> ...\n");
        exit(1);
    } else {

        char *src_file = argv[1];
        char *target_file;

        int report_nr;
        rd_file_enum file_type;
        bool fmt_file;
        file_type = rd_get_file_type(src_file, &fmt_file, &report_nr);

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
            int file_nr;
            for (file_nr = 1; file_nr < argc; file_nr++) {
                char *path;
                char *basename;
                char *extension;
                src_file = argv[file_nr];
                file_type = rd_get_file_type(src_file, &fmt_file, &report_nr);
                if (file_type == RD_OTHER_FILE) {
                    fprintf(stderr, "File: %s - problem \n", src_file);
                    fprintf(stderr, "In a list of many files ALL must be "
                                    "recognizable by their name. \n");
                    exit(1);
                }
                util_alloc_file_components(src_file, &path, &basename,
                                           &extension);

                target_file = rd_alloc_filename(path, basename, file_type,
                                                !fmt_file, report_nr);
                file_convert(src_file, target_file, file_type, fmt_file);

                free(path);
                free(basename);
                free(extension);
                free(target_file);
            }
        }
        return 0;
    }
}
