/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'rd_unpack.cpp' is part of ERT - Ensemble based Reservoir Tool.

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

#include <cstddef>
#include <cstdio>

#include <ios>
#include <memory>
#include <filesystem>
#include <string>

#include <ert/util/util.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_view.hpp>

namespace fs = std::filesystem;

static void unpack_file(const fs::path &filepath) {
    std::string filename = filepath;
    FileType target_type = FileType::OTHER;
    FileType file_type;
    bool fmt_file;
    file_type = rd_get_file_type(filename.c_str(), &fmt_file, NULL);
    if (file_type == FileType::UNIFIED_SUMMARY)
        target_type = FileType::SUMMARY;
    else if (file_type == FileType::UNIFIED_RESTART)
        target_type = FileType::RESTART;
    else
        util_exit("Can only unpack unified summary and restart files\n");

    if (target_type == FileType::SUMMARY) {
        printf("** Warning: when unpacking unified summary files it as "
               "ambigous - starting with 0001  -> \n");
    }
    std::unique_ptr<rd::File> src_file = rd::File::open(filename);
    size_t size;
    size_t offset;
    int report_step = 0;

    if (target_type == FileType::SUMMARY)
        size = src_file->num_named_kw("SEQHDR");
    else
        size = src_file->num_named_kw("SEQNUM");

    for (size_t block_index = 0; block_index < size; block_index++) {
        std::shared_ptr<rd::FileView> active_view;

        if (target_type == FileType::SUMMARY) {
            active_view = src_file->blockview(SEQHDR_KW, block_index);
            report_step += 1;
            offset = 0;
        } else {
            rd_kw_type *seqnum_kw;
            active_view = src_file->blockview(SEQNUM_KW, block_index);
            seqnum_kw = active_view->get_kw(SEQNUM_KW, 0);
            report_step = rd_kw_iget_int(seqnum_kw, 0);
            offset = 1;
        }

        /*
        Will unpack to cwd, even though the source files might be
        somewhere else. To unpack to the same directory as the source
        files, just send in @path as first argument when creating the
        target_file.
        */

        fs::path target_file =
            rd::filename(filepath.stem(), target_type, fmt_file, report_step);
        ERT::FortIO fortio_target(target_file, std::ios_base::out, fmt_file);
        active_view->write(fortio_target, offset);
    }
}

int main(int argc, char **argv) {
    if (argc == 1)
        util_exit("rd_unpack UNIFIED_FILE1   UNIFIED_FILE2   ...\n");
    for (int i = 1; i < argc; i++)
        unpack_file(fs::path(argv[i]));
}
