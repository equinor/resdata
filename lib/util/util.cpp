/**
  This file contains a large number of utility functions for memory
  handling, string handling and file handling. Observe that all these
  functions are just that - functions - there is no associated state
  with any of these functions.

  The file util_path.c is included in this, and contains path
  manipulation functions which explicitly use the PATH_SEP variable.
*/

#include <algorithm>
#include <filesystem>
#include <ios>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#include <cassert>
#include <cstring>
#include <ctime>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <fmt/format.h>

#include <fcntl.h>
#include <climits>

#include <ert/util/ert_api_config.hpp>
#include "ert/util/build_config.hpp"

#include <cerrno>
#include <system_error>

#include <cstdint>
#include <cctype>
#include <cstdlib>
#include <string_view>
#include <sys/stat.h>

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Shlwapi.h>
#endif

#ifdef ERT_HAVE_SPAWN
#include <unistd.h>
#include <sys/wait.h>
#endif

#ifdef HAVE__USLEEP
#include <unistd.h>
#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef HAVE_FTRUNCATE
#include <unistd.h>
#include <sys/types.h>
#else
#include <io.h>
#endif

#ifdef HAVE_WINDOWS_GETCWD
#include <direct.h>
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#endif

#include <cstddef>
#include <cstdint>

#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>

#include <memory>

/* The endian_convert() overloads produce the byte reversed value of a fixed
   width unsigned integer, and flip_elements() applies one of them to every
   element of a buffer. */

static uint16_t endian_convert(uint16_t u) {
    return static_cast<uint16_t>(((u >> 8U) & 0x00FFU) | ((u << 8U) & 0xFF00U));
}

static uint32_t endian_convert(uint32_t u) {
    const uint32_t m8 = (uint32_t)0x00FF00FFUL;
    const uint32_t m16 = (uint32_t)0x0000FFFFUL;

    u = ((u >> 8U) & m8) | ((u & m8) << 8U);
    u = ((u >> 16U) & m16) | ((u & m16) << 16U);
    return u;
}

static uint64_t endian_convert(uint64_t u) {
    const uint64_t m8 = (uint64_t)0x00FF00FF00FF00FFULL;
    const uint64_t m16 = (uint64_t)0x0000FFFF0000FFFFULL;
    const uint64_t m32 = (uint64_t)0x00000000FFFFFFFFULL;

    u = ((u >> 8U) & m8) | ((u & m8) << 8U);
    u = ((u >> 16U) & m16) | ((u & m16) << 16U);
    u = ((u >> 32U) & m32) | ((u & m32) << 32U);
    return u;
}

/** Byte reverses two adjacent 32 bit values held in one 64 bit word.

    Identical to applying the 32 bit conversion to each half, but in half the
    number of operations; this measurably halves the cost of flipping a vector
    of 4 byte elements, which is the common case for restart format files. */
static uint64_t endian_convert_pair32(uint64_t u) {
    const uint64_t m8 = (uint64_t)0x00FF00FF00FF00FFULL;
    const uint64_t m16 = (uint64_t)0x0000FFFF0000FFFFULL;

    u = ((u >> 8U) & m8) | ((u & m8) << 8U);
    u = ((u >> 16U) & m16) | ((u & m16) << 16U);
    return u;
}

template <typename T> static void flip_elements(void *data, size_t elements) {
    std::byte *bytes = static_cast<std::byte *>(data);

    for (size_t i = 0; i < elements; i++) {
        T value;
        std::memcpy(&value, bytes + i * sizeof(T), sizeof(T));
        value = endian_convert(value);
        std::memcpy(bytes + i * sizeof(T), &value, sizeof(T));
    }
}

/** Flips 4 byte elements two at a time, with the odd last element, if any,
    flipped on its own. */
static void flip_elements32(void *data, size_t elements) {
    std::byte *bytes = static_cast<std::byte *>(data);
    size_t i = 0;

    for (; i + 1 < elements; i += 2) {
        uint64_t pair;
        std::memcpy(&pair, bytes + i * sizeof(uint32_t), sizeof(pair));
        pair = endian_convert_pair32(pair);
        std::memcpy(bytes + i * sizeof(uint32_t), &pair, sizeof(pair));
    }

    if (i < elements) {
        uint32_t value;
        std::memcpy(&value, bytes + i * sizeof(value), sizeof(value));
        value = endian_convert(value);
        std::memcpy(bytes + i * sizeof(value), &value, sizeof(value));
    }
}

void util_endian_flip_vector(void *data, size_t element_size, size_t elements) {
    switch (element_size) {
    case (1):
        break;
    case (2):
        flip_elements<uint16_t>(data, elements);
        break;
    case (4):
        flip_elements32(data, elements);
        break;
    case (8):
        flip_elements<uint64_t>(data, elements);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("can only endian flip 1/2/4/8 byte variables - got {}",
                        element_size));
    }
}

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

/**
   Kahan summation is a technique to retain numerical precision when
   summing a long vector of values. See:
   http://en.wikipedia.org/wiki/Kahan_summation_algorithm
*/

double util_kahan_sum(const double *data, size_t N) {
    double S = data[0];
    double C = 0;
    double Y, T;
    size_t i;

    for (i = 1; i < N; i++) {
        Y = data[i] - C;
        T = S + Y;
        C = (T - S) - Y;
        S = T;
    }
    return S;
}

bool util_float_approx_equal__(float d1, float d2, float rel_eps,
                               float abs_eps) {
    if ((fabsf(d1) + fabsf(d2)) == 0)
        return true;
    else {
        float diff = fabsf(d1 - d2);
        if ((abs_eps > 0) && (diff > abs_eps))
            return false;
        {
            float sum = fabsf(d1) + fabsf(d2);
            float rel_diff = diff / sum;

            if ((rel_eps > 0) && (rel_diff > rel_eps))
                return false;
        }
        return true;
    }
}

/*
  If an epsilon value is identically equal to zero that comparison
  will be ignored.
*/

bool util_double_approx_equal__(double d1, double d2, double rel_eps,
                                double abs_eps) {
    if ((fabs(d1) + fabs(d2)) == 0)
        return true;
    else {
        double diff = fabs(d1 - d2);
        if ((abs_eps > 0) && (diff > abs_eps))
            return false;
        {
            double sum = fabs(d1) + fabs(d2);
            double rel_diff = diff / sum;

            if ((rel_eps > 0) && (rel_diff > rel_eps))
                return false;
        }
        return true;
    }
}

bool util_double_approx_equal(double d1, double d2) {
    double epsilon = 1e-6;
    return util_double_approx_equal__(d1, d2, epsilon, 0.0);
}

char *util_alloc_string_copy(const char *src) {
    if (src != NULL) {
        size_t byte_size = (strlen(src) + 1) * sizeof *src;
        char *copy = rd::checked_calloc<char>(byte_size).release();
        memcpy(copy, src, byte_size);
        return copy;
    } else
        return NULL;
}

bool util_fgetc_while_equal(FILE *stream, std::string_view str,
                            bool case_sensitive) {
    if (str.empty())
        return true;
    if (!stream) {
        return false;
    }
    offset_type current_pos = util_ftell(stream);
    if (current_pos == (offset_type)-1) {
        return false;
    }

    bool equal = true;
    for (size_t string_index = 0; string_index < str.length(); ++string_index) {
        int c = std::fgetc(stream);

        if (c == EOF) {
            equal = false;
            break;
        }

        unsigned char target_char =
            static_cast<unsigned char>(str[string_index]);
        unsigned char stream_char = static_cast<unsigned char>(c);

        if (!case_sensitive) {
            stream_char = static_cast<unsigned char>(std::toupper(stream_char));
            target_char = static_cast<unsigned char>(std::toupper(target_char));
        }
        if (stream_char != target_char) {
            equal = false;
            break;
        }
    }

    if (!equal) { /* not equal - go back. */
        if (util_fseek(stream, current_pos, SEEK_SET) != 0)
            throw std::system_error(errno, std::generic_category(),
                                    "util_fgetc_while_equal: Failed to seek "
                                    "back");
    }

    return equal;
}

/**
   This function will reposition the stream pointer at the the first
   occurence of 'string'. If 'string' is found the function will
   return true, otherwise the function will return false, and stream
   pointer will be at the original position.

   If skip_string == true the stream position will be positioned
   immediately after the 'string', otherwise it will be positioned at
   the beginning of 'string'.

   If case_sensitive is true we require exact match with respect to
   case, otherwise we accept any case combination.
*/

bool util_fseek_string(FILE *stream, std::string_view string, bool skip_string,
                       bool case_sensitive) {
    /* Searching for the empty string succeeds immediately, without moving the
       stream position. */
    if (string.empty())
        return true;

    bool string_found = false;
    int string_start = static_cast<unsigned char>(string[0]);
    if (!case_sensitive)
        string_start = std::toupper(string_start);
    std::string_view string_rest = string.substr(1);

    offset_type initial_pos =
        util_ftell(stream); /* Store the inital position. */
    int c;
    do {
        c = fgetc(stream);
        if (!case_sensitive && c != EOF)
            c = std::toupper(c);

        if (c == string_start) {
            /* we got the first character right - lets try in more detail: */
            string_found =
                util_fgetc_while_equal(stream, string_rest, case_sensitive);
        }
    } while (c != EOF && !string_found);

    if (string_found) {
        if (!skip_string) {
            offset_type offset = (offset_type)string.size();
            util_fseek(stream, -offset,
                       SEEK_CUR); /* Reposition to the beginning of 'string' */
        }
    } else
        util_fseek(
            stream, initial_pos,
            SEEK_SET); /* Could not find the string reposition at initial position. */
    return string_found;
}

char *util_fscanf_alloc_line(FILE *stream, bool *at_eof) {
    offset_type init_pos = util_ftell(stream);
    int len;
    int end_char;
    bool cont;
    bool dos_newline;

    len = 0;
    cont = true;
    {
        int c;
        do {
            c = fgetc(stream);
            if (c == EOF)
                cont = false;
            else {
                if (c == '\r' || c == '\n')
                    cont = false;
                else
                    len++;
            }
        } while (cont);
        if (c == '\r')
            dos_newline = true;
        else
            dos_newline = false;
        end_char = c;
    }

    if (util_fseek(stream, init_pos, SEEK_SET) != 0)
        throw std::ios_base::failure(fmt::format(
            "{}: fseek failed: {}/{}", __func__, errno, strerror(errno)));

    auto new_line = rd::checked_calloc<char>(len + 1);
    util_fread(new_line.get(), sizeof(char), len, stream, __func__);
    new_line.get()[len] = '\0';

    /*
    Skipping the end of line marker(s).
  */

    fgetc(stream);
    if (dos_newline)
        fgetc(stream);

    if (at_eof != NULL) {
        if (end_char == EOF)
            *at_eof = true;
        else
            *at_eof = false;
    }

    return util_alloc_strip_copy(new_line.get());
}

/**
   Takes a char buffer as input, and parses it as an integer. Returns
   true if the parsing succeeded, and false otherwise. If parsing
   succeeded, the integer value is returned by reference.
*/
bool util_sscanf_int(const char *buffer, int *value) {
    if (!buffer)
        return false;

    char *error_ptr;

    errno = 0;
    long tmp_value = strtol(buffer, &error_ptr, 10);

    if (error_ptr == buffer)
        return false;

    // ERANGE signals overflow; on Windows long is 32 bit so that is the only
    // way to detect it. Any other errno is also treated as a failed parse.
    if (errno != 0 || tmp_value < INT_MIN || tmp_value > INT_MAX)
        return false;

    // Skip trailing white-space
    while (error_ptr[0] != '\0' && isspace(error_ptr[0]))
        error_ptr++;

    if (error_ptr[0] != '\0')
        return false;

    if (value != NULL)
        *value = static_cast<int>(tmp_value);
    return true;
}

/*
  If any (or both) s1 or s2 is equal to NULL - the function will
  return false.
*/
bool util_string_equal(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL)
        return false;

    /* Now we know that BOTH s1 and s2 are != NULL */
    if (strcmp(s1, s2) == 0)
        return true;
    else
        return false;
}

/*
  Windows *might* have both the symbols _access() and access(), but we prefer
  the _access() symbol as that seems to be preferred by Windows. We therefor do
  the #HAVE_WINDOWS__ACCESS check first.
*/

#ifdef HAVE_WINDOWS__ACCESS

bool util_access(const char *entry, int mode) {
    return (_access(entry, mode) == 0);
}

#else

#ifdef HAVE_POSIX_ACCESS
bool util_access(const char *entry, mode_t mode) {
    return (access(entry, mode) == 0);
}
#endif

#endif

/**
   Currently only checks if the entry exists - this will return true
   if path points to directory.
*/
bool util_file_exists(const char *filename) {
    return util_entry_exists(filename);
}

/**
   Checks if there is an entry in the filesystem - i.e.  ordinary
   file, directory , symbolic link, ... with name 'entry'; and returns
   true/false accordingly.
*/

bool util_entry_exists(const char *entry) { return util_access(entry, F_OK); }

size_t util_file_size(const char *file) {
    size_t file_size;

    {
        int fildes = open(file, O_RDONLY);
        if (fildes == -1)
            throw std::ios_base::failure(fmt::format(
                "{}: failed to open:{} - {}", __func__, file, strerror(errno)));

        file_size = util_fd_size(fildes);
        close(fildes);
    }

    return file_size;
}

size_t util_fd_size(int fd) {
    stat_type buffer;

    util_fstat(fd, &buffer);

    return buffer.st_size;
}

bool util_ftruncate(FILE *stream, offset_type size) {
    int fd = fileno(stream);
    int int_return;

#ifdef HAVE_FTRUNCATE
    int_return = ftruncate(fd, size);
#else
    int_return = _chsize_s(fd, size);
#endif

    if (int_return == 0)
        return true;
    else
        return false;
}

bool util_same_file(const char *file1, const char *file2) {
    // Symlinks are followed. Nonexistent files are never equal.
    std::error_code ec;
    return std::filesystem::equivalent(file1, file2, ec);
}

static bool util_fmt_bit8_stream(FILE *stream) {
    const size_t min_read = 256; /* Critically small */
    const double bit8set_limit = 0.00001;
    const size_t buffer_size = 131072;
    offset_type start_pos = util_ftell(stream);
    bool fmt_file;
    {
        double bit8set_fraction;
        int N_bit8set = 0;
        size_t elm_read;
        std::vector<char> buffer(buffer_size);

        elm_read = fread(buffer.data(), 1, buffer_size, stream);
        if (elm_read < min_read)
            throw std::invalid_argument(
                fmt::format("{}: file is too small to automatically determine "
                            "formatted/unformatted status",
                            __func__));

        for (size_t i = 0; i < elm_read; i++)
            N_bit8set += (buffer[i] & (1 << 7)) >> 7;

        bit8set_fraction =
            static_cast<double>(N_bit8set) / static_cast<double>(elm_read);
        if (bit8set_fraction < bit8set_limit)
            fmt_file = true;
        else
            fmt_file = false;
    }
    util_fseek(stream, start_pos, SEEK_SET);
    return fmt_file;
}

bool util_fmt_bit8(const char *filename) {
    if (!util_file_exists(filename))
        throw std::ios_base::failure(
            fmt::format("{}: could not find file: {}", __func__, filename));

    std::unique_ptr<FILE, void (*)(FILE *)> stream{util_fopen(filename, "rb"),
                                                   [](FILE *f) { fclose(f); }};
    return util_fmt_bit8_stream(stream.get());
}

/*
  time_t        st_atime;    time of last access
  time_t        st_mtime;    time of last modification
  time_t        st_ctime;    time of last status change
*/

/*
  The returned value is the difference in file mtime from the file2 to
  file1 (what a mess). I.e. if file1 is newer than file2 the returned
  value will be positive.
*/

double util_file_difftime(const char *file1, const char *file2) {
    stat_type b1, b2;
    int f1, f2;
    time_t t1, t2;

    f1 = open(file1, O_RDONLY);
    util_fstat(f1, &b1);
    t1 = b1.st_mtime;
    close(f1);

    f2 = open(file2, O_RDONLY);
    util_fstat(f2, &b2);
    t2 = b2.st_mtime;
    close(f2);

    return difftime(t1, t2);
}

static void __util_set_timevalues_utc(time_t t, int *sec, int *min, int *hour,
                                      int *mday, int *month, int *year) {
    struct tm ts;

    util_time_utc(&t, &ts);
    if (sec != NULL)
        *sec = ts.tm_sec;
    if (min != NULL)
        *min = ts.tm_min;
    if (hour != NULL)
        *hour = ts.tm_hour;
    if (mday != NULL)
        *mday = ts.tm_mday;
    if (month != NULL)
        *month = ts.tm_mon + 1;
    if (year != NULL)
        *year = ts.tm_year + 1900;
}

static bool util_make_datetime_utc__(int sec, int min, int hour, int mday,
                                     int month, int year, bool force_set,
                                     time_t *t);

/*
  This function takes a time_t instance as input, and
  returns the the time broken down in sec:min:hour  mday:month:year.

  The return values are by pointers - you can pass in NULL to any of
  the fields.
*/

/*
  This function takes a time_t instance as input, and
  returns the the time broken down in sec:min:hour  mday:month:year.

  The return values are by pointers - you can pass in NULL to any of
  the fields.
*/
void util_set_datetime_values_utc(time_t t, int *sec, int *min, int *hour,
                                  int *mday, int *month, int *year) {
    __util_set_timevalues_utc(t, sec, min, hour, mday, month, year);
}

/*
   The date format is HARD assumed to be

   dd/mm/yyyy

   Where mm is [1..12]
   yyyy ~2000

*/

void util_inplace_forward_seconds_utc(time_t *t, double seconds) {
    *t = static_cast<time_t>(static_cast<double>(*t) + seconds);
}

void util_inplace_forward_days_utc(time_t *t, double days) {
    util_inplace_forward_seconds_utc(t, days * 3600 * 24);
}

double util_difftime_seconds(time_t start_time, time_t end_time) {
    double dt = difftime(end_time, start_time);
    return dt;
}

/*
  Observe that this routine does the following transform before calling mktime:

  1. month -> month - 1;
  2. year  -> year  - 1900;

  Then it is on the format which mktime expects.

*/

char *util_get_timezone() {
#if defined(HAVE_TZNAME)
    return tzname[0];
#elif defined(HAVE_WINDOWS_TZNAME)
    return _tzname[0];
#endif
}

/*
  The underlying timegm() function will happily accept dates like
  December 33.th 2012 - which is wrapped around to 2.nd of January
  2013. Such wrap-arounds are not accepted by this function, which
  will return false in that case.

  The time_t output is by reference, and will be set to the return
  value of timegm() irrespective of the true/false return value.
*/

static bool util_make_datetime_utc__(int sec, int min, int hour, int mday,
                                     int month, int year, bool force_set,
                                     time_t *t) {
    bool valid = false;
    struct tm ts;
    ts.tm_sec = sec;
    ts.tm_min = min;
    ts.tm_hour = hour;
    ts.tm_mday = mday;
    ts.tm_mon = month - 1;
    ts.tm_year = year - 1900;
    ts.tm_isdst =
        -1; /* Negative value means mktime tries to determine automagically whether Daylight Saving Time is in effect. */
    {

#ifdef HAVE_TIMEGM
        time_t work_t = timegm(&ts);
#else
        time_t work_t = _mkgmtime(&ts);
#endif

        if ((ts.tm_sec == sec) && (ts.tm_min == min) && (ts.tm_hour == hour) &&
            (ts.tm_mday == mday) && (ts.tm_mon == (month - 1)) &&
            (ts.tm_year == (year - 1900)))
            valid = true;

        if (t) {
            if (valid || force_set)
                *t = work_t;
        }
    }
    return valid;
}

time_t util_make_datetime_utc(int sec, int min, int hour, int mday, int month,
                              int year) {
    time_t t;
    util_make_datetime_utc__(sec, min, hour, mday, month, year, true, &t);
    return t;
}

time_t util_make_date_utc(int mday, int month, int year) {
    return util_make_datetime_utc(0, 0, 0, mday, month, year);
}

/**
   The function will allocate a new copy of src where leading and
   trailing whitespace has been stripped off. If the source string is
   all blanks a string of length one - only containing \0 is returned,
   i.e. not NULL.

   If src is NULL the function will return NULL. The incoming source
   string is not modified.
*/

char *util_alloc_strip_copy(const char *src) {
    size_t end_index = strlen(src);
    while (end_index > 0 && src[end_index - 1] == ' ')
        end_index--;

    // end_index is now one past the last non-space character
    size_t start_index = 0;
    while (start_index < end_index && src[start_index] == ' ')
        start_index++;

    size_t strip_length = end_index - start_index;
    char *target = rd::checked_calloc<char>(strip_length + 1).release();
    memcpy(target, &src[start_index], strip_length);
    target[strip_length] = '\0';
    return target;
}

void util_fwrite_int(int value, FILE *stream) {
    if (fwrite(&value, sizeof value, 1, stream) != 1)
        throw std::ios_base::failure(
            fmt::format("{}: write failed: {}", __func__, strerror(errno)));
}

FILE *util_fopen__(const char *filename, const char *mode) {
    return fopen(filename, mode);
}

FILE *util_fopen(const char *filename, const char *mode) {
    FILE *stream = util_fopen__(filename, mode);
    if (stream == NULL)
        throw std::ios_base::failure(
            fmt::format("{}: failed to open:{} with mode:'{}' - error:{}({})",
                        __func__, filename, mode, strerror(errno), errno));

    return stream;
}

void util_fwrite(const void *ptr, size_t element_size, size_t items,
                 FILE *stream, const char *caller) {
    if (!ptr)
        return;
    size_t items_written = fwrite(ptr, element_size, items, stream);
    if (items_written != items)
        throw std::ios_base::failure(fmt::format(
            "{}/{}: only wrote {}/{} items to disk: {}({})", caller, __func__,
            items_written, items, strerror(errno), errno));
}

void util_fread(void *ptr, size_t element_size, size_t items, FILE *stream,
                const char *caller) {
    size_t items_read = fread(ptr, element_size, items, stream);
    if (items_read != items)
        throw std::ios_base::failure(fmt::format(
            "{}/{}: only read {}/{} items from disk: {}({})", caller, __func__,
            items_read, items, strerror(errno), errno));
}

/**
   This function allocates a string acoording to the fmt
   specification, and arguments. The arguments (except the format) are
   entered as a variable length argument list, and the function is
   basically a thin wrapper around vsnprintf().

   Example of usage:

   char * s = util_alloc_sprintf("/%s/File:%04d/%s" , "prefix" , 67 , "Suffix");

   => s = /prefix/File:0067/Suffix

   Observe that when it is based in vsnprintf() essentially no
   error-checking is performed.
*/

char *util_alloc_sprintf_va(const char *fmt, va_list ap) {
    char *s = NULL;
    int length;
    va_list tmp_va;

    UTIL_VA_COPY(tmp_va, ap);

    length = vsnprintf(NULL, 0, fmt, tmp_va);
    va_end(tmp_va);
    s = rd::checked_calloc<char>(length + 1).release();
    vsnprintf(s, length + 1, fmt, ap);
    return s;
}

char *util_alloc_sprintf(const char *fmt, ...) {
    char *s;
    va_list ap;
    va_start(ap, fmt);
    try {
        s = util_alloc_sprintf_va(fmt, ap);
    } catch (...) {
        va_end(ap);
        throw;
    }
    va_end(ap);
    return s;
}

void util_exit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

/**
   This function finds the current linenumber (by counting '\n'
   characters) in a currently open stream. It is an extremely
   primitive routine, and should only be used exceptionally.

   Observe that it implements "natural" counting, starting at line_nr
   1, and not line_nr 0.
*/

int util_get_current_linenr(FILE *stream) {
    offset_type init_pos = util_ftell(stream);
    int line_nr = 0;
    util_fseek(stream, 0L, SEEK_SET);
    {
        offset_type char_nr;
        int c;
        for (char_nr = 0; char_nr < init_pos; char_nr++) {
            c = fgetc(stream);
            if (c == '\n')
                line_nr++;
        }
    }
    return line_nr;
}

int util_fnmatch(const char *pattern, const char *string) {
#ifdef HAVE_FNMATCH
    return fnmatch(pattern, string, 0);
#else
#pragma comment(lib, "shlwapi.lib")
    bool match = PathMatchSpec(string, pattern); // shlwapi
    if (match)
        return 0;
    else
        return 1;

#endif
}

/* Conditional compilation; this last section includes several
   functions which are included if certain features like e.g. posix_spawn()
   are present. */

void util_time_utc(time_t *t, struct tm *ts) {
#ifdef HAVE_GMTIME_R
    gmtime_r(t, ts);
#else
    struct tm *ts_shared = localtime(t);
    memcpy(ts, ts_shared, sizeof *ts);
#endif
}
