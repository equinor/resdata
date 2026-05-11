#include <cstring>
#include <cctype>
#include <optional>
#include <memory>

#include <ert/util/util.hpp>
#include <fmt/format.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_util.hpp>

#define MAX_GRDECL_HEADER_SIZE 512
#define MAX_GRDECL_HEADER_SCANF_FMT "%511s"

/*
  This file is devoted to different routines for reading and writing
  GRDECL formatted files. These files are very weakly formatted; and
  generally hard to work with. Things to consider
  include:

   1. The files have no proper header; only the name of the keyword.

      a) If you have a corresponding grid file it might be possible to
         infer the correct size from this, otherwise you must just
         load all the data you find.

      b) There is not type information; presented with a bunch of
         formatted numbers it is in general impossible to determine
         whether the underlying datatype should be integer, float or
         double. Therefore all the file-reading routines here expect an
         rd_data_type as input.

   2. The files can have comment sections; even in the data block.

   3. The * notation can be used to print repeated values in a compact
      form, i.e.  1000*0.25 to get 1000 conecutive 0.25 values.

  The typical ECLIPSE keywords found in the datafile contain a mixture
  of numeric and character data; the current code assumes that all the
  data of a keyword is of the same underlying type, and do NOT support
  such keywords.
*/

/*
  Will seek from the current position to the next keyword. If a valid
  next keyword is found the function will position the file reader at
  the beginning of the string header and return true, otherwise the
  file position will be left untouched and the function will return
  false.

  An eligible kw string should be first non whitespace string on a
  line; i.e. the function will start by checking if there are any
  non-whitespace characters on the current line. In that case the
  current line is skipped. Lines starting with the "--" comment marker
  are ignored.
*/
static bool rd_kw_grdecl_fseek_next_kw(FILE *stream) {
    long start_pos = util_ftell(stream);
    long current_pos;
    char next_kw[MAX_GRDECL_HEADER_SIZE] = {0};

    // Determine if the current position of the file pointer is at the
    // beginning of the line; if not skip the rest of the line; this is
    // applies even though the tokens leading up this are not comments.
    {
        while (true) {
            char c;
            if (util_ftell(stream) == 0)
                // We are at the very beginning of the file. Can just jump out
                // of the loop.
                break;

            util_fseek(stream, -1, SEEK_CUR);
            c = fgetc(stream);
            if (c == '\n') {
                // We have walked backwards reaching the start of the line. We
                // have not reached any !isspace() characters on the way and
                // can go back to start_pos and read from there.
                util_fseek(stream, start_pos, SEEK_SET);
                break;
            }

            if (!isspace(c)) {
                // We hit a non-whitespace character; this means that start_pos
                // was not at the start of the line. We skip the rest of this
                // line, and then start reading on the next line.
                util_fskip_lines(stream, 1);
                break;
            }
            util_fseek(stream, -2, SEEK_CUR);
        }
    }

    while (true) {
        current_pos = util_ftell(stream);
        if (fscanf(stream, MAX_GRDECL_HEADER_SCANF_FMT, next_kw) == 1) {
            if ((next_kw[0] == next_kw[1]) && (next_kw[0] == RD_COMMENT_CHAR))
                // This is a comment line - skip it.
                util_fskip_lines(stream, 1);
            else {
                // This is a valid keyword i.e. a non-commented out string; return true.
                util_fseek(stream, current_pos, SEEK_SET);
                return true;
            }
        } else {
            // EOF reached - return False.
            util_fseek(stream, start_pos, SEEK_SET);
            return false;
        }
    }
}

/**
  This function will search through a GRDECL file to look for the
  'kw'; input variables and return vales are similar to
  rd_kw_fseek_kw(). Observe that the GRDECL files are extremely
  weakly structured, it is therefore veeeery easy to fool this function
  with a malformed GRDECL file.

  In particular the comparison is case sensitive; that is probably not
  the case with ECLIPSE proper?

  If the kw is not found the file pointer is repositioned.
*/
static bool rd_kw_grdecl_fseek_kw__(const char *kw, FILE *stream) {
    long init_pos = util_ftell(stream);
    while (true) {
        if (rd_kw_grdecl_fseek_next_kw(stream)) {
            char next_kw[256] = {0};
            fscanf(stream, "%255s", next_kw);
            if (strcmp(kw, next_kw) == 0) {
                offset_type offset = (offset_type)strlen(next_kw);
                util_fseek(stream, -offset, SEEK_CUR);
                return true;
            }
        } else {
            util_fseek(stream, init_pos, SEEK_SET);
            return false;
        }
    }
}

bool rd_kw_grdecl_fseek_kw(const char *kw, bool rewind, FILE *stream) {
    if (rd_kw_grdecl_fseek_kw__(kw, stream))
        return true; /* We found the kw between current file pos and EOF. */
    else if (rewind) {
        long int init_pos = util_ftell(stream);

        util_fseek(stream, 0L, SEEK_SET);
        if (rd_kw_grdecl_fseek_kw__(
                kw, stream)) /* Try again from the beginning of the file. */
            return true;
        else
            util_fseek(
                stream, init_pos,
                SEEK_SET); /* Could not find it - reposition to initial position. */
    }

    /* If we are here - that means that we failed to find the kw. */
    return false;
}

/**
   Observe that this function does not preserve the '*' structure
   which (might) have been used in the input.
*/
static void iset_range(char *data, int data_index, int sizeof_ctype,
                       void *value_ptr, int multiplier) {
    size_t byte_offset;
    for (int index = 0; index < multiplier; index++) {
        byte_offset =
            static_cast<size_t>(data_index) + static_cast<size_t>(index);
        byte_offset *= sizeof_ctype;
        memcpy(&data[byte_offset], value_ptr, sizeof_ctype);
    }
}

template <typename T>
std::unique_ptr<T[], void (*)(void *)> checked_calloc(size_t num) {
    T *ptr = static_cast<T *>(std::calloc(num, sizeof(T)));

    if (ptr == nullptr) {
        throw std::bad_alloc{};
    }
    return std::unique_ptr<T[], void (*)(void *)>(
        ptr, [](void *p) { std::free(p); });
}

template <typename T>
void checked_realloc(std::unique_ptr<T[], void (*)(void *)> &ptr,
                     size_t new_element_count) {
    if (new_element_count == 0) {
        ptr.reset();
        return;
    }
    T *raw_ptr = ptr.release();
    void *new_raw_ptr = std::realloc(raw_ptr, new_element_count * sizeof(T));
    if (new_raw_ptr == nullptr) {
        ptr.reset(raw_ptr);
        throw std::bad_alloc{};
    }
    ptr.reset(static_cast<T *>(new_raw_ptr));
}

/**
   The @strict flag is used to indicate whether the loader will accept
   character strings embedded into a numerical grdecl keyword; this
   should of course in general not be allowed and @strict should be
   set to true. However the SPECGRID keyword used when specifying a
   grid is often given as:

     SPECGRID
         10 10 100 100 F /

   Whatever that 'F' is - it is discarded when the SPECGRID header is
   written to a GRID/EGRID file. For this reason we have the
   possibility of setting @strict to false; in which case the 'F' or
   other characters in the numerical input will be ignored.

   If @strict is set to true the function will bomb when meeting a
   non-numeric character like the 'F' above.

   ----------------------------------------------------------------

   The function supports multiplier keywords like:

   PERMX
      10000*0.15  0.16 0.17 0.18 0.19 10000*0.20
   /

   Observe that no-spaces-are-allowed-around-the-*
*/
static char *fscanf_alloc_grdecl_data(const char *header, bool strict,
                                      rd_data_type data_type, int *kw_size,
                                      FILE *stream) {
    char newline = '\n';
    bool atEOF = false;
    size_t init_size = 32;
    size_t buffer_size = 256;
    size_t data_index = 0;
    int sizeof_ctype = rd_type_get_sizeof_ctype(data_type);
    size_t data_size = init_size;
    auto buffer = checked_calloc<char>(buffer_size + 1);
    auto data = checked_calloc<char>(sizeof_ctype * data_size);

    while (true) {
        if (fscanf(stream, "%32s", buffer.get()) == 1) {
            if (strcmp(buffer.get(), RD_COMMENT_STRING) == 0) {
                // We have read a comment marker - just read up to the end of line.
                char c;
                while (true) {
                    c = fgetc(stream);
                    if (c == newline)
                        break;
                    if (c == EOF) {
                        atEOF = true;
                        break;
                    }
                }
            } else if (strcmp(buffer.get(), RD_DATA_TERMINATION) == 0)
                break;
            else {
                // We have read a valid input string; scan numerical input values from it.
                // The multiplier algorithm will fail hard if there are spaces on either side
                // of the '*'.
                union {
                    int i;
                    float f;
                    double d;
                } value;

                int multiplier;
                bool char_input = false;

                if (rd_type_is_int(data_type)) {
                    if (sscanf(buffer.get(), "%d*%d", &multiplier, &value.i) ==
                        2) {
                    } else if (sscanf(buffer.get(), "%d", &value.i) == 1)
                        multiplier = 1;
                    else {
                        char_input = true;
                        if (strict)
                            throw std::invalid_argument(
                                fmt::format("Malformed content:\"{}\" when "
                                            "reading keyword:{}",
                                            std::string(buffer.get()),
                                            std::string(header)));
                    }
                } else if (rd_type_is_float(data_type)) {
                    if (sscanf(buffer.get(), "%d*%128g", &multiplier,
                               &value.f) == 2) {
                    } else if (sscanf(buffer.get(), "%128g", &value.f) == 1)
                        multiplier = 1;
                    else {
                        char_input = true;
                        if (strict)
                            throw std::invalid_argument(
                                fmt::format("Malformed content:\"{}\" when "
                                            "reading keyword:{}",
                                            std::string(buffer.get()),
                                            std::string(header)));
                    }
                } else if (rd_type_is_double(data_type)) {
                    if (sscanf(buffer.get(), "%d*%128lg", &multiplier,
                               &value.d) == 2) {
                    } else if (sscanf(buffer.get(), "%128lg", &value.d) == 1)
                        multiplier = 1;
                    else {
                        char_input = true;
                        if (strict)
                            throw std::invalid_argument(
                                fmt::format("Malformed content:\"{}\" when "
                                            "reading keyword:{}",
                                            std::string(buffer.get()),
                                            std::string(header)));
                    }
                } else
                    throw std::invalid_argument(fmt::format(
                        "Type:{} not supported", rd_type_name(data_type)));

                // Removing this warning on user request:
                // if (char_input)
                // fprintf(stderr,"Warning: character string: \'%s\' ignored when reading keyword:%s \n",buffer , header);
                if (!char_input) {
                    size_t min_size = data_index + multiplier;
                    if (min_size >= data_size) {
                        if (min_size <= RD_KW_MAX_SIZE) {
                            size_t byte_size =
                                sizeof_ctype * sizeof *data.get();

                            data_size = util_size_t_min(
                                RD_KW_MAX_SIZE, 2 * (data_index + multiplier));
                            byte_size *= data_size;

                            checked_realloc<char>(data, byte_size);
                        } else {
                            // We are asking for more elements than can possible
                            // be adressed in an integer. Return NULL - and
                            // data size == 0; let calling scope try to handle it.
                            data_index = 0;
                            break;
                        }
                    }

                    iset_range(data.get(), data_index, sizeof_ctype, &value,
                               multiplier);
                    data_index += multiplier;
                }
            }
            if (atEOF)
                break;
        } else
            break;
    }
    *kw_size = data_index;
    checked_realloc<char>(data, sizeof_ctype * data_index * sizeof *data.get());
    return data.release();
}

/*
   This function will load a keyword from a grdecl file, and return
   it. If input argument @kw is NULL it will just try loading from the
   current position, otherwise it will start with seeking to find @kw
   first.

   Currently only integer and float types are supported in rd_type -
   any other types will lead to a hard failure.

   The rd_kw class has a quite deeply wired assumption that the
   header is a string of length 8 (that is an ECLIPSE
   limitation). The class cannot read/write kw with headers longer than 8 bytes.
   rd_kw_grdecl is a workaround allowing for reading/writing kw with long
   headers.

   kw: If the @kw argument is != NULL the function will start
     by seeking through the file to find the kw string; if the
     kw can not be found the function will return NULL - but not
     fail any more than that.

     If @kw == NULL on input the function will just fscanf() the
     first available string and use that as kw for the
     keyword. This can lead to failure in a zillion different ways;
     it is highly recommended to supply a valid string for the
     @kw argument.

   strict: see the documentation of the strict flag in the
     fscanf_alloc_grdecl_data() function. Most of the exported
     functions have hardwired strict = true.


   size: If the @size is set to <= 0 the function will just load all
     data it can find until a terminating '/' is found. If a size
     argument is given the function will check that there is
     agreement between the size input argument and the number of
     elements found on the file.


   rd_type: The files have no embedded type information and the type
     must be supplied by the calling scope. Currently only the
     RD_FLOAT_TYPE and RD_INT_TYPE types are supported.
*/
rd_kw_type *rd_kw_fscanf_alloc_grdecl(FILE *stream, const char *kw,
                                      rd_data_type data_type, int size = 0,
                                      bool strict = true) {
    if (kw && strlen(kw) >= MAX_GRDECL_HEADER_SIZE)
        throw std::invalid_argument(fmt::format(
            "Cannot read KW of more than {} bytes. strlen(kw) == {}",
            MAX_GRDECL_HEADER_SIZE, strlen(kw)));

    if (!rd_type_is_numeric(data_type))
        throw std::invalid_argument(
            "Only types FLOAT, INT and DOUBLE supported");

    if (kw != NULL)
        if (!rd_kw_grdecl_fseek_kw(kw, true, stream))
            return NULL; /* Could not find it. */

    {
        char file_header[MAX_GRDECL_HEADER_SIZE] = {0};
        if (fscanf(stream, MAX_GRDECL_HEADER_SCANF_FMT, file_header) == 1) {
            int kw_size;
            char *data = fscanf_alloc_grdecl_data(file_header, strict,
                                                  data_type, &kw_size, stream);

            // Verify size
            if (size > 0)
                if (size != kw_size) {
                    free(data);
                    throw std::invalid_argument(
                        fmt::format("size mismatch when loading:{}. File:{} "
                                    "elements. Requested:{} elements",
                                    file_header, kw_size, size));
                }

            {
                rd_kw_type *rd_kw =
                    rd_kw_alloc_new(file_header, kw_size, data_type, NULL);
                rd_kw_set_data_ptr(rd_kw, data);
                return rd_kw;
            }

        } else
            /** No header read - probably at EOF */
            return NULL;
    }
}

void rd_kw_fprintf_grdecl(const rd_kw_type *rd_kw, FILE *stream,
                          const char *special_header = nullptr) {
    if (special_header)
        fprintf(stream, "%s\n", special_header);
    else
        fprintf(stream, "%s\n", rd_kw_get_header(rd_kw));

    {
        auto fortio =
            std::unique_ptr<fortio_type, decltype(&fortio_free_FILE_wrapper)>(
                fortio_alloc_FILE_wrapper(nullptr, false, true, true, stream),
                &fortio_free_FILE_wrapper);
        rd_kw_fwrite_data(rd_kw, fortio.get());
    }
    fprintf(stream, "/\n");
}
