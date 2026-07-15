#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <ctime>

#include <ios>
#include <memory>
#include <string>
#include <utility>
#include <filesystem>

#include <ert/util/vector.hpp>
#include <ert/util/util.hpp>
#include <fmt/format.h>

#include <resdata/FortIO.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_util.hpp>

namespace fs = std::filesystem;
/**
   This file implements functionality to load a file in
   restart format. The implementation works by first searching through
   the complete file to create an index over all the keywords present
   in the file. The actual keyword data is not loaded before they are
   explicitly requested.

   The rd_file_type is the middle layer of abstraction in the resdata
   hierarchy (see the file overview.txt in this directory); it works
   with a collection of rd_kw instances and has various query
   functions, however it does not utilize knowledge of the
   structure/content of the files in the way e.g. rd_grid.c.

   The main datatype here is the rd_file type, but in addition each
   rd_kw instance is wrapped in an rd_file_kw (implemented in
   rd_file_kw.c) structure and all the indexing is implemented with
   the rd_file_view type. The rd_file_view type is not used outside this file.

   When the file is opened an index of all the keywords is created and
   stored in the field global_map, and the field active_view is set to
   point to global_map, i.e. all query/get operations on the rd_file
   will be based on the complete index:

   In many cases (in particular for unified restart files) it is quite
   painful to work with this large and unvieldy index, and it is
   convenient to create a sub index based on a subset of the
   keywords. The creation of these sub indices is based on identifying
   a keyword from name and occurence number, and then including all
   keywords up to the next occurence of the same keyword:

      SEQHDR            ---\
      MINISTEP  0          |
      PARAMS    .....      |
      MINISTEP  1          |   Block 0
      PARAMS    .....      |
      MINISTEP  2          |
      PARAMS    .....      |
      SEQHDR            ---+
      MINISTEP  3          |
      PARAMS    .....      |
      MINISTEP  4          |   Block 1
      PARAMS    .....      |
      MINISTEP  5          |
      SEQHDR            ---+
      MINISTEP  6          |   Block 2
      PARAMS    ....       |
      SEQHDR            ---+
      MINISTEP  7          |
      PARAMS    ....       |   Block 3
      MINISTEP  8          |
      PARAMS    ....       |

   For the unified summary file depicted here e.g. the call

      rd_file_get_blockmap( rd_file , "SEQHDR" , 2 )

   Will create a sub-index consisting of the (three) keywords in what
   is called 'Block 2' in the figure above. In particular for restart
   files this abstraction is very convenient, because an extra layer
   of functionality is required to get from natural time coordinates
   (i.e. simulation time or report step) to the occurence number (see
   rd_rstfile for more details).

   The following illustrates the indexing. The rd_file contains in
   total 7 rd_kw instances, the global index [0...6] is the internal
   way to access the various keywords. The kw_index is a hash table
   with entries 'SEQHDR', 'MINISTEP' and 'PARAMS'. Each entry in the
   hash table is an integer vector which again contains the internal
   index of the various occurences:

   ------------------
   SEQHDR            \
   MINISTEP  0        |
   PARAMS    .....    |
   MINISTEP  1        |
   PARAMS    .....    |
   MINISTEP  2        |
   PARAMS    .....   /
   ------------------

   kw_index    = {"SEQHDR": [0], "MINISTEP": [1,3,5], "PARAMS": [2,4,6]}    <== This is hash table.
   kw_list     = [SEQHDR , MINISTEP , PARAMS , MINISTEP , PARAMS , MINISTEP , PARAMS]

   [1]: This is not entirely true - there are several specialized function
        for working with restart files. However the restart files are
        still treated as collections of rd_kw instances, and not
        internalized as in e.g. rd_sum.
*/

void rd_file_fwrite_fortio(const rd_file_type *rd_file, ERT::FortIO &target,
                           size_t offset) {
    rd_file->active_view->write(target, offset);
}

/**
   Here comes several functions for querying the rd_file instance, and
   getting pointers to the rd_kw content of the rd_file. For getting
   rd_kw instances there are two principally different access methods:

   * rd_file_iget_named_kw(): This function will take a keyword
   (char *) and an integer as input. The integer corresponds to the
   ith occurence of the keyword in the file.

   * rd_file_iget_kw(): This function just takes an integer index as
   input, and returns the corresponding rd_kw instance - without
   considering which keyword it is.

*/

/** Will return the number of times a particular keyword occurs in a
    rd_file instance. Will return 0 if the keyword can not be found. */
int rd_file_get_num_named_kw(const rd_file_type *rd_file, const char *kw) {
    return rd_file->active_view->num_named_kw(kw);
}

/** The total number of rd_kw instances in the rd_file instance. */
int rd_file_get_size(const rd_file_type *rd_file) {
    return rd_file->active_view->size();
}

/** true if the rd_file instance has at-least one occurence of @kw. */
bool rd_file_has_kw(const rd_file_type *rd_file, const char *kw) {
    return rd_file->active_view->has_kw(kw);
}

const char *rd_file_get_src_file(const rd_file_type *rd_file) {
    return rd_file->context->fortio.filename_ref();
}

rd_kw_type *rd_file_iget_kw(const rd_file_type *file, int global_index) {
    return file->active_view->get_kw(global_index);
}

/** Will return the ith occurence of @kw in @file. */
rd_kw_type *rd_file_iget_named_kw(const rd_file_type *file, const char *kw,
                                  int ith) {
    return file->active_view->get_kw(kw, ith);
}

std::shared_ptr<rd::FileView> rd_file_get_global_view(rd_file_type *rd_file) {
    return rd_file->global_view;
}

std::shared_ptr<rd::FileView> rd_file_get_active_view(rd_file_type *rd_file) {
    return rd_file->active_view;
}

std::shared_ptr<rd::FileView>
rd_file_get_global_blockview(rd_file_type *rd_file, const char *kw,
                             int occurence) {
    return rd_file->global_view->blockview(kw, kw, occurence);
}

std::shared_ptr<rd::FileView> rd_file_get_summary_view(rd_file_type *rd_file,
                                                       int report_step) {
    return rd_file->global_view->summary_view(report_step);
}

/*
  Different functions to open and close a file.
*/

/** Will scan through the whole file and build an index of all kewyords.
   The map created from this scan will be stored under the 'global_view'
   field; and all subsequent lookup operations will ultimately be based
   on the global map.

   The rd_file_scan function will scan through the file as long as it finds
   valid rd_kw instances on the disk; it will return when EOF is encountered or
   an invalid rd_kw instance is detected. This implies that for a partly broken
   file the rd_file_scan function will index the valid keywords which are in
   the file, possible garbage at the end will be ignored. */
static void rd_file_scan(rd_file_type *rd_file) {
    rd_file->context->fortio.fseek(0, SEEK_SET);
    {
        rd_kw_ptr work_kw = make_rd_kw("WORK-KW", 0, RD_INT, nullptr);

        while (true) {
            if (rd_file->context->fortio.read_at_eof())
                break;

            {
                offset_type current_offset = rd_file->context->fortio.ftell();
                rd_read_status_enum read_status =
                    rd_kw_fread_header(work_kw.get(), rd_file->context->fortio);
                if (read_status == RD_KW_READ_FAIL)
                    break;

                if (read_status == RD_KW_READ_OK) {
                    auto file_kw =
                        std::make_shared<FileKW>(work_kw.get(), current_offset);

                    if (file_kw->skip_data(rd_file->context->fortio)) {
                        rd_file->global_view->add_kw(file_kw);
                    } else {
                        break;
                    }
                }
            }
        }
    }
    rd_file->global_view->make_index();
}

static void rd_file_select_global(rd_file_type *rd_file) {
    rd_file->active_view = rd_file->global_view;
}

static std::unique_ptr<ERT::FortIO>
rd_file_alloc_fortio(const std::string &filename, FileMode flags) {
    bool fmt_file;
    rd_fmt_file(filename.c_str(), &fmt_file);

    if ((flags & FileMode::WRITABLE) == FileMode::WRITABLE)
        return std::make_unique<ERT::FortIO>(
            filename, std::ios_base::in | std::ios_base::out, fmt_file);
    else
        return std::make_unique<ERT::FortIO>(filename, std::ios_base::in,
                                             fmt_file);
}

/** The fundamental open file function; all alternative open()
   functions start by calling this one. This function will read
   through the complete file, extract all the keyword headers and
   create the map/index stored in the global_view field of the rd_file
   structure. No keyword data will be loaded from the file.

   The rd_file instance will retain an open fortio reference to the
   file until rd_file_close() is called. */
rd_file_ptr rd::File::open(const std::string &filename, FileMode flags) {
    auto fortio = rd_file_alloc_fortio(filename, flags);

    auto context = std::make_shared<rd::FileContext>(std::move(*fortio), flags);
    auto global_view = std::make_shared<rd::FileView>(context);
    auto rd_file = std::make_unique<rd::File>(
        context, global_view, std::shared_ptr<rd::FileView>{nullptr});

    rd_file_scan(rd_file.get());
    rd_file_select_global(rd_file.get());

    if ((rd_file->context->flags & FileMode::CLOSE_STREAM) ==
        FileMode::CLOSE_STREAM)
        rd_file->context->fortio.fclose_stream();

    return rd_file;
}

bool rd_file_writable(const rd_file_type *rd_file) {
    return (rd_file->context->flags & FileMode::WRITABLE) == FileMode::WRITABLE;
}

/** The rd_file_close() function will close the fortio instance */
void rd_file_close(rd_file_type *rd_file) {
    if (rd_file->context)
        rd_file->context->fortio.fclose_stream();
}

bool rd_file_load_all(rd_file_type *rd_file) {
    return rd_file->active_view->load_all();
}

/* Functions specialized to work with restart files.  */

/* Query functions. */

/** Will look through all the INTEHEAD kw instances of the current
    rd_file and look for @sim_time. If the value is found true is
    returned, otherwise false. */
bool rd_file_has_sim_time(const rd_file_type *rd_file, time_t sim_time) {
    return rd_file->active_view->has_sim_time(sim_time);
}

/** Will determine the restart block corresponding to the
    world time @sim_time; if @sim_time can not be found the function
    will return 0.

    The returned index is the 'occurence number' in the restart file,
    i.e. in the (quite typical case) that not all report steps are
    present the return value will not agree with report step.

    The return value from this function can then be used to get a
    corresponding solution field directly, or the file map can
    restricted to this block.

    Direct access:

       int index = rd_file_get_restart_index( rd_file , sim_time );
       if (index >= 0) {
          rd_kw_type * pressure_kw = rd_file_iget_named_kw( rd_file , "PRESSURE" , index );
          ....
       }

    Specially in the case of LGRs the block restriction should be used. */
int rd_file_get_restart_index(const rd_file_type *rd_file, time_t sim_time) {
    return rd_file->active_view->find_sim_time(sim_time);
}

/** Will look through all the SEQNUM kw instances of the current
    rd_file and look for @report_step. If the value is found true is
    returned, otherwise false. */
bool rd_file_has_report_step(const rd_file_type *rd_file, int report_step) {
    return rd_file->active_view->has_report_step(report_step);
}

/** Will look up the INTEHEAD keyword in a rd_file_type
    instance, and calculate simulation date from this instance.

    Will return -1 if the requested INTEHEAD keyword can not be found. */
time_t rd_file_iget_restart_sim_date(const rd_file_type *restart_file,
                                     int index) {
    return restart_file->active_view->restart_sim_date(index);
}

double rd_file_iget_restart_sim_days(const rd_file_type *restart_file,
                                     int index) {
    return restart_file->active_view->restart_sim_days(index);
}

/** The input @file must be either an INIT file or a restart file. Will
    fail hard if an INTEHEAD kw can not be found - or if the INTEHEAD
    keyword is not sufficiently large.

    The eclipse files can distinguish between ECLIPSE300 ( value == 300)
    and ECLIPSE300-Thermal option (value == 500). This function will
    return ECLIPSE300 in both those cases. */
rd_version_enum rd_file_get_rd_version(const rd_file_type *file) {
    rd_kw_type *intehead_kw = rd_file_iget_named_kw(file, INTEHEAD_KW, 0);
    int int_value = rd_kw_iget_int(intehead_kw, INTEHEAD_IPROG_INDEX);

    if (int_value == INTEHEAD_ECLIPSE100_VALUE)
        return ECLIPSE100;

    if (int_value == INTEHEAD_ECLIPSE300_VALUE)
        return ECLIPSE300;

    if (int_value == INTEHEAD_ECLIPSE300THERMAL_VALUE)
        return ECLIPSE300_THERMAL;

    if (int_value == INTEHEAD_INTERSECT_VALUE)
        return INTERSECT;

    if (int_value == INTEHEAD_FRONTSIM_VALUE)
        return FRONTSIM;

    util_abort("%s: Simulator version value:%d not recognized \n", __func__,
               int_value);
    return (rd_version_enum)0;
}

/**
  1: Oil
  2: Water
  3: Oil + water
  4: Gas
  5: Gas + Oil
  6: Gas + water
  7: Gas + Water + Oil */
int rd_file_get_phases(const rd_file_type *init_file) {
    rd_kw_type *intehead_kw = rd_file_iget_named_kw(init_file, INTEHEAD_KW, 0);
    int phases = rd_kw_iget_int(intehead_kw, INTEHEAD_PHASE_INDEX);
    return phases;
}

/** Will save the content of @rd_kw to the on-disk file wrapped by the
    rd_file instance. This function is quite strict:

    1. The actual keyword which should be updated is identified using
       pointer comparison; i.e. the rd_kw argument must be the actual
       return value from an earlier rd_file_get_kw() operation.

    2. The header data of the rd_kw must be unmodified, and will throw
       std::runtime_error if there is a mismatch.

    3. The rd_file must have been opened with one of the _writable()
       open functions. */
bool rd_file_save_kw(const rd_file_type *rd_file, const rd_kw_type *rd_kw) {
    FileKW *file_kw = rd_file->context->inv_map.at(rd_kw);
    if (rd_file->context->fortio.assert_stream_open()) {

        file_kw->inplace_write(rd_file->context->fortio);

        if ((rd_file->context->flags & FileMode::CLOSE_STREAM) ==
            FileMode::CLOSE_STREAM)
            rd_file->context->fortio.fclose_stream();

        return true;
    } else
        return false;
}

static std::shared_ptr<rd::FileView>
rd_file_get_relative_blockview(rd_file_type *rd_file, const char *kw,
                               int occurence) {
    return rd_file->active_view->blockview(kw, kw, occurence);
}

bool rd_file_subselect_block(rd_file_type *rd_file, const char *kw,
                             int occurence) {
    if (auto blockmap =
            rd_file_get_relative_blockview(rd_file, kw, occurence)) {
        rd_file->active_view = std::move(blockmap);
        return true;
    } else
        return false;
}

static void check_valid_index(const std::string &file_name,
                              const std::string &index_file_name) {
    if (!util_file_exists(file_name.c_str()))
        throw std::ios_base::failure(
            fmt::format("File \"{}\" does not exist", file_name));

    if (!util_file_exists(index_file_name.c_str()))
        throw std::ios_base::failure(
            fmt::format("File \"{}\" does not exist", index_file_name));

    if (util_file_difftime(file_name.c_str(), index_file_name.c_str()) > 0)
        throw std::ios_base::failure(
            fmt::format("The file \"{}\" is newer than its index file \"{}\"",
                        file_name, index_file_name));
}

static void check_valid_index_stream(const std::string &file_name,
                                     FILE *stream) {
    bool name_equal;
    char *source_file = util_fread_alloc_string(stream);
    std::string input_name = fs::path{file_name}.filename().string();

    name_equal = util_string_equal(source_file, input_name.c_str());

    free(source_file);
    if (!name_equal)
        throw std::ios_base::failure(fmt::format(
            "Index file did not contain a valid index for \"{}\"", file_name));
}

bool rd_file_write_index(const rd_file_type *rd_file,
                         const char *index_filename) {
    FILE *ostream = fopen(index_filename, "wb");
    if (!ostream)
        return false;

    util_fwrite_string(fs::path{rd_file->context->fortio.filename()}
                           .filename()
                           .string()
                           .c_str(),
                       ostream);
    rd_file->global_view->write_index(ostream);
    fclose(ostream);
    return true;
}

rd_file_ptr rd::File::fast_open(const std::string &file_name,
                                const std::string &index_file_name,
                                FileMode flags) {
    check_valid_index(file_name, index_file_name);

    std::unique_ptr<FILE, decltype(&fclose)> istream(
        fopen(index_file_name.c_str(), "rb"), fclose);
    if (!istream)
        throw std::ios_base::failure(
            fmt::format("Failed to open file \"{}\"", index_file_name));

    check_valid_index_stream(file_name, istream.get());

    auto fortio = rd_file_alloc_fortio(file_name, flags);
    auto context = std::make_shared<rd::FileContext>(std::move(*fortio), flags);
    auto global_view = rd::FileView::read(context, istream.get());
    auto rd_file = std::make_unique<rd::File>(
        context, global_view, std::shared_ptr<rd::FileView>{nullptr});
    rd_file_select_global(rd_file.get());
    if ((rd_file->context->flags & FileMode::CLOSE_STREAM) ==
        FileMode::CLOSE_STREAM)
        rd_file->context->fortio.fclose_stream();
    return rd_file;
}
