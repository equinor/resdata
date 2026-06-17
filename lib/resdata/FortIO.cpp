#include <ios>
#include <stdexcept>
#include <string>
#include <fmt/format.h>

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <ios>
#include <stdexcept>
#include <string>

#include <ert/util/util_unlink.hpp>
#include <ert/util/util.hpp>
#include <ert/util/type_macros.hpp>

#include <resdata/FortIO.hpp>

namespace ERT {

FortIO::FortIO(const std::string &filename, std::ios_base::openmode mode,
               bool fmt_file, bool endian_flip_header) {
    open(filename, mode, fmt_file, endian_flip_header);
}

void FortIO::open(const std::string &filename, std::ios_base::openmode mode,
                  bool fmt_file, bool endian_flip_header) {
    if (mode == std::ios_base::in) {
        if (util_file_exists(filename.c_str())) {
            fortio_type *c_ptr = fortio_open_reader(filename.c_str(), fmt_file,
                                                    endian_flip_header);
            m_fortio.reset(c_ptr);
        } else
            throw std::invalid_argument("File " + filename + " does not exist");
    } else if (mode == std::ios_base::app) {
        fortio_type *c_ptr =
            fortio_open_append(filename.c_str(), fmt_file, endian_flip_header);
        m_fortio.reset(c_ptr);
    } else {
        fortio_type *c_ptr =
            fortio_open_writer(filename.c_str(), fmt_file, endian_flip_header);
        m_fortio.reset(c_ptr);
    }
}

void FortIO::close() {
    if (m_fortio)
        m_fortio.reset();
}

fortio_type *FortIO::get() const { return m_fortio.get(); }

void FortIO::fflush() const { fortio_fflush(m_fortio.get()); }

bool FortIO::ftruncate(offset_type new_size) {
    return fortio_ftruncate(m_fortio.get(), new_size);
}
} // namespace ERT

#define FORTIO_ID 345116

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

static FILE *fortio_fopen_read(const char *filename, bool fmt_file) {
    FILE *stream;
    const char *mode = fortio_fopen_read_mode(fmt_file);
    stream = fopen(filename, mode);
    return stream;
}

static const char *fortio_fopen_write_mode(bool fmt_file) {
    if (fmt_file)
        return WRITE_MODE_TXT;
    else
        return WRITE_MODE_BINARY;
}

static FILE *fortio_fopen_write(const char *filename, bool fmt_file) {
    FILE *stream;
    const char *mode = fortio_fopen_write_mode(fmt_file);
    stream = fopen(filename, mode);
    return stream;
}

static const char *fortio_fopen_readwrite_mode(bool fmt_file) {
    if (fmt_file)
        return READ_WRITE_MODE_TXT;
    else
        return READ_WRITE_MODE_BINARY;
}

static FILE *fortio_fopen_readwrite(const char *filename, bool fmt_file) {
    FILE *stream;
    const char *mode = fortio_fopen_readwrite_mode(fmt_file);
    stream = fopen(filename, mode);
    return stream;
}

static const char *fortio_fopen_append_mode(bool fmt_file) {
    if (fmt_file)
        return APPEND_MODE_TXT;
    else
        return APPEND_MODE_BINARY;
}

static FILE *fortio_fopen_append(const char *filename, bool fmt_file) {
    FILE *stream;
    const char *mode = fortio_fopen_append_mode(fmt_file);
    stream = fopen(filename, mode);
    return stream;
}

struct fortio_struct {
    UTIL_TYPE_ID_DECLARATION;
    FILE *stream;
    char *filename;
    bool endian_flip_header;
    bool
        fmt_file; /* This is not really used by the fortio instance - but it is very convenient to store it here. */
    const char *fopen_mode;
    bool stream_owner;

    /*
    The internal variable read_size is used in the functions
    fortio_fseek() and fortio_read_at_eof() - if-and-only-if - the
    file is opened in read only mode.

    Observe that the semantics of the fortio_fseek() function depends
    on whether the file is writable.
    */
    bool writable;
    offset_type read_size;
    char opts[3];
};

static fortio_type *fortio_alloc__(const char *filename, bool fmt_file,
                                   bool endian_flip_header, bool stream_owner,
                                   bool writable) {
    fortio_type *fortio = (fortio_type *)util_malloc(sizeof *fortio);
    UTIL_TYPE_ID_INIT(fortio, FORTIO_ID);
    fortio->filename = util_alloc_string_copy(filename);
    fortio->endian_flip_header = endian_flip_header;
    fortio->fmt_file = fmt_file;
    fortio->stream_owner = stream_owner;
    fortio->writable = writable;
    fortio->read_size = 0;
    strcpy(fortio->opts, endian_flip_header ? "c" : "ce");

    return fortio;
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
   Helper function for fortio_looks_like_fortran_file(). Checks whether a
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
bool fortio_looks_like_fortran_file(const char *filename, bool endian_flip) {
    FILE *stream = util_fopen(filename, "rb");
    bool is_fortran_stream = fortio_is_fortran_stream__(stream, endian_flip);
    fclose(stream);
    return is_fortran_stream;
}

static int fortio_fileno(fortio_type *fortio) { return fileno(fortio->stream); }

static void fortio_init_size(fortio_type *fortio) {
    fortio->read_size = util_fd_size(fortio_fileno(fortio));
}

fortio_type *fortio_alloc_FILE_wrapper(const char *filename,
                                       bool endian_flip_header, bool fmt_file,
                                       bool writable, FILE *stream) {
    fortio_type *fortio =
        fortio_alloc__(filename, fmt_file, endian_flip_header, false, writable);
    fortio->stream = stream;
    return fortio;
}

fortio_type *fortio_open_reader(const char *filename, bool fmt_file,
                                bool endian_flip_header) {
    FILE *stream = fortio_fopen_read(filename, fmt_file);
    if (stream) {
        fortio_type *fortio =
            fortio_alloc__(filename, fmt_file, endian_flip_header, true, false);
        fortio->stream = stream;
        fortio->fopen_mode = fortio_fopen_read_mode(fmt_file);
        fortio_init_size(fortio);
        return fortio;
    } else
        return NULL;
}

fortio_type *fortio_open_writer(const char *filename, bool fmt_file,
                                bool endian_flip_header) {
    FILE *stream = fortio_fopen_write(filename, fmt_file);
    if (stream) {
        fortio_type *fortio =
            fortio_alloc__(filename, fmt_file, endian_flip_header, true, true);
        fortio->stream = stream;
        fortio->fopen_mode = fortio_fopen_write_mode(fmt_file);
        fortio_init_size(fortio);
        return fortio;
    } else
        return NULL;
}

fortio_type *fortio_open_readwrite(const char *filename, bool fmt_file,
                                   bool endian_flip_header) {
    FILE *stream = fortio_fopen_readwrite(filename, fmt_file);
    if (stream) {
        fortio_type *fortio =
            fortio_alloc__(filename, fmt_file, endian_flip_header, true, true);
        fortio->stream = stream;
        fortio->fopen_mode = fortio_fopen_readwrite_mode(fmt_file);
        fortio_init_size(fortio);
        return fortio;
    } else
        return NULL;
}

fortio_type *fortio_open_append(const char *filename, bool fmt_file,
                                bool endian_flip_header) {
    FILE *stream = fortio_fopen_append(filename, fmt_file);
    if (stream) {
        fortio_type *fortio =
            fortio_alloc__(filename, fmt_file, endian_flip_header, true, true);

        fortio->stream = stream;
        fortio->fopen_mode = fortio_fopen_append_mode(fmt_file);
        fortio_init_size(fortio);

        return fortio;
    } else
        return NULL;
}

bool fortio_fclose_stream(fortio_type *fortio) {
    if (fortio->stream_owner) {
        if (fortio->stream) {
            int fclose_return = fclose(fortio->stream);
            fortio->stream = NULL;
            if (fclose_return == 0)
                return true;
            else
                return false;
        } else
            return false; // Already closed.
    } else
        return false;
}

bool fortio_fopen_stream(fortio_type *fortio) {
    if (fortio->stream == NULL) {
        fortio->stream = fopen(fortio->filename, fortio->fopen_mode);
        if (fortio->stream)
            return true;
        else
            return false;
    } else
        return false;
}

bool fortio_stream_is_open(const fortio_type *fortio) {
    if (fortio->stream)
        return true;
    else
        return false;
}

bool fortio_assert_stream_open(fortio_type *fortio) {
    if (fortio->stream)
        return true;
    else {
        fortio_fopen_stream(fortio);
        return fortio_stream_is_open(fortio);
    }
}

static void fortio_free__(fortio_type *fortio) {
    free(fortio->filename);
    free(fortio);
}

void fortio_free_FILE_wrapper(fortio_type *fortio) { fortio_free__(fortio); }

void fortio_fclose(fortio_type *fortio) {
    if (fortio->stream) {
        fclose(fortio->stream);
        fortio->stream = NULL;
    }

    fortio_free__(fortio);
}

/**
  This function reads the header (i.e. the number of bytes in the
  following record), stores that internally in the fortio struct, and
  also returns it. If the function fails to read a header (i.e. EOF)
  it will return -1.
*/
int fortio_init_read(fortio_type *fortio) {
    int elm_read;
    int record_size;

    elm_read = fread(&record_size, sizeof(record_size), 1, fortio->stream);
    if (elm_read == 1) {
        if (fortio->endian_flip_header)
            util_endian_flip_vector(&record_size, sizeof record_size, 1);

        return record_size;
    } else
        return -1;
}

bool fortio_data_fskip(fortio_type *fortio, const int element_size,
                       const int element_count, const int block_count) {
    offset_type headers = static_cast<offset_type>(block_count) * 4;
    offset_type trailers = static_cast<offset_type>(block_count) * 4;
    offset_type bytes_to_skip =
        headers + trailers +
        (static_cast<offset_type>(element_size) * element_count);

    return fortio_fseek(fortio, bytes_to_skip, SEEK_CUR);
}

void fortio_data_fseek(fortio_type *fortio, offset_type data_offset,
                       size_t data_element, const int element_size,
                       const int element_count, const int block_size) {
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

        fortio_fseek(fortio, bytes_to_skip, SEEK_SET);
    }
}

int fortio_fclean(fortio_type *fortio) {
    long current_pos = ftell(fortio->stream);
    if (current_pos == -1)
        return -1;

    int flush_status = fflush(fortio->stream);
    if (flush_status != 0)
        return flush_status;

    return fseek(fortio->stream, current_pos, SEEK_SET);
}

bool fortio_complete_read(fortio_type *fortio, int record_size) {
    int trailer;
    size_t read_count = fread(&trailer, sizeof trailer, 1, fortio->stream);

    if (read_count == 1) {
        if (fortio->endian_flip_header)
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
bool fortio_fread_buffer(fortio_type *fortio, char *buffer, int buffer_size) {
    if (buffer == nullptr && buffer_size != 0)
        return false;
    if (buffer_size < 0)
        return false;
    char *end = buffer + buffer_size;
    char *itr = buffer;
    do {
        int record_size = fortio_init_read(fortio);
        if (record_size < 0)
            return false;
        if (end - itr < static_cast<ptrdiff_t>(record_size))
            return false;
        size_t items_read = 0;
        if (record_size > 0)
            items_read = fread(itr, 1, record_size, fortio->stream);
        if (items_read != static_cast<size_t>(record_size) ||
            !fortio_complete_read(fortio, record_size))
            return false;
        itr += record_size;
    } while (itr < end);
    return itr == end;
}

int fortio_fskip_record(fortio_type *fortio) {
    int record_size = fortio_init_read(fortio);
    fortio_fseek(fortio, (offset_type)record_size, SEEK_CUR);
    fortio_complete_read(fortio, record_size);
    return record_size;
}

void fortio_init_write(fortio_type *fortio, int record_size) {
    int file_header;
    file_header = record_size;
    if (fortio->endian_flip_header)
        util_endian_flip_vector(&file_header, sizeof file_header, 1);

    util_fwrite_int(file_header, fortio->stream);
}

void fortio_complete_write(fortio_type *fortio, int record_size) {
    int file_header = record_size;
    if (fortio->endian_flip_header)
        util_endian_flip_vector(&file_header, sizeof file_header, 1);

    util_fwrite_int(file_header, fortio->stream);
}

void fortio_fwrite_record(fortio_type *fortio, const char *buffer,
                          int record_size) {
    fortio_init_write(fortio, record_size);
    util_fwrite(buffer, 1, record_size, fortio->stream, __func__);
    fortio_complete_write(fortio, record_size);
}

offset_type fortio_ftell(const fortio_type *fortio) {
    return util_ftell(fortio->stream);
}

static bool fortio_fseek__(fortio_type *fortio, offset_type offset,
                           int whence) {
    int fseek_return = util_fseek(fortio->stream, offset, whence);
    if (fseek_return == 0)
        return true;
    else
        return false;
}

/**
  The semantics of this function depends on the readable flag of the
  fortio structure:

    writable == true: Ordinary fseek() semantics which can potentially
       grow the file.

    writable == false: The function will only seek within the range of
       the file, and fail if you try to seek beyond the EOF marker.

*/
bool fortio_fseek(fortio_type *fortio, offset_type offset, int whence) {
    if (fortio->writable)
        return fortio_fseek__(fortio, offset, whence);
    else {
        offset_type new_offset = 0;

        switch (whence) {
        case (SEEK_CUR):
            new_offset = fortio_ftell(fortio) + offset;
            break;
        case (SEEK_END):
            new_offset = fortio->read_size + offset;
            break;
        case (SEEK_SET):
            new_offset = offset;
            break;
        default:
            throw std::invalid_argument(
                fmt::format("invalid whence in fortio_fseek: {}", whence));
        }

        if (new_offset <= fortio->read_size)
            return fortio_fseek__(fortio, new_offset, SEEK_SET);
        else
            return false;
    }
}

bool fortio_ftruncate(fortio_type *fortio, offset_type size) {
    fortio_fseek(fortio, size, SEEK_SET);
    return util_ftruncate(fortio->stream, size);
}

/**
  It is undefined behaviour to call this function for a file
  which has been updated; in that case the util_fd_size() function
  will return the size of the file *when it was opened*.
*/
bool fortio_read_at_eof(fortio_type *fortio) {

    if (fortio_ftell(fortio) == fortio->read_size)
        return true;
    else
        return false;
}

/**
  When this function is called the underlying file is unlinked, and
  the entry will be removed from the filesystem. Subsequent calls which
  write to this file will still (superficially) succeed.
*/
void fortio_fwrite_error(fortio_type *fortio) {
    if (fortio->writable)
        util_unlink(fortio->filename);
}

void fortio_fflush(fortio_type *fortio) { fflush(fortio->stream); }
FILE *fortio_get_FILE(const fortio_type *fortio) { return fortio->stream; }
bool fortio_fmt_file(const fortio_type *fortio) { return fortio->fmt_file; }
void fortio_rewind(const fortio_type *fortio) { util_rewind(fortio->stream); }
const char *fortio_filename_ref(const fortio_type *fortio) {
    return fortio->filename;
}
