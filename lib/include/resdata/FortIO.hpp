#pragma once

#include <ios>
#include <string>
#include <cstdlib>
#include <cstdio>

#include <ert/util/util.hpp>
#include <resdata/rd_endian_flip.hpp>

typedef enum {
    FORTIO_NOENTRY = 0, /* File does not exists at all - application error. */
    FORTIO_EOF = 1,     /* The file / record is empty */
    FORTIO_OK =
        2, /* The file / record is OK with: [32 bit header | data | 32 bit footer] */
    FORTIO_MISSING_DATA = 3,
    FORTIO_MISSING_TAIL = 4,
    FORTIO_HEADER_MISMATCH = 5
} fortio_status_type;

namespace ERT {

/**
The fortio class handles fortran io. The problem is
that when a Fortran program writes unformatted data to file in a
statement like:

   integer array(100)
   write(unit) array

it actually writes a head and tail in addition to the actual
data. The header and tail is a 4 byte integer, which value is the
number of bytes in the immediately following record. I.e. what is
actually found on disk after the Fortran code above is:

  | 400 | array ...... | 400 |

Where the "400" head and tail is the number of bytes in the following
record. Fortran IO handles this transparently, but when mixing with
other programming languages care must be taken. This class implements
functionality to read and write these fortran generated files
transparently.
*/
class FortIO {
public:
    FortIO() = delete;
    FortIO(const std::string &filename, std::ios_base::openmode mode,
           bool fmt_file = false, bool endian_flip_header = RD_ENDIAN_FLIP);
    FortIO(const std::string &filename, bool fmt_file, bool writable,
           FILE *stream, bool endian_flip_header = RD_ENDIAN_FLIP);
    ~FortIO();

    FortIO(FortIO &&other) noexcept;
    FortIO &operator=(FortIO &&other) noexcept;
    FortIO(const FortIO &) = delete;
    FortIO &operator=(const FortIO &) = delete;

    static bool looks_like_fortran_file(const char *filename, bool endian_flip);
    void open(const std::string &filename, std::ios_base::openmode mode,
              bool fmt_file = false, bool endian_flip_header = RD_ENDIAN_FLIP);
    void close();

    int init_read();
    bool complete_read(int record_size);
    void init_write(int record_size);
    void complete_write(int record_size);
    int fskip_record();
    bool fread_buffer(char *buffer, int buffer_size);
    void fwrite_record(const char *buffer, int buffer_size);
    [[nodiscard]] FILE *get_FILE() const;
    void fflush() const;
    void rewind() const;
    [[nodiscard]] const char *filename_ref() const;
    [[nodiscard]] const std::string &filename() const { return m_filename; };
    [[nodiscard]] bool fmt_file() const;
    [[nodiscard]] offset_type ftell() const;
    bool fseek(offset_type offset, int whence);
    bool data_fskip(int element_size, int element_count, int block_count);
    void data_fseek(offset_type data_offset, size_t data_element,
                    int element_size, int element_count, int block_size);
    bool ftruncate(offset_type size);
    int fclean();
    bool fclose_stream();
    bool fopen_stream();
    [[nodiscard]] bool stream_is_open() const;
    bool assert_stream_open();
    bool read_at_eof();
    void fwrite_error();

private:
    bool fseek_(offset_type offset, int whence);

    FILE *m_stream = nullptr;
    std::string m_filename;
    bool m_endian_flip_header = false;
    bool m_fmt_file = false;
    const char *m_fopen_mode = nullptr;
    bool m_stream_owner = false;

    /*
    The internal variable m_read_size is used in the functions fseek() and
    read_at_eof() - if-and-only-if - the file is opened in read only mode.

    Observe that the semantics of the fseek() function depends on whether the
    file is writable.
    */
    bool m_writable = false;
    offset_type m_read_size = 0;
};
} // namespace ERT
