#include <ios>
#include <fstream>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <fmt/format.h>
#include <filesystem>

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <ert/util/util.hpp>

#include <resdata/FortIO.hpp>

/*
  Observe that the stream open functions accept a failure, and call
  the fopen() function directly.
*/

static std::ios_base::openmode fortio_open_mode(std::ios_base::openmode mode,
                                                bool fmt_file,
                                                const std::string &filename) {
    std::ios_base::openmode result;
    if (mode == (std::ios_base::in | std::ios_base::out)) {
        result = std::ios_base::in | std::ios_base::out;
    } else if (mode == std::ios_base::in) {
        if (util_file_exists(filename.c_str()))
            result = std::ios_base::in;
        else
            throw std::ios_base::failure("File " + filename +
                                         " does not exist");
    } else if (mode == std::ios_base::app) {
        result = std::ios_base::out | std::ios_base::app;
    } else {
        result = std::ios_base::out | std::ios_base::trunc;
    }

    if (!fmt_file)
        result |= std::ios_base::binary;

    return result;
}

/**
   Helper function for fortio_is_fortran_stream__().
*/
static bool __read_int(std::istream &stream, int *value, bool endian_flip) {
    if (stream.read(reinterpret_cast<char *>(value), sizeof *value)) {
        if (endian_flip)
            util_endian_flip_vector(value, sizeof *value, 1);
        return true;
    } else {
        stream.clear();
        return false;
    }
}

/**
   Helper function for FortIO::looks_like_fortran_file(). Checks whether a
   particular stream is formatted according to fortran io, for a fixed
   endianness.
*/
static bool fortio_is_fortran_stream__(std::istream &stream, bool endian_flip) {
    const bool strict_checking =
        true; /* True: requires that *ALL* records in the file are fortran formatted */
    offset_type init_pos = stream.tellg();
    bool is_fortran_stream = false;
    int header, tail;
    bool cont;

    do {
        cont = false;
        if (__read_int(stream, &header, endian_flip)) {
            if (header >= 0) {
                stream.seekg(static_cast<offset_type>(header),
                             std::ios_base::cur);
                if (stream.good()) {
                    if (__read_int(stream, &tail, endian_flip)) {
                        cont = true;
                        // Read a header and a tail so it might be a fortran file.
                        if (header == tail) {
                            if (header != 0) {
                                // This is (most probably) a fortran file
                                is_fortran_stream = true;
                                if (strict_checking)
                                    cont = true;
                                else
                                    cont = false;
                            }
                            // Header == tail == 0 - we don't make any inference on this.
                        } else {
                            // Header != tail => this is *not* a fortran file
                            cont = false;
                            is_fortran_stream = false;
                        }
                    }
                }
            }
        }
    } while (cont);
    stream.clear();
    stream.seekg(init_pos, std::ios_base::beg);
    return is_fortran_stream;
}

namespace ERT {

FortIO::FortIO(const std::string &filename, std::ios_base::openmode mode,
               bool fmt_file, bool endian_flip_header) {
    open(filename, mode, fmt_file, endian_flip_header);
}

FortIO::~FortIO() { close(); }

FortIO::FortIO(FortIO &&other) noexcept
    : m_stream(std::move(other.m_stream)),
      m_filename(std::move(other.m_filename)),
      m_endian_flip_header(std::exchange(other.m_endian_flip_header, false)),
      m_fmt_file(std::exchange(other.m_fmt_file, false)),
      m_open_mode(std::exchange(other.m_open_mode, std::ios_base::openmode{})),
      m_writable(std::exchange(other.m_writable, false)),
      m_read_size(std::exchange(other.m_read_size, 0)) {
    other.m_filename = "";
}

FortIO &FortIO::operator=(FortIO &&other) noexcept {
    if (this == &other)
        return *this;

    close();

    m_stream = std::move(other.m_stream);
    m_filename = std::move(other.m_filename);
    m_endian_flip_header = std::exchange(other.m_endian_flip_header, false);
    m_fmt_file = std::exchange(other.m_fmt_file, false);
    m_open_mode = std::exchange(other.m_open_mode, std::ios_base::openmode{});
    m_writable = std::exchange(other.m_writable, false);
    m_read_size = std::exchange(other.m_read_size, 0);

    other.m_filename = "";

    return *this;
}

void FortIO::open(const std::string &filename, std::ios_base::openmode mode,
                  bool fmt_file, bool endian_flip_header) {
    std::ios_base::openmode open_mode =
        fortio_open_mode(mode, fmt_file, filename);

    std::fstream stream(filename, open_mode);
    if (!stream.is_open())
        throw std::ios_base::failure("Failed to open FortIO file " + filename);

    m_filename = filename;
    m_endian_flip_header = endian_flip_header;
    m_fmt_file = fmt_file;
    m_writable = (mode & std::ios_base::out) || (mode & std::ios_base::app);
    m_read_size = 0;
    m_stream = std::move(stream);
    m_open_mode = open_mode;
    m_read_size =
        static_cast<offset_type>(std::filesystem::file_size(filename));
}

void FortIO::close() {
    if (m_stream.is_open())
        m_stream.close();
    m_stream = std::fstream();
    m_filename = "";
    m_open_mode = std::ios_base::openmode{};
    m_writable = false;
    m_read_size = 0;
}

/**
   This function tries (using some heuristic) to guess whether a
   particular file is a Fortran file.

   The heuristic algorithm which is used is as follows:

    1. Read four bytes as an integer (header)
    2. Skip that number of bytes forward.
    3. Read four bytes again (tail).

   Now, when this is done we do the following test:

   If header == tail. This is (probably) a fortran file, however if
   header == 0, we might have a normal file with two consecutive
   zeroes. In that case it is difficult to determine, and we continue.
*/
bool FortIO::looks_like_fortran_file(const char *filename, bool endian_flip) {
    std::ifstream stream(filename, std::ios_base::binary);
    if (!stream.is_open())
        throw std::system_error(errno, std::generic_category(),
                                "looks_like_fortran_file: failed to open file");
    bool is_fortran_stream = fortio_is_fortran_stream__(stream, endian_flip);
    return is_fortran_stream;
}

bool FortIO::fclose_stream() {
    if (m_stream.is_open()) {
        m_stream.close();
        return !m_stream.fail();
    } else
        return false; // Already closed.
}

bool FortIO::fopen_stream() {
    if (!m_stream.is_open()) {
        m_stream.clear();
        m_stream.open(m_filename, m_open_mode);
        return m_stream.is_open();
    } else
        return false;
}

bool FortIO::stream_is_open() const { return m_stream.is_open(); }

bool FortIO::assert_stream_open() {
    if (m_stream.is_open())
        return true;
    else {
        fopen_stream();
        return stream_is_open();
    }
}

/**
  This function reads the header (i.e. the number of bytes
  in the following record), stores that internally in the fortio struct, and
  also returns it. If the function fails to read a header (i.e. EOF)
  it will return -1.
*/
int FortIO::init_read() {
    int record_size;
    if (m_stream.read(reinterpret_cast<char *>(&record_size),
                      sizeof record_size)) {
        if (m_endian_flip_header)
            util_endian_flip_vector(&record_size, sizeof record_size, 1);

        return record_size;
    } else {
        m_stream.clear();
        return -1;
    }
}

bool FortIO::data_fskip(size_t element_size, size_t element_count,
                        size_t block_count) {
    offset_type headers = static_cast<offset_type>(block_count) * 4;
    offset_type trailers = static_cast<offset_type>(block_count) * 4;
    offset_type bytes_to_skip =
        headers + trailers +
        (static_cast<offset_type>(element_size) * element_count);

    return fseek(bytes_to_skip, SEEK_CUR);
}

void FortIO::data_fseek(offset_type data_offset, size_t data_element,
                        size_t element_size, int element_count,
                        size_t block_size) {
    if (element_count < 0 || data_element >= static_cast<size_t>(element_count))
        throw std::invalid_argument(
            fmt::format("Element index is out of range: 0 <= {} < {}",
                        data_element, element_count));

    {
        offset_type block_index = data_element / block_size;
        offset_type headers = (block_index + 1) * 4;
        offset_type trailers = block_index * 4;
        offset_type bytes_to_skip =
            data_offset + headers + trailers + (data_element * element_size);

        fseek(bytes_to_skip, SEEK_SET);
    }
}

int FortIO::fclean() {
    offset_type current_pos = m_stream.tellg();
    if (current_pos == static_cast<offset_type>(-1))
        return -1;

    m_stream.flush();
    if (!m_stream)
        return -1;

    m_stream.clear();
    m_stream.seekg(current_pos, std::ios_base::beg);
    m_stream.seekp(current_pos, std::ios_base::beg);
    return m_stream.good() ? 0 : -1;
}

bool FortIO::complete_read(int record_size) {
    int trailer;
    if (m_stream.read(reinterpret_cast<char *>(&trailer), sizeof trailer)) {
        if (m_endian_flip_header)
            util_endian_flip_vector(&trailer, sizeof trailer, 1);

        if (record_size == trailer)
            return true;
    } else
        m_stream.clear();

    return false;
}

/**
   This function fills the buffer with 'buffer_size' bytes from the
   fortio stream. The point of this is to handle the ECLIPSE system with blocks
   of e.g. 1000 floats (which then become one fortran record).
*/
bool FortIO::fread_buffer(char *buffer, int buffer_size) {
    if (buffer == nullptr && buffer_size != 0)
        return false;
    if (buffer_size < 0)
        return false;
    char *end = buffer + buffer_size;
    char *itr = buffer;
    do {
        int record_size = init_read();
        if (record_size < 0)
            return false;
        if (end - itr < static_cast<ptrdiff_t>(record_size))
            return false;
        std::streamsize items_read = 0;
        if (record_size > 0) {
            m_stream.read(itr, record_size);
            items_read = m_stream.gcount();
            if (items_read != record_size)
                m_stream.clear();
        }
        if (items_read != static_cast<std::streamsize>(record_size) ||
            !complete_read(record_size))
            return false;
        itr += record_size;
    } while (itr < end);
    return itr == end;
}

int FortIO::fskip_record() {
    int record_size = init_read();
    fseek((offset_type)record_size, SEEK_CUR);
    complete_read(record_size);
    return record_size;
}

void FortIO::init_write(int record_size) {
    int file_header;
    file_header = record_size;
    if (m_endian_flip_header)
        util_endian_flip_vector(&file_header, sizeof file_header, 1);

    m_stream.write(reinterpret_cast<const char *>(&file_header),
                   sizeof file_header);
}

void FortIO::complete_write(int record_size) {
    int file_header = record_size;
    if (m_endian_flip_header)
        util_endian_flip_vector(&file_header, sizeof file_header, 1);

    m_stream.write(reinterpret_cast<const char *>(&file_header),
                   sizeof file_header);
}

void FortIO::fwrite_record(const char *buffer, int record_size) {
    init_write(record_size);
    m_stream.write(buffer, record_size);
    if (!m_stream)
        throw std::runtime_error(fmt::format(
            "{}: failed to write {} bytes to disk", __func__, record_size));
    complete_write(record_size);
}

offset_type FortIO::ftell() const { return m_stream.tellg(); }

bool FortIO::fseek_(offset_type offset, int whence) {
    m_stream.clear();
    switch (whence) {
    case SEEK_SET:
        m_stream.seekg(offset, std::ios_base::beg);
        m_stream.seekp(offset, std::ios_base::beg);
        break;
    case SEEK_END:
        m_stream.seekg(offset, std::ios_base::end);
        m_stream.seekp(offset, std::ios_base::end);
        break;
    case SEEK_CUR: {
        offset_type target = ftell() + offset;
        m_stream.seekg(target, std::ios_base::beg);
        m_stream.seekp(target, std::ios_base::beg);
        break;
    }
    default:
        return false;
    }
    return m_stream.good();
}

/**
  The semantics of this function depends on the writable flag of the
  fortio structure:

    writable == true: Ordinary fseek() semantics which can potentially
       grow the file.

    writable == false: The function will only seek within the range of
       the file, and fail if you try to seek beyond the EOF marker.
*/
bool FortIO::fseek(offset_type offset, int whence) {
    if (m_writable)
        return fseek_(offset, whence);
    else {
        offset_type new_offset = 0;

        switch (whence) {
        case (SEEK_CUR):
            new_offset = ftell() + offset;
            break;
        case (SEEK_END):
            new_offset = m_read_size + offset;
            break;
        case (SEEK_SET):
            new_offset = offset;
            break;
        default:
            throw std::invalid_argument(
                fmt::format("invalid whence in fortio_fseek: {}", whence));
        }

        if (new_offset <= m_read_size)
            return fseek_(new_offset, SEEK_SET);
        else
            return false;
    }
}

bool FortIO::ftruncate(std::uintmax_t size) {
    if (size >
        static_cast<std::uintmax_t>(std::numeric_limits<std::streamoff>::max()))
        throw std::invalid_argument(
            "Size to ftruncate exceeded std::streamoff size");
    std::streamoff offset = static_cast<std::streamoff>(size);

    if (!m_writable)
        return false;

    // Resize the file on disk directly; no need to close/reopen the stream
    // since resize_file() operates on the path, not the open file handle.
    m_stream.flush();

    std::error_code ec;
    std::filesystem::resize_file(m_filename, size, ec);
    if (ec)
        return false;

    m_stream.clear();

    return fseek(offset, SEEK_SET);
}

/**
  It is undefined behaviour to call this function for a file
  which has been updated; in that case the util_fd_size() function
  will return the size of the file *when it was opened*.
*/
bool FortIO::read_at_eof() {
    if (ftell() == m_read_size)
        return true;
    else
        return false;
}

/**
  When this function is called the underlying file is unlinked, and
  the entry will be removed from the filesystem. Subsequent calls which
  write to this file will still (superficially) succeed.
*/
void FortIO::fwrite_error() {
    if (m_writable)
        std::filesystem::remove(m_filename);
}

void FortIO::fflush() const { m_stream.flush(); }
std::istream &FortIO::get_istream() { return m_stream; }
std::ostream &FortIO::get_ostream() { return m_stream; }
bool FortIO::fmt_file() const { return m_fmt_file; }
void FortIO::rewind() const {
    m_stream.clear();
    m_stream.seekg(0, std::ios_base::beg);
    m_stream.seekp(0, std::ios_base::beg);
}
const char *FortIO::filename_ref() const { return m_filename.c_str(); }

} // namespace ERT
