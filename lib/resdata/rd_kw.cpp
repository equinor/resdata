#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

#include <ert/util/util.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_type.hpp>

#define RD_KW_TYPE_ID 6111098

struct rd_kw_struct {
    UTIL_TYPE_ID_DECLARATION;
    int size;
    rd_data_type data_type;
    char *
        header8; /* Header which is right padded with ' ' to become exactly 8 characters long. Should only be used internally.*/
    char *header;     /* Header which is trimmed to no-space. */
    char *data;       /* The actual data vector. */
    bool shared_data; /* Whether this keyword has shared data or not. */
};

UTIL_IS_INSTANCE_FUNCTION(rd_kw, RD_KW_TYPE_ID)

/* For some peculiar reason the keyword data is written in blocks, all
   numeric data is in blocks of 1000 elements, and character data is
   in blocks of 105 elements.
*/

#define BLOCKSIZE_NUMERIC 1000
#define BLOCKSIZE_CHAR 105

/* When writing formatted data, the data comes in columns, with a
   certain number of elements in each row, i.e. four columns for float
   data:

   0.000   0.000   0.000   0.000
   0.000   0.000   0.000   0.000
   0.000   0.000   0.000   0.000
   ....

   These #define symbols define the number of columns for the
   different datatypes.
*/
#define COLUMNS_CHAR 7
#define COLUMNS_FLOAT 4
#define COLUMNS_DOUBLE 3
#define COLUMNS_INT 6
#define COLUMNS_MESSAGE 1
#define COLUMNS_BOOL 25

/* Format string used when writing a formatted header. */
#define WRITE_HEADER_FMT " '%-8s' %11d '%-4s'\n"

/* Format string used when reading and writing formatted
   files. Observe the following about these format strings:

    1. The format string for reading double contains two '%'
       identifiers, that is because doubles are read by parsing a
       prefix and power separately.

    2. For both double and float the write format contains two '%'
       characters - that is because the values are split in a prefix
       and a power prior to writing - see the function
       __fprintf_scientific().

    3. The logical type involves converting back and forth between 'T'
       and 'F' and internal logical representation. The format strings
       are therefore for reading/writing a character.

*/

#define READ_FMT_CHAR "%8c"
#define READ_FMT_FLOAT "%gE"
#define READ_FMT_INT "%d"
#define READ_FMT_MESS "%8c"
#define READ_FMT_BOOL "  %c"
#define READ_FMT_DOUBLE "%lgD%d"

#define WRITE_FMT_CHAR " '%-8s'"
#define WRITE_FMT_INT " %11d"
#define WRITE_FMT_FLOAT "  %11.8fE%+03d"
#define WRITE_FMT_DOUBLE "  %17.14fD%+03d"
#define WRITE_FMT_MESS "%s"
#define WRITE_FMT_BOOL "  %c"

/* The boolean type is not a native type which can be uniquely
   identified between Fortran, C, formatted and unformatted
   files:

    o In the formatted files the characters BOOL_TRUE_CHAR and
      BOOL_FALSE_CHAR are used to represent true and false values
      repsectively.

    o In the unformatted files the boolean values are
      represented as integers with the values RD_BOOL_TRUE_INT and
      RD_BOOL_FALSE_INT respectively.

   Internally in an rd_kw instance boolean values are represented as
   integers (NOT bool), with the representation given by RD_BOOL_TRUE_INT
   and RD_BOOL_FALSE_INT. This implies that read/write of unformatted
   data can go transparently without between ECLIPSE and the rd_kw
   implementation, but exported set()/get() functions with bool must
   intercept the bool values and convert to the appropriate integer
   value.
*/

// For formatted files:
#define BOOL_TRUE_CHAR 'T'
#define BOOL_FALSE_CHAR 'F'

rd_type_enum rd_kw_get_type(const rd_kw_type *);
void rd_kw_set_data_type(rd_kw_type *rd_kw, rd_data_type data_type);

static std::string read_fmt_string(const rd_data_type rd_type) {
    return fmt::format("%{}c", rd_type_get_sizeof_iotype(rd_type));
}

static std::string read_fmt(const rd_data_type data_type) {
    switch (rd_type_get_type(data_type)) {
    case (RD_CHAR_TYPE):
        return READ_FMT_CHAR;
    case (RD_INT_TYPE):
        return READ_FMT_INT;
    case (RD_FLOAT_TYPE):
        return READ_FMT_FLOAT;
    case (RD_DOUBLE_TYPE):
        return READ_FMT_DOUBLE;
    case (RD_BOOL_TYPE):
        return READ_FMT_BOOL;
    case (RD_MESS_TYPE):
        return READ_FMT_MESS;
    case (RD_STRING_TYPE):
        return read_fmt_string(data_type);
    default:
        throw std::invalid_argument(
            fmt::format("invalid rd_type: {}", rd_type_name(data_type)));
    }
}

static std::string write_fmt_string(const rd_data_type rd_type) {
    return fmt::format(" '%-{}s'", rd_type_get_sizeof_iotype(rd_type));
}

static std::string write_fmt(const rd_data_type data_type) {
    switch (rd_type_get_type(data_type)) {
    case (RD_CHAR_TYPE):
        return WRITE_FMT_CHAR;
    case (RD_INT_TYPE):
        return WRITE_FMT_INT;
    case (RD_FLOAT_TYPE):
        return WRITE_FMT_FLOAT;
    case (RD_DOUBLE_TYPE):
        return WRITE_FMT_DOUBLE;
    case (RD_BOOL_TYPE):
        return WRITE_FMT_BOOL;
    case (RD_MESS_TYPE):
        return WRITE_FMT_MESS;
    case (RD_STRING_TYPE):
        return write_fmt_string(data_type);
    default:
        throw std::invalid_argument(
            fmt::format("invalid rd_type: {}", rd_type_name(data_type)));
    }
}

static int get_blocksize(rd_data_type data_type) {
    if (rd_type_is_alpha(data_type))
        return BLOCKSIZE_CHAR;

    return BLOCKSIZE_NUMERIC;
}

static int get_columns(const rd_data_type data_type) {
    switch (rd_type_get_type(data_type)) {
    case (RD_CHAR_TYPE):
        return COLUMNS_CHAR;
    case (RD_INT_TYPE):
        return COLUMNS_INT;
    case (RD_FLOAT_TYPE):
        return COLUMNS_FLOAT;
    case (RD_DOUBLE_TYPE):
        return COLUMNS_DOUBLE;
    case (RD_BOOL_TYPE):
        return COLUMNS_BOOL;
    case (RD_MESS_TYPE):
        return COLUMNS_MESSAGE;
    case (RD_STRING_TYPE):
        return COLUMNS_CHAR; // TODO: Is this correct?
    default:
        throw std::invalid_argument(
            fmt::format("invalid rd_type: {}", rd_type_name(data_type)));
    }
}

static void rd_kw_assert_index(const rd_kw_type *rd_kw, int index,
                               const char *caller) {
    if (index < 0 || index >= rd_kw->size)
        throw std::invalid_argument(
            fmt::format("Invalid index lookup. kw:{} input_index:{}   size:{}",
                        rd_kw->header, index, rd_kw->size));
}

static char *rd_kw_alloc_output_buffer(const rd_kw_type *rd_kw) {
    size_t sizeof_iotype = rd_type_get_sizeof_iotype(rd_kw->data_type);
    size_t buffer_size = rd_kw->size * sizeof_iotype;
    char *buffer = (char *)util_malloc(buffer_size);

    if (rd_type_is_bool(rd_kw->data_type)) {
        int *int_data = (int *)buffer;
        bool *bool_data = (bool *)rd_kw->data;

        for (int i = 0; i < rd_kw->size; i++)
            if (bool_data[i])
                int_data[i] = RD_BOOL_TRUE_INT;
            else
                int_data[i] = RD_BOOL_FALSE_INT;

        util_endian_flip_vector(buffer, sizeof_iotype, rd_kw->size);
        return buffer;
    }

    if (rd_type_is_char(rd_kw->data_type) ||
        rd_type_is_string(rd_kw->data_type)) {
        size_t sizeof_ctype = rd_type_get_sizeof_ctype(rd_kw->data_type);
        for (int i = 0; i < rd_kw->size; i++) {
            size_t buffer_offset = i * sizeof_iotype;
            size_t data_offset = i * sizeof_ctype;
            size_t string_length = strlen(&rd_kw->data[data_offset]);

            for (size_t i = 0; i < string_length; i++)
                buffer[buffer_offset + i] = rd_kw->data[data_offset + i];

            // Pad with spaces
            for (size_t i = string_length; i < sizeof_iotype; i++)
                buffer[buffer_offset + i] = ' ';
        }

        return buffer;
    }

    if (rd_type_is_mess(rd_kw->data_type))
        return buffer;

    if (rd_kw->data && buffer_size > 0) {
        memcpy(buffer, rd_kw->data, buffer_size);
        util_endian_flip_vector(buffer, sizeof_iotype, rd_kw->size);
    }

    return buffer;
}

static char *rd_kw_alloc_input_buffer(const rd_kw_type *rd_kw) {
    if (rd_kw->size < 0)
        throw std::invalid_argument(
            fmt::format("rd_kw->size was negative: {}", rd_kw->size));

    size_t sizeof_iotype = rd_type_get_sizeof_iotype(rd_kw->data_type);
    size_t count = static_cast<size_t>(rd_kw->size);
    if (sizeof_iotype != 0 &&
        count > std::numeric_limits<size_t>::max() / sizeof_iotype)
        throw std::invalid_argument(
            fmt::format("buffer size overflow: {} * {}", count, sizeof_iotype));

    size_t buffer_size = count * sizeof_iotype;
    char *buffer = (char *)util_malloc(buffer_size);

    return buffer;
}

static void rd_kw_load_from_input_buffer(rd_kw_type *rd_kw, char *buffer) {
    size_t sizeof_iotype = rd_type_get_sizeof_iotype(rd_kw->data_type);
    size_t sizeof_ctype = rd_type_get_sizeof_ctype(rd_kw->data_type);
    size_t buffer_size = rd_kw->size * sizeof_iotype;
    if (RD_ENDIAN_FLIP) {
        if (rd_type_is_numeric(rd_kw->data_type) ||
            rd_type_is_bool(rd_kw->data_type))
            util_endian_flip_vector(buffer, sizeof_iotype, rd_kw->size);
    }

    /*
    Special case bool: Return Eclipse integer representation of bool to native bool.
  */
    if (rd_type_is_bool(rd_kw->data_type)) {
        int *int_data = (int *)buffer;
        bool *bool_data = (bool *)rd_kw->data;

        for (int i = 0; i < rd_kw->size; i++) {
            if (int_data[i] == RD_BOOL_TRUE_INT)
                bool_data[i] = true;
            else
                bool_data[i] = false;
        }
        return;
    }

    /*
    Special case: insert '\0' termination at end of strings loaded from file;
    when writing out again strlen() will be called on data - i.e. it is
    paramount to add this '\0'.
  */
    if (rd_type_is_char(rd_kw->data_type) ||
        rd_type_is_string(rd_kw->data_type)) {
        const char null_char = '\0';
        for (int i = 0; i < rd_kw->size; i++) {
            size_t buffer_offset = i * sizeof_iotype;
            size_t data_offset = i * sizeof_ctype;
            memcpy(&rd_kw->data[data_offset], &buffer[buffer_offset],
                   sizeof_iotype);
            rd_kw->data[data_offset + sizeof_iotype] = null_char;
        }
        return;
    }

    if (rd_type_is_mess(rd_kw->data_type))
        return;

    /*
    Plain int, double, float data - that can be copied straight over to the ->data field.
  */
    memcpy(rd_kw->data, buffer, buffer_size);
}

static const char *rd_kw_get_header8(const rd_kw_type *rd_kw) {
    return rd_kw->header8;
}

/*
   Return the header without the trailing spaces
*/
const char *rd_kw_get_header(const rd_kw_type *rd_kw) { return rd_kw->header; }

void rd_kw_get_memcpy_data(const rd_kw_type *rd_kw, void *target) {
    if (rd_kw->size < 0)
        throw std::invalid_argument(
            fmt::format("rd_kw size was negative: {}", rd_kw->size));
    memcpy(target, rd_kw->data,
           static_cast<size_t>(rd_kw->size) *
               rd_type_get_sizeof_ctype(rd_kw->data_type));
}

void rd_kw_set_memcpy_data(rd_kw_type *rd_kw, const void *src) {
    if (rd_kw->size < 0)
        throw std::invalid_argument(
            fmt::format("rd_kw size was negative: {}", rd_kw->size));
    if (src != NULL)
        memcpy(rd_kw->data, src,
               static_cast<size_t>(rd_kw->size) *
                   rd_type_get_sizeof_ctype(rd_kw->data_type));
}

static bool rd_kw_string_eq(const char *s1, const char *s2) {
    const char space_char = ' ';
    const char *long_kw = (strlen(s1) >= strlen(s2)) ? s1 : s2;
    const char *short_kw = (strlen(s1) < strlen(s2)) ? s1 : s2;
    const int len1 = strlen(long_kw);
    const int len2 = strlen(short_kw);
    int index;
    bool eq = true;
    if (len1 > RD_STRING8_LENGTH)
        throw std::invalid_argument(
            fmt::format("eclipse keyword:{} is too long", long_kw));

    for (index = 0; index < len2; index++)
        eq = eq & (long_kw[index] == short_kw[index]);

    if (eq) {
        for (index = len2; index < len1; index++)
            eq = eq & (long_kw[index] == space_char);
    }

    return eq;
}

bool rd_kw_size_and_type_equal(const rd_kw_type *rd_kw1,
                               const rd_kw_type *rd_kw2) {
    return (rd_kw1->size == rd_kw2->size &&
            rd_type_is_equal(rd_kw1->data_type, rd_kw2->data_type));
}

static bool rd_kw_header_eq(const rd_kw_type *rd_kw1,
                            const rd_kw_type *rd_kw2) {
    bool equal = true;

    if (strcmp(rd_kw1->header8, rd_kw2->header8) != 0)
        equal = false;
    else
        equal = rd_kw_size_and_type_equal(rd_kw1, rd_kw2);

    return equal;
}

static bool rd_kw_data_equal__(const rd_kw_type *rd_kw, const void *data,
                               int cmp_elements) {
    if (cmp_elements < 0)
        throw std::invalid_argument(
            fmt::format("cmp_elements was negative: {}", cmp_elements));
    int cmp = memcmp(rd_kw->data, data,
                     static_cast<size_t>(cmp_elements) *
                         rd_type_get_sizeof_ctype(rd_kw->data_type));
    if (cmp == 0)
        return true;
    else
        return false;
}

/**
   Observe that the comparison is done with memcmp() -
   i.e. "reasonably good" numerical agreement is *not* enough.
*/

bool rd_kw_data_equal(const rd_kw_type *rd_kw, const void *data) {
    return rd_kw_data_equal__(rd_kw, data, rd_kw->size);
}

bool rd_kw_content_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2) {
    if (rd_kw_size_and_type_equal(rd_kw1, rd_kw2))
        return rd_kw_data_equal__(rd_kw1, rd_kw2->data, rd_kw1->size);
    else
        return false;
}

/**
   This function compares two rd_kw instances, and returns true if they are equal.
*/

bool rd_kw_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2) {
    bool equal = rd_kw_header_eq(rd_kw1, rd_kw2);
    if (equal)
        equal = rd_kw_data_equal(rd_kw1, rd_kw2->data);

    return equal;
}

#define RD_KW_NUMERIC_CMP(ctype)                                               \
    static bool rd_kw_numeric_equal_##ctype(const rd_kw_type *rd_kw1,          \
                                            const rd_kw_type *rd_kw2,          \
                                            ctype abs_diff, ctype rel_diff) {  \
        int index;                                                             \
        bool equal = true;                                                     \
        {                                                                      \
            const ctype *data1 = (const ctype *)rd_kw1->data;                  \
            const ctype *data2 = (const ctype *)rd_kw2->data;                  \
            for (index = 0; index < rd_kw1->size; index++) {                   \
                equal = util_##ctype##_approx_equal__(                         \
                    data1[index], data2[index], rel_diff, abs_diff);           \
                if (!equal)                                                    \
                    break;                                                     \
            }                                                                  \
        }                                                                      \
        return equal;                                                          \
    }

RD_KW_NUMERIC_CMP(float)
RD_KW_NUMERIC_CMP(double)
#undef RD_KW_NUMERIC_CMP

/**
   This function compares the data of two rd_kw instances, and
   returns true if the relative numerical difference is less than
   @rel_diff. Does not consider consider the kw header.
*/

bool rd_kw_numeric_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2,
                         double abs_diff, double rel_diff) {
    if (!rd_kw_size_and_type_equal(rd_kw1, rd_kw2))
        return false;

    if (rd_type_is_float(rd_kw1->data_type))
        return rd_kw_numeric_equal_float(rd_kw1, rd_kw2, abs_diff, rel_diff);
    else if (rd_type_is_double(rd_kw1->data_type))
        return rd_kw_numeric_equal_double(rd_kw1, rd_kw2, abs_diff, rel_diff);
    else
        return rd_kw_data_equal(rd_kw1, rd_kw2->data);
}

static void rd_kw_set_shared_ref(rd_kw_type *rd_kw, void *data_ptr) {
    if (!rd_kw->shared_data) {
        if (rd_kw->data != NULL)
            throw std::invalid_argument(
                "can not change to shared for keyword with allocated storage");
    }
    rd_kw->shared_data = true;
    rd_kw->data = (char *)data_ptr;
}

static void rd_kw_initialize(rd_kw_type *rd_kw, const char *header, int size,
                             rd_data_type data_type) {
    rd_kw_set_data_type(rd_kw, data_type);
    rd_kw_set_header_name(rd_kw, header);
    rd_kw->size = size;
}

static size_t rd_kw_fortio_data_size(const rd_kw_type *rd_kw) {
    if (rd_kw->size < 0)
        throw std::invalid_argument(
            fmt::format("rd_kw->size was negative: {}", rd_kw->size));

    const int blocksize = get_blocksize(rd_kw->data_type);
    const int num_blocks =
        rd_kw->size / blocksize + (rd_kw->size % blocksize == 0 ? 0 : 1);

    return static_cast<size_t>(num_blocks) *
               (4 + 4) + // Fortran fluff for each block
           static_cast<size_t>(rd_kw->size) *
               rd_type_get_sizeof_iotype(rd_kw->data_type); // Actual data
}

/**
   Returns the number of bytes this rd_kw instance would occupy in
   BINARY file; we add 2*4 to the header size to include the size of
   the fortran header and trailer combo.
*/

size_t rd_kw_fortio_size(const rd_kw_type *rd_kw) {
    size_t size = RD_KW_HEADER_FORTIO_SIZE;
    size += rd_kw_fortio_data_size(rd_kw);
    return size;
}

/**
   This is where the storage buffer of the rd_kw is allocated.
*/
static void rd_kw_alloc_data(rd_kw_type *rd_kw) {
    if (rd_kw->shared_data)
        throw std::invalid_argument(
            "trying to allocate data for rd_kw object which has been declared "
            "with shared storage");

    {

        if (rd_kw->size < 0)
            throw std::invalid_argument(
                fmt::format("rd_kw size was negative: {}", rd_kw->size));
        size_t byte_size = static_cast<size_t>(rd_kw->size) *
                           rd_type_get_sizeof_ctype(rd_kw->data_type);
        rd_kw->data = (char *)util_realloc(rd_kw->data, byte_size);
        if (rd_kw->data) {
            memset(rd_kw->data, 0, byte_size);
        }
    }
}

/**
   The data is copied from the input argument to the rd_kw; data can be NULL.
*/
rd_kw_type *rd_kw_alloc_new(const char *header, int size,
                            rd_data_type data_type, const void *data) {
    rd_kw_ptr rd_kw = make_rd_kw();
    rd_kw_initialize(rd_kw.get(), header, size, data_type);
    if (data != NULL) {
        rd_kw_alloc_data(rd_kw.get());
        rd_kw_set_memcpy_data(rd_kw.get(), data);
    }
    return rd_kw.release();
}

rd_kw_type *rd_kw_alloc(const char *header, int size, rd_data_type data_type) {
    rd_kw_ptr rd_kw = make_rd_kw();
    rd_kw_initialize(rd_kw.get(), header, size, data_type);
    rd_kw_alloc_data(rd_kw.get());
    return rd_kw.release();
}

rd_kw_type *rd_kw_alloc_new_shared(const char *header, int size,
                                   rd_data_type data_type, void *data) {
    rd_kw_ptr rd_kw = make_rd_kw();
    rd_kw_initialize(rd_kw.get(), header, size, data_type);
    rd_kw_set_shared_ref(rd_kw.get(), data);
    return rd_kw.release();
}

rd_kw_type *rd_kw_alloc_empty() {
    rd_kw_type *rd_kw;

    rd_kw = (rd_kw_type *)util_malloc(sizeof *rd_kw);
    rd_kw->header = NULL;
    rd_kw->header8 = NULL;
    rd_kw->data = NULL;
    rd_kw->shared_data = false;
    rd_kw->size = 0;

    UTIL_TYPE_ID_INIT(rd_kw, RD_KW_TYPE_ID);

    return rd_kw;
}

static void rd_kw_free_data(rd_kw_type *rd_kw) {
    if (!rd_kw->shared_data)
        free(rd_kw->data);

    rd_kw->data = NULL;
}

void rd_kw_free(rd_kw_type *rd_kw) {
    free(rd_kw->header);
    free(rd_kw->header8);
    rd_kw_free_data(rd_kw);
    free(rd_kw);
}

void rd_kw_memcpy_data(rd_kw_type *target, const rd_kw_type *src) {
    if (!rd_kw_size_and_type_equal(target, src))
        throw std::invalid_argument("type/size mismatch");

    if (target->size < 0)
        throw std::invalid_argument(
            fmt::format("target size was negative: {}", target->size));
    if (target->size == 0)
        return;
    memcpy(target->data, src->data,
           static_cast<size_t>(target->size) *
               rd_type_get_sizeof_ctype(target->data_type));
}

void rd_kw_memcpy(rd_kw_type *target, const rd_kw_type *src) {
    target->size = src->size;
    rd_kw_set_data_type(target, src->data_type);

    rd_kw_set_header_name(target, src->header);
    rd_kw_alloc_data(target);
    rd_kw_memcpy_data(target, src);
}

rd_kw_type *rd_kw_alloc_copy(const rd_kw_type *src) {
    rd_kw_ptr new_ = make_rd_kw();
    rd_kw_memcpy(new_.get(), src);
    return new_.release();
}

/**
   This function will allocate a new copy of @src, where only the
   elements corresponding to the slice [index1:index2) is included.

   The input parameters @index1 and @index2 can to some extent be
   out-of-range:

       index1 = max( index1 , 0 );
       index2 = min( index2 , size );

   If index1 > index2 it will fail hard; the same applies if stride is
   <= 0.
*/

rd_kw_type *rd_kw_alloc_slice_copy(const rd_kw_type *src, int index1,
                                   int index2, int stride) {
    if (index1 < 0)
        index1 = 0;
    if (index2 > src->size)
        index2 = src->size;
    if (index1 >= src->size)
        throw std::invalid_argument(
            fmt::format("index1={} > size:{}", index1, src->size));
    if (stride <= 0)
        throw std::invalid_argument(
            fmt::format("stride:{} completely broken ...", stride));

    rd_kw_ptr new_kw(nullptr, rd_kw_free);
    int src_index = index1;
    /* 1: Determine size of the sliced copy. */
    int new_size = 0;
    while (src_index < index2) {
        new_size++;
        src_index += stride;
    }
    if (new_size > 0) {
        new_kw.reset(rd_kw_alloc_empty());
        rd_kw_initialize(new_kw.get(), src->header, new_size, src->data_type);
        rd_kw_alloc_data(new_kw.get());

        /* 2: Copy over the elements. */
        src_index = index1;
        {
            int target_index = 0;
            const char *src_ptr = src->data;
            char *new_ptr = new_kw->data;
            int sizeof_ctype = rd_type_get_sizeof_ctype(new_kw->data_type);

            while (src_index < index2) {
                memcpy(&new_ptr[target_index * sizeof_ctype],
                       &src_ptr[src_index * sizeof_ctype], sizeof_ctype);
                src_index += stride;
                target_index += 1;
            }
        }
    }
    return new_kw.release();
}

void rd_kw_resize(rd_kw_type *rd_kw, int new_size) {
    if (rd_kw->shared_data)
        throw std::invalid_argument(
            "trying to allocate data for rd_kw object which has been declared "
            "with shared storage");

    if (new_size != rd_kw->size) {
        if (rd_kw->size < 0)
            throw std::invalid_argument(
                fmt::format("rd_kw size was negative: {}", rd_kw->size));
        if (new_size < 0)
            throw std::invalid_argument(
                fmt::format("new_size was negative: {}", new_size));
        size_t old_byte_size = static_cast<size_t>(rd_kw->size) *
                               rd_type_get_sizeof_ctype(rd_kw->data_type);
        size_t new_byte_size = static_cast<size_t>(new_size) *
                               rd_type_get_sizeof_ctype(rd_kw->data_type);

        rd_kw->data = (char *)util_realloc(rd_kw->data, new_byte_size);
        if (new_byte_size > old_byte_size) {
            size_t offset = old_byte_size;
            memset(&rd_kw->data[offset], 0, new_byte_size - old_byte_size);
        }
        rd_kw->size = new_size;
    }
}

/**
   Will allocate a copy of the src_kw. Will copy @count elements
   starting at @offset. If @count < 0 all remaining elements from
   @offset will be copied. If new_kw == NULL the new keyword will have
   the same header as the @src, otherwise the value @new_kw will be
   used.
*/

rd_kw_type *rd_kw_alloc_sub_copy(const rd_kw_type *src, const char *new_kw,
                                 int offset, int count) {
    if (new_kw == NULL)
        new_kw = src->header;

    if (count < 0)
        count = src->size - offset;

    if ((offset < 0) || (offset >= src->size))
        throw std::invalid_argument(
            fmt::format("invalid offset - limits: [{},{})", 0, src->size));
    if ((count + offset) > src->size)
        throw std::invalid_argument(
            fmt::format("invalid count value: {}", count));

    {
        void *src_data = rd_kw_iget_ptr(src, offset);
        return rd_kw_alloc_new(new_kw, count, src->data_type, src_data);
    }
}

static void *rd_kw_iget_ptr_static(const rd_kw_type *rd_kw, int i) {
    rd_kw_assert_index(rd_kw, i, __func__);
    return &rd_kw->data[i * rd_type_get_sizeof_ctype(rd_kw->data_type)];
}

static void rd_kw_iget_static(const rd_kw_type *rd_kw, int i, void *iptr) {
    memcpy(iptr, rd_kw_iget_ptr_static(rd_kw, i),
           rd_type_get_sizeof_ctype(rd_kw->data_type));
}

static void rd_kw_iset_static(rd_kw_type *rd_kw, int i, const void *iptr) {
    size_t sizeof_ctype = rd_type_get_sizeof_ctype(rd_kw->data_type);
    rd_kw_assert_index(rd_kw, i, __func__);
    memcpy(&rd_kw->data[i * sizeof_ctype], iptr, sizeof_ctype);
}

void rd_kw_iget(const rd_kw_type *rd_kw, int i, void *iptr) {
    rd_kw_iget_static(rd_kw, i, iptr);
}

/**
   Will return a double value for underlying data types of double,
   float and int.
*/
double rd_kw_iget_as_double(const rd_kw_type *rd_kw, int index) {
    if (rd_type_is_float(rd_kw->data_type))
        return rd_kw_iget_float(
            rd_kw,
            index); /* Here the compiler will silently insert a float -> double conversion. */
    else if (rd_type_is_double(rd_kw->data_type))
        return rd_kw_iget_double(rd_kw, index);
    else if (rd_type_is_int(rd_kw->data_type))
        return rd_kw_iget_int(rd_kw, index); /*  */
    else {
        throw std::invalid_argument(
            "can not be converted to double - no data for you!");
        return -1;
    }
}

#define RD_KW_IGET_TYPED(ctype, RD_TYPE)                                       \
    ctype rd_kw_iget_##ctype(const rd_kw_type *rd_kw, int i) {                 \
        ctype value;                                                           \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        rd_kw_iget_static(rd_kw, i, &value);                                   \
        return value;                                                          \
    }

RD_KW_IGET_TYPED(double, RD_DOUBLE_TYPE);
RD_KW_IGET_TYPED(float, RD_FLOAT_TYPE);
RD_KW_IGET_TYPED(int, RD_INT_TYPE);
RD_KW_IGET_TYPED(bool, RD_BOOL_TYPE);
#undef RD_KW_IGET_TYPED

const char *rd_kw_iget_char_ptr(const rd_kw_type *rd_kw, int i) {
    if (rd_kw_get_type(rd_kw) != RD_CHAR_TYPE)
        throw std::invalid_argument(
            fmt::format("Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));
    return (const char *)rd_kw_iget_ptr(rd_kw, i);
}

const char *rd_kw_iget_string_ptr(const rd_kw_type *rd_kw, int i) {
    if (rd_kw_get_type(rd_kw) != RD_STRING_TYPE)
        throw std::invalid_argument(
            fmt::format("Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));
    return (const char *)rd_kw_iget_ptr(rd_kw, i);
}

/**
   This will set the elemnts of the rd_kw data storage in index to
   the value of s8; if s8 is shorter than 8 characters the result will
   be padded, if s8 is longer than 8 characters the characters from 9
   and out will be ignored.
*/
void rd_kw_iset_string8(rd_kw_type *rd_kw, int index, const char *s8) {
    char *rd_string = (char *)rd_kw_iget_ptr(rd_kw, index);
    if (strlen(s8) >= RD_STRING8_LENGTH) {
        /* The whole string goes in - possibly loosing content at the end. */
        int i;
        for (i = 0; i < RD_STRING8_LENGTH; i++)
            rd_string[i] = s8[i];
    } else {
        /* The string is padded with trailing spaces. */
        int string_length = strlen(s8);
        int i;

        for (i = 0; i < string_length; i++)
            rd_string[i] = s8[i];

        for (i = string_length; i < RD_STRING8_LENGTH; i++)
            rd_string[i] = ' ';
    }

    rd_string[RD_STRING8_LENGTH] = '\0';
}

/**
   This function will set the string @index in the rd_kw string array
   to @s. IFF @s is longer than 8 characters, the first part will go
   in element @index, and then we will continue writing into the next
   elements. If the resulting index goes beyond the length of the
   keyword - WhamBang!

   You should know what you are doing when sending in a string of
   length greater than 8 - maybe the overwriting of consecutive
   elements is not what you want?
*/
void rd_kw_iset_char_ptr(rd_kw_type *rd_kw, int index, const char *s) {
    int strings = strlen(s) / RD_STRING8_LENGTH;
    if ((strlen(s) % RD_STRING8_LENGTH) != 0)
        strings++;
    {
        int sub_index;
        for (sub_index = 0; sub_index < strings; sub_index++)
            rd_kw_iset_string8(rd_kw, index + sub_index,
                               &s[sub_index * RD_STRING8_LENGTH]);
    }
}

/**
 This function will verify that the given string is of approperiate
 length (0 <= lenght <= data_type.element_size). If so, the elements
 of @s will be written to the @rd_kw string array starting at
 @index. If the input string is shorter than the type length the
 string will be padded with trailing spaces.
 */
void rd_kw_iset_string_ptr(rd_kw_type *rd_kw, int index, const char *s) {
    if (!rd_type_is_alpha(rd_kw_get_data_type(rd_kw))) {
        throw std::invalid_argument(
            fmt::format("Expected alphabetic data type (CHAR, CXXX or MESS), "
                        "was {}",
                        rd_type_name(rd_kw_get_data_type(rd_kw))));
    }

    size_t input_len = strlen(s);
    size_t type_len = rd_type_get_sizeof_iotype(rd_kw->data_type);

    if (input_len > type_len)
        throw std::invalid_argument(fmt::format(
            "String of length {} cannot hold input string of length {}",
            type_len, input_len));

    {
        char *rd_string = (char *)rd_kw_iget_ptr(rd_kw, index);
        size_t i;

        for (i = 0; i < input_len; ++i)
            rd_string[i] = s[i];

        for (i = input_len; i < type_len; ++i)
            rd_string[i] = ' ';

        rd_string[type_len] = '\0';
    }
}

/**
   This function will compare the string at position @index with the
   input @other string. The comparison will be done in a
   'space-tolerant', i.e. trailing spaces are ignored in the
   comparison. If the strings are considered equal true is returned.
*/

bool rd_kw_icmp_string(const rd_kw_type *rd_kw, int index,
                       const char *other_string) {
    const char *kw_string = (const char *)rd_kw_iget_char_ptr(rd_kw, index);
    if (strlen(other_string)) {
        const char *match = strstr(kw_string, other_string);
        if (match == kw_string)
            return true;
    }

    return false;
}

#define RD_KW_ISET_TYPED(ctype, RD_TYPE)                                       \
    void rd_kw_iset_##ctype(rd_kw_type *rd_kw, int i, ctype value) {           \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        rd_kw_iset_static(rd_kw, i, &value);                                   \
    }

RD_KW_ISET_TYPED(double, RD_DOUBLE_TYPE);
RD_KW_ISET_TYPED(float, RD_FLOAT_TYPE);
RD_KW_ISET_TYPED(int, RD_INT_TYPE);
RD_KW_ISET_TYPED(bool, RD_BOOL_TYPE);
#undef RD_KW_ISET_TYPED

#define RD_KW_SET_INDEXED(ctype, RD_TYPE)                                      \
    void rd_kw_set_indexed_##ctype(                                            \
        rd_kw_type *rd_kw, const int_vector_type *index_list, ctype value) {   \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        {                                                                      \
            ctype *data = (ctype *)rd_kw->data;                                \
            int size = int_vector_size(index_list);                            \
            const int *index_ptr = int_vector_get_const_ptr(index_list);       \
            int i;                                                             \
            for (i = 0; i < size; i++)                                         \
                data[index_ptr[i]] = value;                                    \
        }                                                                      \
    }

RD_KW_SET_INDEXED(double, RD_DOUBLE_TYPE);
RD_KW_SET_INDEXED(float, RD_FLOAT_TYPE);
RD_KW_SET_INDEXED(int, RD_INT_TYPE);
#undef RD_KW_SET_INDEXED

#define RD_KW_SHIFT_INDEXED(ctype, RD_TYPE)                                    \
    void rd_kw_shift_indexed_##ctype(                                          \
        rd_kw_type *rd_kw, const int_vector_type *index_list, ctype shift) {   \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        {                                                                      \
            ctype *data = (ctype *)rd_kw->data;                                \
            int size = int_vector_size(index_list);                            \
            const int *index_ptr = int_vector_get_const_ptr(index_list);       \
            int i;                                                             \
            for (i = 0; i < size; i++)                                         \
                data[index_ptr[i]] += shift;                                   \
        }                                                                      \
    }

RD_KW_SHIFT_INDEXED(double, RD_DOUBLE_TYPE);
RD_KW_SHIFT_INDEXED(float, RD_FLOAT_TYPE);
RD_KW_SHIFT_INDEXED(int, RD_INT_TYPE);
#undef RD_KW_SHIFT_INDEXED

#define RD_KW_SCALE_INDEXED(ctype, RD_TYPE)                                    \
    void rd_kw_scale_indexed_##ctype(                                          \
        rd_kw_type *rd_kw, const int_vector_type *index_list, ctype scale) {   \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        {                                                                      \
            ctype *data = (ctype *)rd_kw->data;                                \
            int size = int_vector_size(index_list);                            \
            const int *index_ptr = int_vector_get_const_ptr(index_list);       \
            int i;                                                             \
            for (i = 0; i < size; i++)                                         \
                data[index_ptr[i]] *= scale;                                   \
        }                                                                      \
    }

RD_KW_SCALE_INDEXED(double, RD_DOUBLE_TYPE);
RD_KW_SCALE_INDEXED(float, RD_FLOAT_TYPE);
RD_KW_SCALE_INDEXED(int, RD_INT_TYPE);
#undef RD_KW_SCALE_INDEXED

#define RD_KW_GET_TYPED_PTR(ctype, RD_TYPE)                                    \
    ctype *rd_kw_get_##ctype##_ptr(const rd_kw_type *rd_kw) {                  \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        return (ctype *)rd_kw->data;                                           \
    }

RD_KW_GET_TYPED_PTR(double, RD_DOUBLE_TYPE);
RD_KW_GET_TYPED_PTR(float, RD_FLOAT_TYPE);
RD_KW_GET_TYPED_PTR(int, RD_INT_TYPE);
RD_KW_GET_TYPED_PTR(bool, RD_BOOL_TYPE);
#undef RD_KW_GET_TYPED_PTR

void *rd_kw_get_void_ptr(const rd_kw_type *rd_kw) { return rd_kw->data; }

void *rd_kw_iget_ptr(const rd_kw_type *rd_kw, int i) {
    return rd_kw_iget_ptr_static(rd_kw, i);
}

void rd_kw_iset(rd_kw_type *rd_kw, int i, const void *iptr) {
    rd_kw_iset_static(rd_kw, i, iptr);
}

static bool rd_kw_qskip(FILE *stream) {
    const char sep = '\'';
    const char space = ' ';
    const char newline = '\n';
    const char tab = '\t';
    bool OK = true;
    char c;
    bool cont = true;
    while (cont) {
        c = fgetc(stream);
        if (c == EOF) {
            cont = false;
            OK = false;
        } else {
            if (c == space || c == newline || c == tab)
                cont = true;
            else if (c == sep)
                cont = false;
        }
    }
    return OK;
}

static bool rd_kw_fscanf_qstring(char *s, const char *fmt, int len,
                                 FILE *stream) {
    const char null_char = '\0';
    char last_sep;
    bool OK;
    OK = rd_kw_qskip(stream);
    if (OK) {
        int read_count = 0;
        read_count += fscanf(stream, fmt, s);
        s[len] = null_char;
        read_count += fscanf(stream, "%c", &last_sep);

        if (read_count != 2)
            throw std::runtime_error(
                "reading 'xxxxxxxx' formatted string failed");
    }
    return OK;
}

/*
  This rather painful parsing is because formatted eclipse double
  format : 0.ddddD+01 - difficult to parse the 'D';
*/
/** Should be: NESTED */

static double __fscanf_RD_double(FILE *stream, const char *fmt) {
    int read_count, power;
    double value, arg;
    read_count = fscanf(stream, fmt, &arg, &power);
    if (read_count == 2)
        value = arg * pow(10, power);
    else {
        throw std::runtime_error("read failed");
        value = -1;
    }
    return value;
}

static bool rd_kw_fread_data(rd_kw_type *rd_kw, ERT::FortIO &fortio) {
    bool fmt_file = fortio.fmt_file();
    if (rd_kw->size > 0) {
        const int blocksize = get_blocksize(rd_kw->data_type);
        if (fmt_file) {
            const int blocks = rd_kw->size / blocksize +
                               (rd_kw->size % blocksize == 0 ? 0 : 1);
            const std::string read_format = read_fmt(rd_kw->data_type);
            FILE *stream = fortio.get_FILE();
            int offset = 0;
            int index = 0;
            int ib, ir;
            for (ib = 0; ib < blocks; ib++) {
                int read_elm = util_int_min((ib + 1) * blocksize, rd_kw->size) -
                               ib * blocksize;
                for (ir = 0; ir < read_elm; ir++) {
                    switch (rd_kw_get_type(rd_kw)) {
                    case (RD_CHAR_TYPE):
                        rd_kw_fscanf_qstring(&rd_kw->data[offset],
                                             read_format.c_str(), 8, stream);
                        break;
                    case (RD_STRING_TYPE):
                        rd_kw_fscanf_qstring(&rd_kw->data[offset],
                                             read_format.c_str(),
                                             rd_type_get_sizeof_iotype(
                                                 rd_kw_get_data_type(rd_kw)),
                                             stream);
                        break;
                    case (RD_INT_TYPE): {
                        int iread = fscanf(stream, read_format.c_str(),
                                           (int *)&rd_kw->data[offset]);
                        if (iread != 1)
                            throw std::runtime_error(fmt::format(
                                "after reading {} values reading of keyword:{} "
                                "from:{} failed",
                                offset /
                                    rd_type_get_sizeof_ctype(rd_kw->data_type),
                                rd_kw->header8, fortio.filename_ref()));
                    } break;
                    case (RD_FLOAT_TYPE): {
                        int iread = fscanf(stream, read_format.c_str(),
                                           (float *)&rd_kw->data[offset]);
                        if (iread != 1) {
                            throw std::runtime_error(fmt::format(
                                "after reading {} values reading of keyword:{} "
                                "from:{} failed",
                                offset /
                                    rd_type_get_sizeof_ctype(rd_kw->data_type),
                                rd_kw->header8, fortio.filename_ref()));
                        }
                    } break;
                    case (RD_DOUBLE_TYPE): {
                        double value =
                            __fscanf_RD_double(stream, read_format.c_str());
                        rd_kw_iset(rd_kw, index, &value);
                    } break;
                    case (RD_BOOL_TYPE): {
                        char bool_char;
                        if (fscanf(stream, read_format.c_str(), &bool_char) ==
                            1) {
                            if (bool_char == BOOL_TRUE_CHAR)
                                rd_kw_iset_bool(rd_kw, index, true);
                            else if (bool_char == BOOL_FALSE_CHAR)
                                rd_kw_iset_bool(rd_kw, index, false);
                            else
                                throw std::runtime_error(fmt::format(
                                    "Logical value: [{}] not recogniced",
                                    bool_char));
                        } else
                            throw std::runtime_error(
                                "read failed - premature file end?");
                    } break;
                    case (RD_MESS_TYPE):
                        rd_kw_fscanf_qstring(&rd_kw->data[offset],
                                             read_format.c_str(), 8, stream);
                        break;
                    default:
                        throw std::runtime_error(
                            fmt::format("Internal error: internal "
                                        "eclipse_type: {} not recognized",
                                        rd_kw_get_type(rd_kw)));
                    }
                    offset += rd_type_get_sizeof_ctype(rd_kw->data_type);
                    index++;
                }
            }

            /* Skip the trailing newline */
            fortio.fseek(1, SEEK_CUR);
            return true;
        } else {
            char *buffer = rd_kw_alloc_input_buffer(rd_kw);
            const int sizeof_iotype =
                rd_type_get_sizeof_iotype(rd_kw->data_type);
            bool read_ok =
                fortio.fread_buffer(buffer, rd_kw->size * sizeof_iotype);

            if (read_ok)
                rd_kw_load_from_input_buffer(rd_kw, buffer);

            free(buffer);
            return read_ok;
        }
    } else
        /* The keyword has zero size - and reading data is trivially OK. */
        return true;
}

/**
   Reads a selection of elements (given by @index_map) from the data
   section of a single keyword. The @kw_offset argument is the byte
   offset of the start of the keyword (i.e. its header) in the file,
   as stored by rd_file_kw.
*/
void rd_kw_fread_indexed_data(ERT::FortIO &fortio, offset_type kw_offset,
                              rd_data_type data_type, int element_count,
                              const int_vector_type *index_map,
                              char *io_buffer) {
    int sizeof_iotype = rd_type_get_sizeof_iotype(data_type);

    // For unformatted (binary) files the individual elements have a fixed
    // on-disk size, so we can seek directly to each requested element. For
    // formatted (ASCII) files the elements have a variable text width and
    // direct seeking is not possible; in that case we read the whole
    // keyword and extract the requested elements afterwards.
    if (fortio.fmt_file()) {
        fortio.fseek(kw_offset, SEEK_SET);
        rd_kw_ptr rd_kw(rd_kw_fread_alloc(fortio), rd_kw_free);
        if (rd_kw == NULL)
            throw std::runtime_error(fmt::format(
                "failed to load keyword at offset:{}", (long)kw_offset));

        for (int index = 0; index < int_vector_size(index_map); index++) {
            int element_index = int_vector_iget(index_map, index);
            memcpy(&io_buffer[index * sizeof_iotype],
                   rd_kw_iget_ptr(rd_kw.get(), element_index), sizeof_iotype);
        }
    } else {
        const int block_size = get_blocksize(data_type);
        FILE *stream = fortio.get_FILE();
        offset_type data_offset = kw_offset + RD_KW_HEADER_FORTIO_SIZE;

        for (int index = 0; index < int_vector_size(index_map); index++) {
            int element_index = int_vector_iget(index_map, index);

            if (element_index < 0 || element_index >= element_count)
                throw std::invalid_argument(
                    fmt::format("Element index is out of range 0 <= {} < {}",
                                element_index, element_count));

            fortio.data_fseek(data_offset, element_index, sizeof_iotype,
                              element_count, block_size);
            util_fread(&io_buffer[index * sizeof_iotype], sizeof_iotype, 1,
                       stream, __func__);
        }

        if (RD_ENDIAN_FLIP)
            util_endian_flip_vector(io_buffer, sizeof_iotype,
                                    int_vector_size(index_map));
    }
}

/**
   Allocates storage and reads data.
*/
bool rd_kw_fread_realloc_data(rd_kw_type *rd_kw, ERT::FortIO &fortio) {
    rd_kw_alloc_data(rd_kw);
    return rd_kw_fread_data(rd_kw, fortio);
}

/**
   Static method without a class instance.
*/

bool rd_kw_fskip_data__(rd_data_type data_type, const int element_count,
                        ERT::FortIO &fortio) {
    if (element_count <= 0)
        return true;

    bool fmt_file = fortio.fmt_file();
    if (fmt_file) {
        /* Formatted skipping actually involves reading the data - nice ??? */
        rd_kw_ptr tmp_kw = make_rd_kw();
        rd_kw_initialize(tmp_kw.get(), "WORK", element_count, data_type);
        rd_kw_alloc_data(tmp_kw.get());
        rd_kw_fread_data(tmp_kw.get(), fortio);
    } else {
        const int blocksize = get_blocksize(data_type);
        const int block_count =
            element_count / blocksize + (element_count % blocksize != 0);
        int element_size = rd_type_get_sizeof_iotype(data_type);

        if (!fortio.data_fskip(element_size, element_count, block_count))
            return false;
    }

    return true;
}

bool rd_kw_fskip_data(rd_kw_type *rd_kw, ERT::FortIO &fortio) {
    return rd_kw_fskip_data__(rd_kw_get_data_type(rd_kw), rd_kw->size, fortio);
}

/**
   This function will skip the header part of an rd_kw instance. The
   function will read the file content at the current position, it is
   therefore essential that the file pointer is positioned at the
   beginning of a keyword when this function is called; otherwise it
   will be complete crash and burn.
*/

void rd_kw_fskip_header(ERT::FortIO &fortio) {
    bool fmt_file = fortio.fmt_file();
    if (fmt_file) {
        rd_kw_ptr rd_kw = make_rd_kw();
        rd_kw_fread_header(rd_kw.get(), fortio);
    } else
        fortio.fskip_record();
}

rd_read_status_enum rd_kw_fread_header(rd_kw_type *rd_kw, ERT::FortIO &fortio) {
    const char null_char = '\0';
    FILE *stream = fortio.get_FILE();
    bool fmt_file = fortio.fmt_file();
    char header[RD_STRING8_LENGTH + 1];
    char rd_type_str[RD_TYPE_LENGTH + 1];
    int record_size;
    int size;

    if (fmt_file) {
        if (!rd_kw_fscanf_qstring(header, "%8c", 8, stream))
            return RD_KW_READ_FAIL;

        int read_count = fscanf(stream, "%d", &size);
        if (read_count != 1)
            return RD_KW_READ_FAIL;

        if (!rd_kw_fscanf_qstring(rd_type_str, "%4c", 4, stream))
            return RD_KW_READ_FAIL;

        fgetc(stream); /* Reading the trailing newline ... */
    } else {
        header[RD_STRING8_LENGTH] = null_char;
        rd_type_str[RD_TYPE_LENGTH] = null_char;
        record_size = fortio.init_read();

        if (record_size <= 0)
            return RD_KW_READ_FAIL;

        char buffer[RD_KW_HEADER_DATA_SIZE];
        size_t read_bytes = fread(buffer, 1, RD_KW_HEADER_DATA_SIZE, stream);

        if (read_bytes != RD_KW_HEADER_DATA_SIZE)
            return RD_KW_READ_FAIL;

        memcpy(header, &buffer[0], RD_STRING8_LENGTH);
        void *ptr = &buffer[RD_STRING8_LENGTH];
        size = *((int *)ptr);

        memcpy(rd_type_str, &buffer[RD_STRING8_LENGTH + sizeof(size)],
               RD_TYPE_LENGTH);

        if (!fortio.complete_read(record_size))
            return RD_KW_READ_FAIL;

        if (RD_ENDIAN_FLIP)
            util_endian_flip_vector(&size, sizeof size, 1);
    }

    rd_data_type data_type = rd_type_create_from_name(rd_type_str);
    rd_kw_initialize(rd_kw, header, size, data_type);

    return RD_KW_READ_OK;
}

/**
   Will seek through the open fortio file and search for a keyword with
   header 'kw'. It will always start the search from the present
   position in the file, but if rewind is true it will rewind the
   fortio file if not finding 'kw' between current offset and EOF.

   If the kw is found the fortio pointer is positioned at the
   beginning of the keyword, and the function returns true. If the the
   'kw' is NOT found the file will be repositioned to the initial
   position, and the function will return false; unless abort_on_error
   == true in which case the function will abort if the 'kw' is not
   found.
*/

bool rd_kw_fseek_kw(const char *kw, bool rewind, bool abort_on_error,
                    ERT::FortIO &fortio) {
    rd_kw_ptr tmp_kw = make_rd_kw();
    long int init_pos = fortio.ftell();
    bool cont, kw_found;

    cont = true;
    kw_found = false;
    while (cont) {
        long current_pos = fortio.ftell();
        if (rd_kw_fread_header(tmp_kw.get(), fortio) == RD_KW_READ_OK) {
            if (rd_kw_string_eq(rd_kw_get_header8(tmp_kw.get()), kw)) {
                fortio.fseek(current_pos, SEEK_SET);
                kw_found = true;
                cont = false;
            } else
                rd_kw_fskip_data(tmp_kw.get(), fortio);
        } else {
            if (rewind) {
                fortio.rewind();
                rewind = false;
            } else
                cont = false;
        }
    }
    if (!kw_found) {
        if (abort_on_error)
            throw std::runtime_error(
                fmt::format("failed to locate keyword:{} in file:{}", kw,
                            fortio.filename_ref()));

        fortio.fseek(init_pos, SEEK_SET);
    }
    return kw_found;
}

void rd_kw_set_data_ptr(rd_kw_type *rd_kw, void *data) {
    if (!rd_kw->shared_data)
        free(rd_kw->data);
    rd_kw->data = (char *)data;
}

void rd_kw_set_header_name(rd_kw_type *rd_kw, const char *header) {
    rd_kw->header8 = (char *)realloc(rd_kw->header8, RD_STRING8_LENGTH + 1);
    if (strlen(header) <= 8) {
        sprintf(rd_kw->header8, "%-8s", header);

        /* Internalizing a header without the trailing spaces as well. */
        free(rd_kw->header);
        rd_kw->header = util_alloc_strip_copy(rd_kw->header8);
    } else {
        free(rd_kw->header);
        rd_kw->header = (char *)util_alloc_copy(header, strlen(header) + 1);
    }
}

void rd_kw_set_data_type(rd_kw_type *rd_kw, rd_data_type data_type) {
    memcpy(&rd_kw->data_type, &data_type, sizeof data_type);
}

bool rd_kw_fread_realloc(rd_kw_type *rd_kw, ERT::FortIO &fortio) {
    if (rd_kw_fread_header(rd_kw, fortio) == RD_KW_READ_OK)
        return rd_kw_fread_realloc_data(rd_kw, fortio);
    else
        return false;
}

rd_kw_type *rd_kw_fread_alloc(ERT::FortIO &fortio) {
    rd_kw_ptr rd_kw = make_rd_kw();
    if (!rd_kw_fread_realloc(rd_kw.get(), fortio)) {
        return nullptr;
    }
    return rd_kw.release();
}

void rd_kw_fskip(ERT::FortIO &fortio) {
    rd_kw_type *tmp_kw;
    tmp_kw = rd_kw_fread_alloc(fortio);
    rd_kw_free(tmp_kw);
}

static void rd_kw_fwrite_data_unformatted(const rd_kw_type *rd_kw,
                                          ERT::FortIO &fortio) {
    char *iobuffer = rd_kw_alloc_output_buffer(rd_kw);
    int sizeof_iotype = rd_type_get_sizeof_iotype(rd_kw->data_type);
    {
        const int blocksize = get_blocksize(rd_kw->data_type);
        const int num_blocks =
            rd_kw->size / blocksize + (rd_kw->size % blocksize == 0 ? 0 : 1);
        int block_nr;

        for (block_nr = 0; block_nr < num_blocks; block_nr++) {
            int this_blocksize =
                util_int_min((block_nr + 1) * blocksize, rd_kw->size) -
                block_nr * blocksize;
            int record_size =
                this_blocksize *
                sizeof_iotype; /* The total size in bytes of the record written by the fortio layer. */
            fortio.fwrite_record(
                &iobuffer[block_nr * blocksize * sizeof_iotype], record_size);
        }
    }
    free(iobuffer);
}

/**
     The point of this awkward function is that I have not managed to
     use C fprintf() syntax to reproduce the ECLIPSE
     formatting. ECLIPSE expects the following formatting for float
     and double values:

        0.ddddddddE+03       (float)
        0.ddddddddddddddD+03 (double)

     The problem with printf have been:

        1. To force the radix part to start with 0.
        2. To use 'D' as the exponent start for double values.

     If you are more proficient with C fprintf() format strings than I
     am, the __fprintf_scientific() function should be removed, and
     the WRITE_FMT_DOUBLE and WRITE_FMT_FLOAT format specifiers
     updated accordingly.
  */

static void __fprintf_scientific(FILE *stream, const char *fmt, double x) {
    double pow_x = ceil(log10(fabs(x)));
    double arg_x = x / pow(10.0, pow_x);
    if (x != 0.0) {
        if (fabs(arg_x) == 1.0) {
            arg_x *= 0.10;
            pow_x += 1;
        }
    } else {
        arg_x = 0.0;
        pow_x = 0.0;
    }
    fprintf(stream, fmt, arg_x, (int)pow_x);
}

static void rd_kw_fwrite_data_formatted(rd_kw_type *rd_kw,
                                        ERT::FortIO &fortio) {

    {

        FILE *stream = fortio.get_FILE();
        const int blocksize = get_blocksize(rd_kw->data_type);
        const int columns = get_columns(rd_kw->data_type);
        const std::string write_format = write_fmt(rd_kw->data_type);
        const int num_blocks =
            rd_kw->size / blocksize + (rd_kw->size % blocksize == 0 ? 0 : 1);
        int block_nr;

        for (block_nr = 0; block_nr < num_blocks; block_nr++) {
            int this_blocksize =
                util_int_min((block_nr + 1) * blocksize, rd_kw->size) -
                block_nr * blocksize;
            int num_lines = this_blocksize / columns +
                            (this_blocksize % columns == 0 ? 0 : 1);
            int line_nr;
            for (line_nr = 0; line_nr < num_lines; line_nr++) {
                int num_columns =
                    util_int_min((line_nr + 1) * columns, this_blocksize) -
                    columns * line_nr;
                int col_nr;
                for (col_nr = 0; col_nr < num_columns; col_nr++) {
                    int data_index =
                        block_nr * blocksize + line_nr * columns + col_nr;
                    void *data_ptr = rd_kw_iget_ptr_static(rd_kw, data_index);
                    switch (rd_kw_get_type(rd_kw)) {
                    case (RD_CHAR_TYPE):
                        fprintf(stream, write_format.c_str(), data_ptr);
                        break;
                    case (RD_STRING_TYPE):
                        fprintf(stream, write_format.c_str(), data_ptr);
                        break;
                    case (RD_INT_TYPE): {
                        int int_value = ((int *)data_ptr)[0];
                        fprintf(stream, write_format.c_str(), int_value);
                    } break;
                    case (RD_BOOL_TYPE): {
                        bool bool_value = ((unsigned char *)data_ptr)[0] != 0;
                        if (bool_value)
                            fprintf(stream, write_format.c_str(),
                                    BOOL_TRUE_CHAR);
                        else
                            fprintf(stream, write_format.c_str(),
                                    BOOL_FALSE_CHAR);
                    } break;
                    case (RD_FLOAT_TYPE): {
                        float float_value = ((float *)data_ptr)[0];
                        __fprintf_scientific(stream, write_format.c_str(),
                                             float_value);
                    } break;
                    case (RD_DOUBLE_TYPE): {
                        double double_value = ((double *)data_ptr)[0];
                        __fprintf_scientific(stream, write_format.c_str(),
                                             double_value);
                    } break;
                    case (RD_MESS_TYPE):
                        throw std::runtime_error(
                            "Internal inconsistency : message type keywords "
                            "should not have data");
                        break;
                    }
                }
                fprintf(stream, "\n");
            }
        }
    }
}

void rd_kw_fwrite_data(const rd_kw_type *_rd_kw, ERT::FortIO &fortio) {
    rd_kw_type *rd_kw = (rd_kw_type *)_rd_kw;
    bool fmt_file = fortio.fmt_file();

    if (fmt_file)
        rd_kw_fwrite_data_formatted(rd_kw, fortio);
    else
        rd_kw_fwrite_data_unformatted(rd_kw, fortio);
}

void rd_kw_fwrite_header(const rd_kw_type *rd_kw, ERT::FortIO &fortio) {
    FILE *stream = fortio.get_FILE();
    bool fmt_file = fortio.fmt_file();
    std::string type_name = rd_type_name(rd_kw->data_type);

    if (fmt_file)
        fprintf(stream, WRITE_HEADER_FMT, rd_kw->header8, rd_kw->size,
                type_name.c_str());
    else {
        int size = rd_kw->size;
        if (RD_ENDIAN_FLIP)
            util_endian_flip_vector(&size, sizeof size, 1);

        fortio.init_write(RD_KW_HEADER_DATA_SIZE);

        fwrite(rd_kw->header8, sizeof(char), RD_STRING8_LENGTH, stream);
        fwrite(&size, sizeof(int), 1, stream);
        fwrite(type_name.c_str(), sizeof(char), RD_TYPE_LENGTH, stream);

        fortio.complete_write(RD_KW_HEADER_DATA_SIZE);
    }
}

bool rd_kw_fwrite(const rd_kw_type *rd_kw, ERT::FortIO &fortio) {
    if (strlen(rd_kw_get_header(rd_kw)) > RD_STRING8_LENGTH) {
        fortio.fwrite_error();
        return false;
    }
    rd_kw_fwrite_header(rd_kw, fortio);
    rd_kw_fwrite_data(rd_kw, fortio);
    return true;
}

static void *rd_kw_get_data_ref(const rd_kw_type *rd_kw) { return rd_kw->data; }

void *rd_kw_get_ptr(const rd_kw_type *rd_kw) {
    return rd_kw_get_data_ref(rd_kw);
}

int rd_kw_get_size(const rd_kw_type *rd_kw) { return rd_kw->size; }

rd_type_enum rd_kw_get_type(const rd_kw_type *rd_kw) {
    return rd_type_get_type(rd_kw->data_type);
}

rd_data_type rd_kw_get_data_type(const rd_kw_type *rd_kw) {
    return rd_kw->data_type;
}

/*
  Untyped - low level alternative.
*/
static void rd_kw_scalar_set__(rd_kw_type *rd_kw, const void *value) {
    int sizeof_ctype = rd_type_get_sizeof_ctype(rd_kw->data_type);
    int i;
    for (i = 0; i < rd_kw->size; i++)
        memcpy(&rd_kw->data[i * sizeof_ctype], value, sizeof_ctype);
}

/**
   Will create a new keyword of the same type as src_kw, and size
   @target_size. The integer array mapping is a list sizeof(src_kw)
   elements, where each element is the new index, i.e.

       new_kw[ mapping[i] ]  = src_kw[i]

   For all inactive elements in new kw are set as follows:

   0          - For float / int / double
   False      - For logical
   ""         - For char
*/

rd_kw_type *rd_kw_alloc_scatter_copy(const rd_kw_type *src_kw, int target_size,
                                     const int *mapping, void *def_value) {
    int default_int = 0;
    double default_double = 0;
    float default_float = 0;
    bool default_bool = false;
    const char *default_char = "";
    rd_kw_ptr new_kw =
        make_rd_kw(src_kw->header, target_size, src_kw->data_type);

    if (def_value != NULL)
        rd_kw_scalar_set__(new_kw.get(), def_value);
    else {
        /** Initialize with defaults .*/
        switch (rd_kw_get_type(src_kw)) {
        case (RD_INT_TYPE):
            rd_kw_scalar_set__(new_kw.get(), &default_int);
            break;
        case (RD_FLOAT_TYPE):
            rd_kw_scalar_set__(new_kw.get(), &default_float);
            break;
        case (RD_DOUBLE_TYPE):
            rd_kw_scalar_set__(new_kw.get(), &default_double);
            break;
        case (RD_BOOL_TYPE):
            rd_kw_scalar_set__(new_kw.get(), &default_bool);
            break;
        case (RD_CHAR_TYPE):
            rd_kw_scalar_set__(new_kw.get(), default_char);
            break;
        default:
            throw std::invalid_argument(
                fmt::format("unsupported type:{}", rd_kw_get_type(src_kw)));
        }
    }

    {
        int sizeof_ctype = rd_type_get_sizeof_ctype(src_kw->data_type);
        int i;
        for (i = 0; i < src_kw->size; i++) {
            int target_index = mapping[i];
            memcpy(&new_kw->data[target_index * sizeof_ctype],
                   &src_kw->data[i * sizeof_ctype], sizeof_ctype);
        }
    }

    return new_kw.release();
}

rd_kw_type *rd_kw_alloc_global_copy(const rd_kw_type *src,
                                    const rd_kw_type *actnum) {
    if (rd_kw_get_type(actnum) != RD_INT_TYPE)
        return NULL;

    const int global_size = rd_kw_get_size(actnum);
    rd_kw_ptr global_copy =
        make_rd_kw(rd_kw_get_header(src), global_size, src->data_type);
    const int *mapping = rd_kw_get_int_ptr(actnum);
    const int src_size = rd_kw_get_size(src);
    int src_index = 0;
    for (int global_index = 0; global_index < global_size; global_index++) {
        if (mapping[global_index]) {
            /* We ran through and beyond the size of the src keyword. */
            if (src_index >= src_size) {
                global_copy = NULL;
                break;
            }
            const void *value_ptr = rd_kw_iget_ptr(src, src_index);
            rd_kw_iset_static(global_copy.get(), global_index, value_ptr);
            src_index++;
        }
    }

    /* Not all the src data was distributed. */
    if (src_index < src_size) {
        global_copy.reset(nullptr);
    }

    return global_copy.release();
}

void rd_kw_summarize(const rd_kw_type *rd_kw) {
    std::string type_name = rd_type_name(rd_kw->data_type);
    printf("%8s   %10d:%4s \n", rd_kw_get_header8(rd_kw), rd_kw_get_size(rd_kw),
           type_name.c_str());
}

#define RD_KW_SCALAR_SET_TYPED(ctype, RD_TYPE)                                 \
    void rd_kw_scalar_set_##ctype(rd_kw_type *rd_kw, ctype value) {            \
        if (rd_kw_get_type(rd_kw) == RD_TYPE) {                                \
            ctype *data = (ctype *)rd_kw_get_data_ref(rd_kw);                  \
            int i;                                                             \
            for (i = 0; i < rd_kw->size; i++)                                  \
                data[i] = value;                                               \
        } else                                                                 \
            throw std::invalid_argument("wrong type");                         \
    }

RD_KW_SCALAR_SET_TYPED(int, RD_INT_TYPE)
RD_KW_SCALAR_SET_TYPED(float, RD_FLOAT_TYPE)
RD_KW_SCALAR_SET_TYPED(double, RD_DOUBLE_TYPE)
RD_KW_SCALAR_SET_TYPED(bool, RD_BOOL_TYPE)
#undef RD_KW_SCALAR_SET_TYPED

void rd_kw_scalar_set_float_or_double(rd_kw_type *rd_kw, double value) {
    rd_type_enum rd_type = rd_kw_get_type(rd_kw);
    if (rd_type == RD_FLOAT_TYPE)
        rd_kw_scalar_set_float(rd_kw, (float)value);
    else if (rd_type == RD_DOUBLE_TYPE)
        rd_kw_scalar_set_double(rd_kw, value);
    else
        throw std::invalid_argument("wrong type");
}

#define RD_KW_SCALE_TYPED(ctype, RD_TYPE)                                      \
    void rd_kw_scale_##ctype(rd_kw_type *rd_kw, ctype scale_factor) {          \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        {                                                                      \
            ctype *data = (ctype *)rd_kw_get_data_ref(rd_kw);                  \
            int size = rd_kw_get_size(rd_kw);                                  \
            int i;                                                             \
            for (i = 0; i < size; i++)                                         \
                data[i] *= scale_factor;                                       \
        }                                                                      \
    }

RD_KW_SCALE_TYPED(int, RD_INT_TYPE)
RD_KW_SCALE_TYPED(float, RD_FLOAT_TYPE)
RD_KW_SCALE_TYPED(double, RD_DOUBLE_TYPE)
#undef RD_KW_SCALE_TYPED

void rd_kw_scale_float_or_double(rd_kw_type *rd_kw, double scale_factor) {
    rd_type_enum rd_type = rd_kw_get_type(rd_kw);
    if (rd_type == RD_FLOAT_TYPE)
        rd_kw_scale_float(rd_kw, (float)scale_factor);
    else if (rd_type == RD_DOUBLE_TYPE)
        rd_kw_scale_double(rd_kw, scale_factor);
    else
        throw std::invalid_argument("wrong type");
}

#define RD_KW_SHIFT_TYPED(ctype, RD_TYPE)                                      \
    void rd_kw_shift_##ctype(rd_kw_type *rd_kw, ctype shift_value) {           \
        if (rd_kw_get_type(rd_kw) != RD_TYPE)                                  \
            throw std::invalid_argument(fmt::format(                           \
                "Keyword: {} is wrong type", rd_kw_get_header8(rd_kw)));       \
        {                                                                      \
            ctype *data = (ctype *)rd_kw_get_data_ref(rd_kw);                  \
            int size = rd_kw_get_size(rd_kw);                                  \
            int i;                                                             \
            for (i = 0; i < size; i++)                                         \
                data[i] += shift_value;                                        \
        }                                                                      \
    }

RD_KW_SHIFT_TYPED(int, RD_INT_TYPE)
RD_KW_SHIFT_TYPED(float, RD_FLOAT_TYPE)
RD_KW_SHIFT_TYPED(double, RD_DOUBLE_TYPE)
#undef RD_KW_SHIFT_TYPED

void rd_kw_shift_float_or_double(rd_kw_type *rd_kw, double shift_value) {
    rd_type_enum rd_type = rd_kw_get_type(rd_kw);
    if (rd_type == RD_FLOAT_TYPE)
        rd_kw_shift_float(rd_kw, (float)shift_value);
    else if (rd_type == RD_DOUBLE_TYPE)
        rd_kw_shift_double(rd_kw, shift_value);
    else
        throw std::invalid_argument("wrong type");
}

bool rd_kw_size_and_numeric_type_equal(const rd_kw_type *kw1,
                                       const rd_kw_type *kw2) {
    return rd_kw_size_and_type_equal(kw1, kw2) &&
           rd_type_is_numeric(kw1->data_type);
}

#define RD_KW_ASSERT_TYPED_BINARY_OP(ctype, RD_TYPE)                           \
    bool rd_kw_assert_binary_##ctype(const rd_kw_type *kw1,                    \
                                     const rd_kw_type *kw2) {                  \
        if (!rd_kw_size_and_numeric_type_equal(kw1, kw2))                      \
            return false;                                                      \
        if (rd_kw_get_type(kw1) != RD_TYPE)                                    \
            return false; /* Type mismatch */                                  \
        return true;                                                           \
    }

RD_KW_ASSERT_TYPED_BINARY_OP(int, RD_INT_TYPE)
RD_KW_ASSERT_TYPED_BINARY_OP(float, RD_FLOAT_TYPE)
RD_KW_ASSERT_TYPED_BINARY_OP(double, RD_DOUBLE_TYPE)
#undef RD_KW_ASSERT_TYPED_BINARY_OP

void rd_kw_copy_indexed(rd_kw_type *target_kw, const int_vector_type *index_set,
                        const rd_kw_type *src_kw) {
    if (!rd_kw_size_and_type_equal(target_kw, src_kw))
        throw std::invalid_argument("type/size  mismatch");
    {
        char *target_data = (char *)rd_kw_get_data_ref(target_kw);
        const char *src_data = (const char *)rd_kw_get_data_ref(src_kw);
        int sizeof_ctype = rd_type_get_sizeof_ctype(target_kw->data_type);
        int set_size = int_vector_size(index_set);
        const int *index_data = int_vector_get_const_ptr(index_set);
        int i;
        for (i = 0; i < set_size; i++) {
            int index = index_data[i];
            memcpy(&target_data[index * sizeof_ctype],
                   &src_data[index * sizeof_ctype], sizeof_ctype);
        }
    }
}

#define RD_KW_TYPED_INPLACE_ADD_INDEXED(ctype)                                 \
    static void rd_kw_inplace_add_indexed_##ctype(                             \
        rd_kw_type *target_kw, const int_vector_type *index_set,               \
        const rd_kw_type *add_kw) {                                            \
        if (!rd_kw_assert_binary_##ctype(target_kw, add_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *add_data = (const ctype *)rd_kw_get_data_ref(add_kw); \
            int set_size = int_vector_size(index_set);                         \
            const int *index_data = int_vector_get_const_ptr(index_set);       \
            int i;                                                             \
            for (i = 0; i < set_size; i++) {                                   \
                int index = index_data[i];                                     \
                target_data[index] += add_data[index];                         \
            }                                                                  \
        }                                                                      \
    }

RD_KW_TYPED_INPLACE_ADD_INDEXED(int)
RD_KW_TYPED_INPLACE_ADD_INDEXED(double)
RD_KW_TYPED_INPLACE_ADD_INDEXED(float)
#undef RD_KW_TYPED_INPLACE_ADD

void rd_kw_inplace_add_indexed(rd_kw_type *target_kw,
                               const int_vector_type *index_set,
                               const rd_kw_type *add_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_add_indexed_float(target_kw, index_set, add_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_add_indexed_double(target_kw, index_set, add_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_add_indexed_int(target_kw, index_set, add_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace add not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_ADD(ctype)                                         \
    static void rd_kw_inplace_add_##ctype(rd_kw_type *target_kw,               \
                                          const rd_kw_type *add_kw) {          \
        if (!rd_kw_assert_binary_##ctype(target_kw, add_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *add_data = (const ctype *)rd_kw_get_data_ref(add_kw); \
            int i;                                                             \
            for (i = 0; i < target_kw->size; i++)                              \
                target_data[i] += add_data[i];                                 \
        }                                                                      \
    }
RD_KW_TYPED_INPLACE_ADD(int)
RD_KW_TYPED_INPLACE_ADD(double)
RD_KW_TYPED_INPLACE_ADD(float)

#undef RD_KW_TYPED_INPLACE_ADD

void rd_kw_inplace_add(rd_kw_type *target_kw, const rd_kw_type *add_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_add_float(target_kw, add_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_add_double(target_kw, add_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_add_int(target_kw, add_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace add not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_ADD_SQUARED(ctype)                                 \
    static void rd_kw_inplace_add_squared_##ctype(rd_kw_type *target_kw,       \
                                                  const rd_kw_type *add_kw) {  \
        if (!rd_kw_assert_binary_##ctype(target_kw, add_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *add_data = (const ctype *)rd_kw_get_data_ref(add_kw); \
            int i;                                                             \
            for (i = 0; i < target_kw->size; i++)                              \
                target_data[i] += add_data[i] * add_data[i];                   \
        }                                                                      \
    }
RD_KW_TYPED_INPLACE_ADD_SQUARED(int)
RD_KW_TYPED_INPLACE_ADD_SQUARED(double)
RD_KW_TYPED_INPLACE_ADD_SQUARED(float)

#undef RD_KW_TYPED_INPLACE_ADD

void rd_kw_inplace_add_squared(rd_kw_type *target_kw,
                               const rd_kw_type *add_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_add_squared_float(target_kw, add_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_add_squared_double(target_kw, add_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_add_squared_int(target_kw, add_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace add not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_SUB(ctype)                                         \
    void rd_kw_inplace_sub_##ctype(rd_kw_type *target_kw,                      \
                                   const rd_kw_type *sub_kw) {                 \
        if (!rd_kw_assert_binary_##ctype(target_kw, sub_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *sub_data = (const ctype *)rd_kw_get_data_ref(sub_kw); \
            int i;                                                             \
            for (i = 0; i < target_kw->size; i++)                              \
                target_data[i] -= sub_data[i];                                 \
        }                                                                      \
    }
RD_KW_TYPED_INPLACE_SUB(int)
RD_KW_TYPED_INPLACE_SUB(double)
RD_KW_TYPED_INPLACE_SUB(float)
#undef RD_KW_TYPED_INPLACE_SUB

void rd_kw_inplace_sub(rd_kw_type *target_kw, const rd_kw_type *sub_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_sub_float(target_kw, sub_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_sub_double(target_kw, sub_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_sub_int(target_kw, sub_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace sub not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_SUB_INDEXED(ctype)                                 \
    static void rd_kw_inplace_sub_indexed_##ctype(                             \
        rd_kw_type *target_kw, const int_vector_type *index_set,               \
        const rd_kw_type *sub_kw) {                                            \
        if (!rd_kw_assert_binary_##ctype(target_kw, sub_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *sub_data = (const ctype *)rd_kw_get_data_ref(sub_kw); \
            int set_size = int_vector_size(index_set);                         \
            const int *index_data = int_vector_get_const_ptr(index_set);       \
            int i;                                                             \
            for (i = 0; i < set_size; i++) {                                   \
                int index = index_data[i];                                     \
                target_data[index] -= sub_data[index];                         \
            }                                                                  \
        }                                                                      \
    }

RD_KW_TYPED_INPLACE_SUB_INDEXED(int)
RD_KW_TYPED_INPLACE_SUB_INDEXED(double)
RD_KW_TYPED_INPLACE_SUB_INDEXED(float)
#undef RD_KW_TYPED_INPLACE_SUB

void rd_kw_inplace_sub_indexed(rd_kw_type *target_kw,
                               const int_vector_type *index_set,
                               const rd_kw_type *sub_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_sub_indexed_float(target_kw, index_set, sub_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_sub_indexed_double(target_kw, index_set, sub_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_sub_indexed_int(target_kw, index_set, sub_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace sub not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_ABS(ctype, abs_func)                               \
    void rd_kw_inplace_abs_##ctype(rd_kw_type *kw) {                           \
        ctype *data = (ctype *)rd_kw_get_data_ref(kw);                         \
        int i;                                                                 \
        for (i = 0; i < kw->size; i++)                                         \
            data[i] = abs_func(data[i]);                                       \
    }

RD_KW_TYPED_INPLACE_ABS(int, abs)
RD_KW_TYPED_INPLACE_ABS(double, fabs)
RD_KW_TYPED_INPLACE_ABS(float, fabsf)
#undef RD_KW_TYPED_INPLACE_ABS

void rd_kw_inplace_abs(rd_kw_type *kw) {
    rd_type_enum type = rd_kw_get_type(kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_abs_float(kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_abs_double(kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_abs_int(kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace abs not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(kw))));
    }
}

static int sqrti(int x) { return round(sqrt(x)); }

#define RD_KW_TYPED_INPLACE_SQRT(ctype, sqrt_func)                             \
    void rd_kw_inplace_sqrt_##ctype(rd_kw_type *kw) {                          \
        ctype *data = (ctype *)rd_kw_get_data_ref(kw);                         \
        int i;                                                                 \
        for (i = 0; i < kw->size; i++)                                         \
            data[i] = sqrt_func(data[i]);                                      \
    }

RD_KW_TYPED_INPLACE_SQRT(double, sqrt)
RD_KW_TYPED_INPLACE_SQRT(float, sqrtf)
RD_KW_TYPED_INPLACE_SQRT(int, sqrti)
#undef RD_KW_TYPED_INPLACE_SQRT

void rd_kw_inplace_sqrt(rd_kw_type *kw) {
    rd_type_enum type = rd_kw_get_type(kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_sqrt_float(kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_sqrt_double(kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_sqrt_int(kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace sqrt not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(kw))));
    }
}

#define RD_KW_TYPED_INPLACE_MUL(ctype)                                         \
    void rd_kw_inplace_mul_##ctype(rd_kw_type *target_kw,                      \
                                   const rd_kw_type *mul_kw) {                 \
        if (!rd_kw_assert_binary_##ctype(target_kw, mul_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *mul_data = (const ctype *)rd_kw_get_data_ref(mul_kw); \
            int i;                                                             \
            for (i = 0; i < target_kw->size; i++)                              \
                target_data[i] *= mul_data[i];                                 \
        }                                                                      \
    }
RD_KW_TYPED_INPLACE_MUL(int)
RD_KW_TYPED_INPLACE_MUL(double)
RD_KW_TYPED_INPLACE_MUL(float)
#undef RD_KW_TYPED_INPLACE_MUL

void rd_kw_inplace_mul(rd_kw_type *target_kw, const rd_kw_type *mul_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_mul_float(target_kw, mul_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_mul_double(target_kw, mul_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_mul_int(target_kw, mul_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace mul not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_MUL_INDEXED(ctype)                                 \
    static void rd_kw_inplace_mul_indexed_##ctype(                             \
        rd_kw_type *target_kw, const int_vector_type *index_set,               \
        const rd_kw_type *mul_kw) {                                            \
        if (!rd_kw_assert_binary_##ctype(target_kw, mul_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *mul_data = (const ctype *)rd_kw_get_data_ref(mul_kw); \
            int set_size = int_vector_size(index_set);                         \
            const int *index_data = int_vector_get_const_ptr(index_set);       \
            int i;                                                             \
            for (i = 0; i < set_size; i++) {                                   \
                int index = index_data[i];                                     \
                target_data[index] *= mul_data[index];                         \
            }                                                                  \
        }                                                                      \
    }

RD_KW_TYPED_INPLACE_MUL_INDEXED(int)
RD_KW_TYPED_INPLACE_MUL_INDEXED(double)
RD_KW_TYPED_INPLACE_MUL_INDEXED(float)
#undef RD_KW_TYPED_INPLACE_MUL

void rd_kw_inplace_mul_indexed(rd_kw_type *target_kw,
                               const int_vector_type *index_set,
                               const rd_kw_type *mul_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_mul_indexed_float(target_kw, index_set, mul_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_mul_indexed_double(target_kw, index_set, mul_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_mul_indexed_int(target_kw, index_set, mul_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace mul not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_DIV(ctype)                                         \
    void rd_kw_inplace_div_##ctype(rd_kw_type *target_kw,                      \
                                   const rd_kw_type *div_kw) {                 \
        if (!rd_kw_assert_binary_##ctype(target_kw, div_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *div_data = (const ctype *)rd_kw_get_data_ref(div_kw); \
            int i;                                                             \
            for (i = 0; i < target_kw->size; i++)                              \
                target_data[i] /= div_data[i];                                 \
        }                                                                      \
    }
RD_KW_TYPED_INPLACE_DIV(int)
RD_KW_TYPED_INPLACE_DIV(double)
RD_KW_TYPED_INPLACE_DIV(float)
#undef RD_KW_TYPED_INPLACE_DIV

void rd_kw_inplace_div(rd_kw_type *target_kw, const rd_kw_type *div_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_div_float(target_kw, div_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_div_double(target_kw, div_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_div_int(target_kw, div_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace div not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

#define RD_KW_TYPED_INPLACE_DIV_INDEXED(ctype)                                 \
    static void rd_kw_inplace_div_indexed_##ctype(                             \
        rd_kw_type *target_kw, const int_vector_type *index_set,               \
        const rd_kw_type *div_kw) {                                            \
        if (!rd_kw_assert_binary_##ctype(target_kw, div_kw))                   \
            throw std::invalid_argument("type/size  mismatch");                \
        {                                                                      \
            ctype *target_data = (ctype *)rd_kw_get_data_ref(target_kw);       \
            const ctype *div_data = (const ctype *)rd_kw_get_data_ref(div_kw); \
            int set_size = int_vector_size(index_set);                         \
            const int *index_data = int_vector_get_const_ptr(index_set);       \
            int i;                                                             \
            for (i = 0; i < set_size; i++) {                                   \
                int index = index_data[i];                                     \
                target_data[index] /= div_data[index];                         \
            }                                                                  \
        }                                                                      \
    }

RD_KW_TYPED_INPLACE_DIV_INDEXED(int)
RD_KW_TYPED_INPLACE_DIV_INDEXED(double)
RD_KW_TYPED_INPLACE_DIV_INDEXED(float)
#undef RD_KW_TYPED_INPLACE_DIV

void rd_kw_inplace_div_indexed(rd_kw_type *target_kw,
                               const int_vector_type *index_set,
                               const rd_kw_type *div_kw) {
    rd_type_enum type = rd_kw_get_type(target_kw);
    switch (type) {
    case (RD_FLOAT_TYPE):
        rd_kw_inplace_div_indexed_float(target_kw, index_set, div_kw);
        break;
    case (RD_DOUBLE_TYPE):
        rd_kw_inplace_div_indexed_double(target_kw, index_set, div_kw);
        break;
    case (RD_INT_TYPE):
        rd_kw_inplace_div_indexed_int(target_kw, index_set, div_kw);
        break;
    default:
        throw std::invalid_argument(
            fmt::format("inplace div not implemented for type:{}",
                        rd_type_name(rd_kw_get_data_type(target_kw))));
    }
}

bool rd_kw_inplace_safe_div(rd_kw_type *target_kw, const rd_kw_type *divisor) {
    if (rd_kw_get_type(target_kw) != RD_FLOAT_TYPE)
        return false;

    if (rd_kw_get_type(divisor) != RD_INT_TYPE)
        return false;

    float *target_data = (float *)rd_kw_get_data_ref(target_kw);
    const int *div_data = (const int *)rd_kw_get_data_ref(divisor);
    for (int i = 0; i < target_kw->size; i++) {
        if (div_data[i] != 0)
            target_data[i] /= div_data[i];
    }

    return true;
}

#define KW_MAX_MIN(type)                                                       \
    {                                                                          \
        type *data = (type *)rd_kw_get_data_ref(rd_kw);                        \
        type max = data[0];                                                    \
        type min = data[0];                                                    \
        int i;                                                                 \
        for (i = 1; i < rd_kw_get_size(rd_kw); i++)                            \
            util_update_##type##_max_min(data[i], &max, &min);                 \
        memcpy(_max, &max, rd_type_get_sizeof_ctype(rd_kw->data_type));        \
        memcpy(_min, &min, rd_type_get_sizeof_ctype(rd_kw->data_type));        \
    }

void rd_kw_max_min(const rd_kw_type *rd_kw, void *_max, void *_min) {
    switch (rd_kw_get_type(rd_kw)) {
    case (RD_FLOAT_TYPE):
        KW_MAX_MIN(float);
        break;
    case (RD_DOUBLE_TYPE):
        KW_MAX_MIN(double);
        break;
    case (RD_INT_TYPE):
        KW_MAX_MIN(int);
        break;
    default:
        throw std::invalid_argument("invalid type for element sum");
    }
}

#define RD_KW_MAX_MIN(ctype)                                                   \
    void rd_kw_max_min_##ctype(const rd_kw_type *rd_kw, ctype *_max,           \
                               ctype *_min) {                                  \
        if (rd_kw->size < 1)                                                   \
            throw std::invalid_argument(                                       \
                "size of zero length array is undefined");                     \
        KW_MAX_MIN(ctype);                                                     \
    }

#define RD_KW_MAX(ctype)                                                       \
    ctype rd_kw_##ctype##_max(const rd_kw_type *rd_kw) {                       \
        ctype max, min;                                                        \
        rd_kw_max_min_##ctype(rd_kw, &max, &min);                              \
        return max;                                                            \
    }

#define RD_KW_MIN(ctype)                                                       \
    ctype rd_kw_##ctype##_min(const rd_kw_type *rd_kw) {                       \
        ctype max, min;                                                        \
        rd_kw_max_min_##ctype(rd_kw, &max, &min);                              \
        return min;                                                            \
    }

RD_KW_MAX_MIN(int)
RD_KW_MAX_MIN(float)
RD_KW_MAX_MIN(double)

RD_KW_MAX(int)
RD_KW_MAX(float)
RD_KW_MAX(double)

RD_KW_MIN(int)
RD_KW_MIN(float)
RD_KW_MIN(double)

#undef RD_KW_MAX
#undef RD_KW_MIN
#undef KW_MAX_MIN
#undef RD_KW_MAX_MIN

#define KW_SUM_INDEXED(type)                                                   \
    {                                                                          \
        const type *data = (const type *)rd_kw_get_data_ref(rd_kw);            \
        type sum = 0;                                                          \
        int size = int_vector_size(index_list);                                \
        const int *index_ptr = int_vector_get_const_ptr(index_list);           \
        for (int i = 0; i < size; i++)                                         \
            sum += data[index_ptr[i]];                                         \
        memcpy(_sum, &sum, rd_type_get_sizeof_ctype(rd_kw->data_type));        \
    }

void rd_kw_element_sum_indexed(const rd_kw_type *rd_kw,
                               const int_vector_type *index_list, void *_sum) {
    switch (rd_kw_get_type(rd_kw)) {
    case (RD_FLOAT_TYPE):
        KW_SUM_INDEXED(float);
        break;
    case (RD_DOUBLE_TYPE):
        KW_SUM_INDEXED(double);
        break;
    case (RD_INT_TYPE):
        KW_SUM_INDEXED(int);
        break;
    case (RD_BOOL_TYPE): {
        const bool *data = (const bool *)rd_kw_get_data_ref(rd_kw);
        const int *index_ptr = int_vector_get_const_ptr(index_list);
        const int size = int_vector_size(index_list);
        int sum = 0;
        for (int i = 0; i < size; i++)
            sum += (data[index_ptr[i]]);

        memcpy(_sum, &sum, sizeof sum);
    } break;
    default:
        throw std::invalid_argument("invalid type for element sum");
    }
}
#undef KW_SUM

#define KW_SUM(type)                                                           \
    {                                                                          \
        const type *data = (const type *)rd_kw_get_data_ref(rd_kw);            \
        type sum = 0;                                                          \
        for (int i = 0; i < rd_kw_get_size(rd_kw); i++)                        \
            sum += data[i];                                                    \
        memcpy(_sum, &sum, rd_type_get_sizeof_ctype(rd_kw->data_type));        \
    }

void rd_kw_element_sum(const rd_kw_type *rd_kw, void *_sum) {
    switch (rd_kw_get_type(rd_kw)) {
    case (RD_FLOAT_TYPE):
        KW_SUM(float);
        break;
    case (RD_DOUBLE_TYPE):
        KW_SUM(double);
        break;
    case (RD_INT_TYPE):
        KW_SUM(int);
        break;
    default:
        throw std::invalid_argument("invalid type for element sum");
    }
}
#undef KW_SUM

double rd_kw_element_sum_float(const rd_kw_type *rd_kw) {
    float float_sum;
    double double_sum;
    void *sum_ptr = NULL;

    if (rd_type_is_double(rd_kw->data_type))
        sum_ptr = &double_sum;
    else if (rd_type_is_float(rd_kw->data_type))
        sum_ptr = &float_sum;
    else
        throw std::invalid_argument("invalid type:");

    rd_kw_element_sum(rd_kw, sum_ptr);

    if (rd_type_is_double(rd_kw->data_type))
        return double_sum;
    else if (rd_type_is_float(rd_kw->data_type))
        return float_sum;
    else
        return 0;
}

int rd_kw_element_sum_int(const rd_kw_type *rd_kw) {
    int int_sum;
    rd_kw_element_sum(rd_kw, &int_sum);

    return int_sum;
}

#define RD_KW_FPRINTF_DATA(ctype)                                              \
    static void rd_kw_fprintf_data_##ctype(const rd_kw_type *rd_kw,            \
                                           const char *fmt, FILE *stream) {    \
        const ctype *data = (const ctype *)rd_kw->data;                        \
        int i;                                                                 \
        for (i = 0; i < rd_kw->size; i++)                                      \
            fprintf(stream, fmt, data[i]);                                     \
    }

RD_KW_FPRINTF_DATA(int)
RD_KW_FPRINTF_DATA(float)
RD_KW_FPRINTF_DATA(double)
RD_KW_FPRINTF_DATA(bool)
#undef RD_KW_FPRINTF_DATA

static void rd_kw_fprintf_data_string(const rd_kw_type *rd_kw, const char *fmt,
                                      FILE *stream) {
    int i;
    for (i = 0; i < rd_kw->size; i++)
        fprintf(stream, fmt,
                &rd_kw->data[i * rd_type_get_sizeof_ctype(rd_kw->data_type)]);
}

void rd_kw_fprintf_data(const rd_kw_type *rd_kw, const char *fmt,
                        FILE *stream) {
    if (rd_type_is_double(rd_kw->data_type))
        rd_kw_fprintf_data_double(rd_kw, fmt, stream);
    else if (rd_type_is_float(rd_kw->data_type))
        rd_kw_fprintf_data_float(rd_kw, fmt, stream);
    else if (rd_type_is_int(rd_kw->data_type))
        rd_kw_fprintf_data_int(rd_kw, fmt, stream);
    else if (rd_type_is_bool(rd_kw->data_type))
        rd_kw_fprintf_data_bool(rd_kw, fmt, stream);
    else if (rd_type_is_char(rd_kw->data_type) ||
             rd_type_is_string(rd_kw->data_type))
        rd_kw_fprintf_data_string(rd_kw, fmt, stream);
}

static bool rd_kw_elm_equal_numeric__(const rd_kw_type *rd_kw1,
                                      const rd_kw_type *rd_kw2, int offset,
                                      double abs_epsilon, double rel_epsilon) {
    double v1 = rd_kw_iget_as_double(rd_kw1, offset);
    double v2 = rd_kw_iget_as_double(rd_kw2, offset);
    return util_double_approx_equal__(v1, v2, rel_epsilon, abs_epsilon);
}

static bool rd_kw_elm_equal__(const rd_kw_type *rd_kw1,
                              const rd_kw_type *rd_kw2, int offset) {
    if (offset < 0)
        throw std::invalid_argument(
            fmt::format("offset was negative: {}", offset));
    size_t data_offset = static_cast<size_t>(offset) *
                         rd_type_get_sizeof_ctype(rd_kw1->data_type);
    int cmp = memcmp(&rd_kw1->data[data_offset], &rd_kw2->data[data_offset],
                     rd_type_get_sizeof_ctype(rd_kw1->data_type));
    if (cmp == 0)
        return true;
    else
        return false;
}

int rd_kw_first_different(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2,
                          int offset, double abs_epsilon, double rel_epsilon) {
    if (!rd_kw_size_and_type_equal(rd_kw1, rd_kw2))
        throw std::invalid_argument("sorry invalid comparison");

    if (offset >= rd_kw_get_size(rd_kw1))
        throw std::invalid_argument("sorry - invalid offset value");

    {
        bool numeric_compare = false;

        if (((abs_epsilon > 0) || (rel_epsilon > 0)) &&
            ((rd_kw_get_type(rd_kw1) == RD_FLOAT_TYPE) ||
             (rd_kw_get_type(rd_kw1) == RD_DOUBLE_TYPE)))
            numeric_compare = true;
        {
            int index = offset;

            while (true) {
                bool equal =
                    (numeric_compare)
                        ? rd_kw_elm_equal_numeric__(rd_kw1, rd_kw2, index,
                                                    abs_epsilon, rel_epsilon)
                        : rd_kw_elm_equal__(rd_kw1, rd_kw2, index);
                if (!equal)
                    break;

                index++;
                if (index == rd_kw_get_size(rd_kw1))
                    break;
            }

            return index;
        }
    }
}

#include "rd_kw_functions.cpp"
