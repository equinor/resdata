/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'rd_pack.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/util.h>
#include <ert/util/stringlist.h>

#include <resdata/rd_file.h>
#include <resdata/rd_util.h>
#include <resdata/rd_endian_flip.h>
#include <resdata/rd_type.h>

int main(int argc, char **argv) {
    int num_files = argc - 1;
    if (num_files >= 1) {
        /* File type and formatted / unformatted is determined from the first argument on the command line. */
        char *rd_base;
        char *path;
        rd_file_enum file_type, target_type;
        bool fmt_file;

        /** Look at the first command line argument to determine type and formatted/unformatted status. */
        file_type = rd_get_file_type(argv[1], &fmt_file, NULL);
        if (file_type == RD_SUMMARY_FILE)
            target_type = RD_UNIFIED_SUMMARY_FILE;
        else if (file_type == RD_RESTART_FILE)
            target_type = RD_UNIFIED_RESTART_FILE;
        else {
            util_exit("The rd_pack program can only be used with "
                      "restart files or summary files.\n");
            target_type = -1;
        }
        util_alloc_file_components(argv[1], &path, &rd_base, NULL);

        /**
       Will pack to cwd, even though the source files might be
       somewhere else. To unpack to the same directory as the source
       files, just send in @path as first argument when creating the
       target_file.
    */

        {
            int i, report_step, prev_report_step;
            char *target_file_name =
                rd_alloc_filename(NULL, rd_base, target_type, fmt_file, -1);
            stringlist_type *filelist =
                stringlist_alloc_argv_copy((const char **)&argv[1], num_files);
            rd_kw_type *seqnum_kw = NULL;
            fortio_type *target =
                fortio_open_writer(target_file_name, fmt_file, RD_ENDIAN_FLIP);

            if (target_type == RD_UNIFIED_RESTART_FILE) {
                int dummy;
                seqnum_kw = rd_kw_alloc_new("SEQNUM", 1, RD_INT, &dummy);
            }

            stringlist_sort(filelist, rd_fname_report_cmp);
            prev_report_step = -1;
            for (i = 0; i < num_files; i++) {
                rd_file_enum this_file_type;
                this_file_type = rd_get_file_type(stringlist_iget(filelist, i),
                                                  NULL, &report_step);
                if (this_file_type == file_type) {
                    if (report_step == prev_report_step)
                        util_exit(
                            "Tried to write same report step twice: %s / %s \n",
                            stringlist_iget(filelist, i - 1),
                            stringlist_iget(filelist, i));

                    prev_report_step = report_step;
                    {
                        rd_file_type *src_file =
                            rd_file_open(stringlist_iget(filelist, i), 0);
                        if (target_type == RD_UNIFIED_RESTART_FILE) {
                            /* Must insert the SEQNUM keyword first. */
                            rd_kw_iset_int(seqnum_kw, 0, report_step);
                            rd_kw_fwrite(seqnum_kw, target);
                        }
                        rd_file_fwrite_fortio(src_file, target, 0);
                        rd_file_close(src_file);
                    }
                } /* Else skipping file of incorrect type. */
            }
            fortio_fclose(target);
            free(target_file_name);
            stringlist_free(filelist);
            if (seqnum_kw != NULL)
                rd_kw_free(seqnum_kw);
        }
        free(rd_base);
        free(path);
    }
}
