#pragma once
#include <ctime>
#include <cstdint>
#include <cstdlib>

#include <new>
#include <filesystem>
#include <system_error>
#include <memory>
#include <string>
#include <string_view>

#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/parser.hpp>

#include <resdata/rd_type.hpp>

enum class FileType : int {
    OTHER = 0,
    RESTART = 1,
    UNIFIED_RESTART = 2,
    SUMMARY = 4,
    UNIFIED_SUMMARY = 8,
    SUMMARY_HEADER = 16,
    GRID = 32,
    EGRID = 64,
    INIT = 128,
    RFT = 256,
    DATA = 512
};

#ifdef __cplusplus
extern "C" {
#endif

/*
    This enum enumerates the four different ways summary and restart information
    can be stored.
  */

typedef enum {
    RD_INVALID_STORAGE = 0,
    RD_BINARY_UNIFIED = 1,
    RD_FORMATTED_UNIFIED = 2,
    RD_BINARY_NON_UNIFIED = 4,
    RD_FORMATTED_NON_UNIFIED = 8
} rd_storage_enum;

/*
  The resdata library has been built and tested 99.5% with ECLIPSE100
  as context, but in thye gravity code there is some very limited
  functionality related to ECLIPSE100 versus ECLIPSE300 functionality.

  Observe that numerical values found as part of the INTEHAD keyword
  differ from these values, and are found in the rd_kw_magic.h
  header.
*/

typedef enum {
    ECLIPSE_UNDEFINED = 0,
    ECLIPSE100 = 1,
    ECLIPSE300 = 2,
    ECLIPSE300_THERMAL = 3,
    INTERSECT = 4,
    FRONTSIM = 5
} rd_version_enum;

/*
  Observe that the numerical enum VALUES matches those found in item
  14 in the INTEHEAD keyword in the INIT files; i.e. the
  distribution of numerical values 1,2,4 can NOT BE CHANGED.

  The function rd_get_phase_name() can be used to lookup a
  string name from an enum value.

  The phases in a simulation will typically be a sum of these
  fundamental phases, and represented as an integer.
*/

typedef enum {
    RD_OIL_PHASE = 1,
    RD_GAS_PHASE = 2,
    RD_WATER_PHASE = 4
} rd_phase_enum;

#define RD_PHASE_ENUM_DEFS                                                     \
    {.value = 1, .name = "RD_OIL_PHASE"},                                      \
        {.value = 2, .name = "RD_GAS_PHASE"},                                  \
        {.value = 4, .name = "RD_WATER_PHASE"}
#define RD_PHASE_ENUM_SIZE 3

typedef enum {
    RD_METRIC_UNITS = 1,
    RD_FIELD_UNITS = 2,
    RD_LAB_UNITS = 3,
    RD_PVT_M_UNITS = 4
} ert_rd_unit_enum;

// For unformatted files:
#define RD_BOOL_TRUE_INT                                                       \
    -1 // Binary representation: 11111111  11111111  11111111  1111111
#define RD_BOOL_FALSE_INT                                                      \
    0 // Binary representation: 00000000  00000000  00000000  0000000
#define RD_COMMENT_STRING "--"
#define RD_COMMENT_CHAR '-' // Need to consecutive to make an ECLIPSE comment
#define RD_DATA_TERMINATION "/"

#ifdef __cplusplus
}
#endif

extern "C" int rd_filename_report_nr(const char *);
extern "C" FileType rd_get_file_type(const char *filename, bool *fmt_file,
                                     int *report_nr);
extern "C" time_t rd_get_start_date(const char *);
extern "C" int rd_get_num_cpu(const char *data_file);
bool rd_fmt_file(const char *filename, bool *__fmt_file);
int rd_fname_report_cmp(const void *f1, const void *f2);
time_t rd_make_date(int mday, int month, int year);
time_t rd_make_date__(int mday, int month, int year, int *year_offset);
time_t rd_make_datetime(int sec, int min, int hour, int mday, int month,
                        int year);

const char *rd_get_phase_name(rd_phase_enum phase);

int rd_select_filelist(const char *path, const char *base, FileType file_type,
                       bool fmt_file, stringlist_type *filelist);
void rd_set_datetime_values(time_t t, int *sec, int *min, int *hour, int *mday,
                            int *month, int *year);
bool rd_path_access(const char *rd_case);
namespace rd {
std::filesystem::path filename(std::filesystem::path path, FileType,
                               bool fmt_file, int report_nr = -1);

inline bool try_exists(std::filesystem::path p) noexcept {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

template <typename T>
std::unique_ptr<T[], void (*)(void *)> checked_malloc(size_t num) {
    if (sizeof(T) == 0 || num == 0)
        return {nullptr, std::free};
    if (num > SIZE_MAX / sizeof(T))
        throw std::bad_alloc{};

    T *ptr = static_cast<T *>(std::malloc(num * sizeof(T)));

    if (ptr == nullptr)
        throw std::bad_alloc{};

    return {ptr, std::free};
}

template <typename T>
std::unique_ptr<T[], void (*)(void *)> checked_calloc(size_t num) {
    if (sizeof(T) == 0 || num == 0)
        return {nullptr, std::free};
    if (num > SIZE_MAX / sizeof(T))
        throw std::bad_alloc{};

    T *ptr = static_cast<T *>(std::calloc(num, sizeof(T)));

    if (ptr == nullptr)
        throw std::bad_alloc{};

    return {ptr, std::free};
}

template <typename T>
void checked_realloc(std::unique_ptr<T[], void (*)(void *)> &ptr,
                     size_t new_element_count) {
    if (new_element_count == 0 || sizeof(T) == 0) {
        ptr.reset();
        return;
    }

    if (new_element_count > SIZE_MAX / sizeof(T))
        throw std::bad_alloc{};

    T *raw_ptr = ptr.release();
    void *new_raw_ptr = std::realloc(raw_ptr, new_element_count * sizeof(T));
    if (new_raw_ptr == nullptr) {
        ptr.reset(raw_ptr);
        throw std::bad_alloc{};
    }
    ptr.reset(static_cast<T *>(new_raw_ptr));
}
inline std::string strip_spaces(std::string_view s) {
    auto first = s.find_first_not_of(' ');
    if (first == std::string_view::npos)
        return {};
    auto last = s.find_last_not_of(' ');
    return std::string(s.substr(first, last - first + 1));
}
} // namespace rd
