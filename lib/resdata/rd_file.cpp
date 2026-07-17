#include <cstdio>

#include <ios>
#include <fstream>
#include <istream>
#include <ostream>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <filesystem>

#include <ert/util/util.hpp>
#include <fmt/format.h>

#include <resdata/FortIO.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
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

   The rd::File is the middle layer of abstraction in the resdata
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

/** Will scan through the whole file and build an index of all kewyords.
   The map created from this scan will be stored under the 'global_view'
   field; and all subsequent lookup operations will ultimately be based
   on the global map.

   The rd_file_scan function will scan through the file as long as it finds
   valid rd_kw instances on the disk; it will return when EOF is encountered or
   an invalid rd_kw instance is detected. This implies that for a partly broken
   file the rd_file_scan function will index the valid keywords which are in
   the file, possible garbage at the end will be ignored. */
void rd::File::scan() {
    context->fortio.fseek(0, SEEK_SET);
    {
        rd_kw_ptr work_kw = make_rd_kw("WORK-KW", 0, RD_INT, nullptr);

        while (true) {
            if (context->fortio.read_at_eof())
                break;

            {
                offset_type current_offset = context->fortio.ftell();
                rd_read_status_enum read_status =
                    rd_kw_fread_header(work_kw.get(), context->fortio);
                if (read_status == RD_KW_READ_FAIL)
                    break;

                if (read_status == RD_KW_READ_OK) {
                    auto file_kw =
                        std::make_shared<FileKW>(work_kw.get(), current_offset);

                    if (file_kw->skip_data(context->fortio)) {
                        global_view->add_kw(file_kw);
                    } else {
                        break;
                    }
                }
            }
        }
    }
    global_view->make_index();
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
std::unique_ptr<rd::File> rd::File::open(const std::string &filename,
                                         FileMode flags) {
    auto fortio = rd_file_alloc_fortio(filename, flags);

    auto context = std::make_shared<rd::FileContext>(std::move(*fortio), flags);
    auto global_view = std::make_shared<rd::FileView>(context);
    auto rd_file =
        std::unique_ptr<rd::File>(new rd::File(context, global_view));

    rd_file->scan();

    if ((rd_file->context->flags & FileMode::CLOSE_STREAM) ==
        FileMode::CLOSE_STREAM)
        rd_file->context->fortio.fclose_stream();

    return rd_file;
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

// We disallow reading strings longer than 32767 bytes
// as this is the most permissive length of a file path
// using windows LongPathsEnabled behavior.
constexpr int max_len = 32767;

static std::string read_index_filename(std::istream &stream) {
    int len = 0;
    stream.read(reinterpret_cast<char *>(&len), sizeof(len));
    if (len <= 0)
        throw std::ios_base::failure(fmt::format(
            "Index file contained empty or negative sized filename: size={}",
            len));

    if (len > max_len)
        throw std::ios_base::failure(
            fmt::format("Invalid filename length: {}", len));

    std::string s(static_cast<std::size_t>(len), '\0');
    stream.read(s.data(), len);
    stream.ignore(1); /* consume the trailing nul */
    return s;
}

static void check_valid_index_stream(const std::string &file_name,
                                     std::istream &stream) {
    std::string input_name = fs::path{file_name}.filename().string();
    std::string source_file;
    try {
        source_file = read_index_filename(stream);
    } catch (const std::ios_base::failure &) {
        std::throw_with_nested(std::ios_base::failure(fmt::format(
            "Index file did not contain a valid index for \"{}\"", file_name)));
    }

    if (source_file != input_name)
        throw std::ios_base::failure(fmt::format(
            "Index file did not contain a valid index for \"{}\"", file_name));
}

static void write_index_filename(const std::string &s, std::ostream &stream) {
    if (s.empty())
        throw std::ios_base::failure("Index filename must not be empty");
    if (s.size() > static_cast<std::size_t>(max_len))
        throw std::ios_base::failure(
            fmt::format("Invalid index filename length: {}", s.size()));
    int len = static_cast<int>(s.size());
    stream.write(reinterpret_cast<const char *>(&len), sizeof(len));
    stream.write(s.data(), static_cast<std::streamsize>(s.size()));
    stream.put('\0');
}

void rd::File::write_index(const std::string &index_filename) {
    std::ofstream ostream(index_filename, std::ios_base::binary);
    if (!ostream)
        throw std::ios_base::failure(fmt::format(
            "Failed to open index file \"{}\" for writing", index_filename));
    ostream.exceptions(std::ios_base::failbit | std::ios_base::badbit);

    write_index_filename(
        fs::path{context->fortio.filename()}.filename().string(), ostream);
    global_view->write_index(ostream);
    ostream.flush();
}

std::unique_ptr<rd::File>
rd::File::fast_open(const std::string &file_name,
                    const std::string &index_file_name, FileMode flags) {
    check_valid_index(file_name, index_file_name);

    std::ifstream istream(index_file_name, std::ios_base::binary);
    if (!istream)
        throw std::ios_base::failure(
            fmt::format("Failed to open file \"{}\"", index_file_name));
    istream.exceptions(std::ios_base::failbit | std::ios_base::badbit);

    check_valid_index_stream(file_name, istream);

    auto fortio = rd_file_alloc_fortio(file_name, flags);
    auto context = std::make_shared<rd::FileContext>(std::move(*fortio), flags);
    auto global_view = rd::FileView::read(context, istream);
    auto rd_file =
        std::unique_ptr<rd::File>(new rd::File(context, global_view));
    if ((rd_file->context->flags & FileMode::CLOSE_STREAM) ==
        FileMode::CLOSE_STREAM)
        rd_file->context->fortio.fclose_stream();
    return rd_file;
}
