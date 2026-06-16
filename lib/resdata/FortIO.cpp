#include <ios>
#include <stdexcept>
#include <string>
#include <utility>
#include <fmt/format.h>

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <ert/util/util_unlink.hpp>
#include <ert/util/util.hpp>

#include <resdata/FortIO.hpp>

#define READ_MODE_TXT "r"
#define READ_MODE_BINARY "rb"
#define WRITE_MODE_TXT "w"
#define WRITE_MODE_BINARY "wb"
#define READ_WRITE_MODE_TXT "r+"
#define READ_WRITE_MODE_BINARY "r+b"
#define APPEND_MODE_TXT "a"
#define APPEND_MODE_BINARY "ab"

/*
  Observe that the stream open functions accept a failure, and call
  the fopen() function directly.
*/

static const char *fortio_fopen_read_mode(bool fmt_file) {
    if (fmt_file)
        return READ_MODE_TXT;
    else
        return READ_MODE_BINARY;
}

static const char *fortio_fopen_write_mode(bool fmt_file) {
    if (fmt_file)
        return WRITE_MODE_TXT;
    else
        return WRITE_MODE_BINARY;
}

static const char *fortio_fopen_readwrite_mode(bool fmt_file) {
    if (fmt_file)
        return READ_WRITE_MODE_TXT;
    else
        return READ_WRITE_MODE_BINARY;
}

static const char *fortio_fopen_append_mode(bool fmt_file) {
    if (fmt_file)
        return APPEND_MODE_TXT;
    else
        return APPEND_MODE_BINARY;
}

/**
   Helper function for fortio_is_fortran_stream__().
*/
static bool __read_int(FILE *stream, int *value, bool endian_flip) {
    /* This fread() can fail - can not use util_fread() here. */
    if (fread(value, sizeof *value, 1, stream) == 1) {
        if (endian_flip)
            util_endian_flip_vector(value, sizeof *value, 1);
        return true;
    } else
        return false;
}

/**
   Helper function for FortIO::looks_like_fortran_file(). Checks whether a
   particular stream is formatted according to fortran io, for a fixed
   endianness.
*/
static bool fortio_is_fortran_stream__(FILE *stream, bool endian_flip) {
    const bool strict_checking =
        true; /* True: requires that *ALL* records in the file are fortran formatted */
    offset_type init_pos = util_ftell(stream);
    bool is_fortran_stream = false;
    int header, tail;
    bool cont;

    do {
        cont = false;
        if (__read_int(stream, &header, endian_flip)) {
            if (header >= 0) {
                if (util_fseek(stream, (offset_type)header, SEEK_CUR) == 0) {
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
    util_fseek(stream, init_pos, SEEK_SET);
    return is_fortran_stream;
}

namespace ERT {

FortIO::FortIO(const std::string &filename, std::ios_base::openmode mode,
               bool fmt_file, bool endian_flip_header) {
    open(filename, mode, fmt_file, endian_flip_header);
}

FortIO::FortIO(const std::string &filename, bool fmt_file, bool writable,
               FILE *stream, bool endian_flip_header) {
    m_filename = filename;
    m_endian_flip_header = endian_flip_header;
    m_fmt_file = fmt_file;
    m_stream_owner = false;
    m_writable = writable;
    m_read_size = 0;
    m_stream = stream;
}

FortIO::~FortIO() { close(); }

FortIO::FortIO(FortIO &&other) noexcept
    : m_stream(std::exchange(other.m_stream, nullptr)),
      m_filename(std::move(other.m_filename)),
      m_endian_flip_header(std::exchange(other.m_endian_flip_header, false)),
      m_fmt_file(std::exchange(other.m_fmt_file, false)),
      m_fopen_mode(std::exchange(other.m_fopen_mode, nullptr)),
      m_stream_owner(std::exchange(other.m_stream_owner, false)),
      m_writable(std::exchange(other.m_writable, false)),
      m_read_size(std::exchange(other.m_read_size, 0)) {
    other.m_filename = "";
}

FortIO &FortIO::operator=(FortIO &&other) noexcept {
    if (this == &other)
        return *this;

    close();

    m_stream = std::exchange(other.m_stream, nullptr);
    m_filename = std::move(other.m_filename);
    m_endian_flip_header = std::exchange(other.m_endian_flip_header, false);
    m_fmt_file = std::exchange(other.m_fmt_file, false);
    m_fopen_mode = std::exchange(other.m_fopen_mode, nullptr);
    m_stream_owner = std::exchange(other.m_stream_owner, false);
    m_writable = std::exchange(other.m_writable, false);
    m_read_size = std::exchange(other.m_read_size, 0);

    other.m_filename = "";

    return *this;
}

void FortIO::open(const std::string &filename, std::ios_base::openmode mode,
                  bool fmt_file, bool endian_flip_header) {
    const char *cmode;
    if (mode == (std::ios_base::in | std::ios_base::out)) {
        cmode = fortio_fopen_readwrite_mode(fmt_file);
    } else if (mode == std::ios_base::in) {
        if (util_file_exists(filename.c_str())) {
            cmode = fortio_fopen_read_mode(fmt_file);
        } else
            throw std::ios_base::failure("File " + filename +
                                         " does not exist");
    } else if (mode == std::ios_base::app) {
        cmode = fortio_fopen_append_mode(fmt_file);
    } else {
        cmode = fortio_fopen_write_mode(fmt_file);
    }

    FILE *stream = fopen(filename.c_str(), cmode);
    if (!stream)
        throw std::ios_base::failure("Failed to open FortIO file " + filename);
    m_filename = filename;
    m_endian_flip_header = endian_flip_header;
    m_fmt_file = fmt_file;
    m_stream_owner = true;
    m_writable = (mode & std::ios_base::out) || (mode & std::ios_base::app);
    m_read_size = 0;
    m_stream = stream;
    m_fopen_mode = cmode;
    m_read_size = util_fd_size(fileno(m_stream));
}

void FortIO::close() {
    if (m_stream && m_stream_owner)
        fclose(m_stream);
    m_stream = nullptr;
    m_filename = "";
    m_fopen_mode = nullptr;
    m_stream_owner = false;
    m_writable = false;
    m_read_size = 0;
}

FortIO *FortIO::get() const {
    return const_cast<FortIO *>(this);
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
    FILE *stream = util_fopen(filename, "rb");
    bool is_fortran_stream = fortio_is_fortran_stream__(stream, endian_flip);
    fclose(stream);
    return is_fortran_stream;
}

bool FortIO::fclose_stream() {
    if (m_stream_owner) {
        if (m_stream) {
            int fclose_return = fclose(m_stream);
            m_stream = nullptr;
            if (fclose_return == 0)
                return true;
            else
                return false;
        } else
            return false; // Already closed.
    } else
        return false;
}

bool FortIO::fopen_stream() {
    if (m_stream == nullptr) {
        m_stream = fopen(m_filename.c_str(), m_fopen_mode);
        if (m_stream)
            return true;
        else
            return false;
    } else
        return false;
}

bool FortIO::stream_is_open() const {
    if (m_stream)
        return true;
    else
        return false;
}

bool FortIO::assert_stream_open() {
    if (m_stream)
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
    int elm_read;
    int record_size;

    elm_read = fread(&record_size, sizeof(record_size), 1, m_stream);
    if (elm_read == 1) {
        if (m_endian_flip_header)
            util_endian_flip_vector(&record_size, sizeof record_size, 1);

        return record_size;
    } else
        return -1;
}

bool FortIO::data_fskip(int element_size, int element_count, int block_count) {
    offset_type headers = static_cast<offset_type>(block_count) * 4;
    offset_type trailers = static_cast<offset_type>(block_count) * 4;
    offset_type bytes_to_skip =
        headers + trailers +
        (static_cast<offset_type>(element_size) * element_count);

    return fseek(bytes_to_skip, SEEK_CUR);
}

void FortIO::data_fseek(offset_type data_offset, size_t data_element,
                        int element_size, int element_count, int block_size) {
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
    long current_pos = ::ftell(m_stream);
    if (current_pos == -1)
        return -1;

    int flush_status = ::fflush(m_stream);
    if (flush_status != 0)
        return flush_status;

    return ::fseek(m_stream, current_pos, SEEK_SET);
}

bool FortIO::complete_read(int record_size) {
    int trailer;
    size_t read_count = fread(&trailer, sizeof trailer, 1, m_stream);

    if (read_count == 1) {
        if (m_endian_flip_header)
            util_endian_flip_vector(&trailer, sizeof trailer, 1);

        if (record_size == trailer)
            return true;
    }

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
        size_t items_read = 0;
        if (record_size > 0)
            items_read = fread(itr, 1, record_size, m_stream);
        if (items_read != static_cast<size_t>(record_size) ||
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

    util_fwrite_int(file_header, m_stream);
}

void FortIO::complete_write(int record_size) {
    int file_header = record_size;
    if (m_endian_flip_header)
        util_endian_flip_vector(&file_header, sizeof file_header, 1);

    util_fwrite_int(file_header, m_stream);
}

void FortIO::fwrite_record(const char *buffer, int record_size) {
    init_write(record_size);
    util_fwrite(buffer, 1, record_size, m_stream, __func__);
    complete_write(record_size);
}

offset_type FortIO::ftell() const { return util_ftell(m_stream); }

bool FortIO::fseek_(offset_type offset, int whence) {
    int fseek_return = util_fseek(m_stream, offset, whence);
    if (fseek_return == 0)
        return true;
    else
        return false;
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

bool FortIO::ftruncate(offset_type size) {
    fseek(size, SEEK_SET);
    return util_ftruncate(m_stream, size);
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
        util_unlink(m_filename.c_str());
}

void FortIO::fflush() const { ::fflush(m_stream); }
FILE *FortIO::get_FILE() const { return m_stream; }
bool FortIO::fmt_file() const { return m_fmt_file; }
void FortIO::rewind() const { util_rewind(m_stream); }
const char *FortIO::filename_ref() const { return m_filename.c_str(); }

} // namespace ERT
