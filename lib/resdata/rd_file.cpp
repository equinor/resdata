#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <ctime>

#include <ios>
#include <memory>
#include <stdexcept>
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
   stored in the field global_map, and the field global_view is set to
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
   structure. No keyword data will be loaded from the file.*/
rd_file_ptr rd::File::open(const std::string &filename, FileMode flags) {
    auto fortio = rd_file_alloc_fortio(filename, flags);

    auto context = std::make_shared<rd::FileContext>(std::move(*fortio), flags);
    auto global_view = std::make_shared<rd::FileView>(context);
    auto rd_file = std::make_unique<rd::File>(context, global_view);

    rd_file_scan(rd_file.get());

    if ((rd_file->context->flags & FileMode::CLOSE_STREAM) ==
        FileMode::CLOSE_STREAM)
        rd_file->context->fortio.fclose_stream();

    return rd_file;
}

/* Functions specialized to work with restart files.  */

/* Query functions. */

/** Will look through all the INTEHEAD kw instances of the current
    rd_file and look for @sim_time. If the value is found true is
    returned, otherwise false. */
bool rd_file_has_sim_time(const rd_file_type *rd_file, time_t sim_time) {
    return rd_file->global_view->has_sim_time(sim_time);
}

/** Will look through all the SEQNUM kw instances of the current
    rd_file and look for @report_step. If the value is found true is
    returned, otherwise false. */
bool rd_file_has_report_step(const rd_file_type *rd_file, int report_step) {
    return rd_file->global_view->has_report_step(report_step);
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
bool rd::File::save_kw(const rd_kw_type *rd_kw) {
    FileKW *file_kw = context->inv_map.at(rd_kw);
    if (context->fortio.assert_stream_open()) {

        file_kw->inplace_write(context->fortio);

        if ((context->flags & FileMode::CLOSE_STREAM) == FileMode::CLOSE_STREAM)
            context->fortio.fclose_stream();

        return true;
    } else
        return false;
}

static std::shared_ptr<rd::FileView>
rd_file_get_relative_blockview(rd_file_type *rd_file, const char *kw,
                               int occurence) {
    return rd_file->global_view->blockview(kw, kw, occurence);
}

bool rd_file_subselect_block(rd_file_type *rd_file, const char *kw,
                             int occurence) {
    if (auto blockmap =
            rd_file_get_relative_blockview(rd_file, kw, occurence)) {
        rd_file->global_view = std::move(blockmap);
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

template <typename T> T checked_fread(FILE *f) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "checked_fread requires a trivially copyable type");
    T out{};
    if (fread(&out, sizeof(T), 1, f) != 1)
        throw std::runtime_error(
            fmt::format("read failed: {}", strerror(errno)));
    return out;
}

inline std::string checked_fread(std::size_t n, FILE *f) {
    std::string s(n, '\0');
    if (n > 0 && std::fread(s.data(), 1, n, f) != n)
        throw std::runtime_error(
            fmt::format("read failed: {}", strerror(errno)));
    return s;
}

/** When writing the empty string we write the sequence "-1\0".
    This was used before to disambiguate with the potential for s being NULL,
    which wrote the sequence "0". */
static std::string read_sized_string(FILE *stream) {
    int len = checked_fread<int>(stream);
    if (len == -1) {              /* length indicates "" */
        checked_fread(1, stream); /* consume the trailing nul */
        return "";
    }
    if (len <= 0)
        return ""; /* length indicates NULL; return "" without consuming nul */
    std::string s = checked_fread(len, stream);
    checked_fread(1, stream); /* consume the trailing nul */
    return s;
}

static void check_valid_index_stream(const std::string &file_name,
                                     FILE *stream) {
    std::string source_file = read_sized_string(stream);
    std::string input_name = fs::path{file_name}.filename().string();

    if (source_file != input_name)
        throw std::ios_base::failure(fmt::format(
            "Index file did not contain a valid index for \"{}\"", file_name));
}

template <typename T> void checked_fwrite(const T &s, FILE *f) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "checked_fwrite requires a trivially copyable type");
    if (fwrite(&s, sizeof(s), 1, f) != 1)
        throw std::runtime_error(
            fmt::format("write failed: {}", strerror(errno)));
}

inline void checked_fwrite(const std::string &s, FILE *f) {
    std::size_t nbytes = s.size() + 1;
    if (std::fwrite(s.data(), 1, nbytes, f) != nbytes)
        throw std::runtime_error(
            fmt::format("write failed: {}", strerror(errno)));
}

static void write_sized_string(const std::string &s, FILE *stream) {
    if (s.size() == 0)
        checked_fwrite<int>(-1, stream); /* Writing magic string for "" */
    else
        checked_fwrite<int>(static_cast<int>(s.size()), stream);
    checked_fwrite(s, stream);
}

bool rd::File::write_index(const std::string &index_filename) {
    std::unique_ptr<FILE, decltype(&fclose)> ostream(
        fopen(index_filename.c_str(), "wb"), fclose);
    if (!ostream)
        return false;

    write_sized_string(fs::path{context->fortio.filename()}.filename().string(),
                       ostream.get());
    global_view->write_index(ostream.get());
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
    auto rd_file = std::make_unique<rd::File>(context, global_view);
    if ((rd_file->context->flags & FileMode::CLOSE_STREAM) ==
        FileMode::CLOSE_STREAM)
        rd_file->context->fortio.fclose_stream();
    return rd_file;
}
