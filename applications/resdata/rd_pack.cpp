/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'rd_pack.cpp' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/util.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_type.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

bool fname_cmp(const std::string &f1, const std::string &f2) {
    return rd_fname_report_cmp(f1.c_str(), f2.c_str()) < 0;
}

int main(int argc, char **argv) {
    int num_files = argc - 1;
    if (num_files >= 1) {
        /* File type and formatted / unformatted is determined from the first argument on the command line. */
        fs::path filepath(argv[1]);
        std::string filename = filepath.string();
        rd_file_enum file_type, target_type;
        bool fmt_file;

        /** Look at the first command line argument to determine type and formatted/unformatted status. */
        file_type = rd_get_file_type(filename.c_str(), &fmt_file, NULL);
        if (file_type == RD_SUMMARY_FILE)
            target_type = RD_UNIFIED_SUMMARY_FILE;
        else if (file_type == RD_RESTART_FILE)
            target_type = RD_UNIFIED_RESTART_FILE;
        else {
            util_exit("The rd_pack program can only be used with "
                      "restart files or summary files.\n");
            target_type = RD_OTHER_FILE;
        }
        /*
         * Will pack to cwd, even though the source files might be
         * somewhere else. To unpack to the same directory as the source
         * files, just send in @path as first argument when creating the
         * target_file.
         */

        fs::path target_file =
            rd::filename(filepath.stem(), target_type, fmt_file, -1);

        std::vector<std::string> filelist(argv + 1, argv + argc);
        std::sort(filelist.begin(), filelist.end(), fname_cmp);

        rd_kw_ptr seqnum_kw(nullptr, &rd_kw_free);
        ERT::FortIO target(target_file, std::ios_base::out, fmt_file);

        if (target_type == RD_UNIFIED_RESTART_FILE) {
            int dummy;
            seqnum_kw.reset(rd_kw_alloc_new("SEQNUM", 1, RD_INT, &dummy));
        }

        int prev_report_step = -1;
        for (int i = 0; i < num_files; i++) {
            rd_file_enum this_file_type;
            int report_step;
            this_file_type =
                rd_get_file_type(filelist.at(i).c_str(), NULL, &report_step);
            if (this_file_type == file_type) {
                if (report_step == prev_report_step)
                    util_exit(
                        "Tried to write same report step twice: %s / %s \n",
                        filelist.at(i - 1).c_str(), filelist.at(i).c_str());

                prev_report_step = report_step;
                rd_file_ptr src_file = open_rd_file(filelist.at(i));
                if (target_type == RD_UNIFIED_RESTART_FILE) {
                    /* Must insert the SEQNUM keyword first. */
                    rd_kw_iset_int(seqnum_kw.get(), 0, report_step);
                    rd_kw_fwrite(seqnum_kw.get(), target);
                }
                rd_file_fwrite_fortio(src_file.get(), target, 0);
            } /* Else skipping file of incorrect type. */
        }
    }
}
