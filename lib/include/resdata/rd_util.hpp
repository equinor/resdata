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
#include <vector>

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

// For unformatted files:
#define RD_BOOL_TRUE_INT                                                       \
    -1 // Binary representation: 11111111  11111111  11111111  1111111
#define RD_BOOL_FALSE_INT                                                      \
    0 // Binary representation: 00000000  00000000  00000000  0000000
#define RD_COMMENT_STRING "--"
#define RD_COMMENT_CHAR '-' // Need to consecutive to make an ECLIPSE comment
#define RD_DATA_TERMINATION "/"

enum class UnitSystem { METRIC = 1, FIELD = 2, LAB = 3, PVT_M = 4 };
/* Observe that the numerical enum VALUES matches those found in item
  14 in the INTEHEAD keyword in the INIT files; i.e. the
  distribution of numerical values 1,2,4 can NOT BE CHANGED.

  The function rd_get_phase_name() can be used to lookup a
  string name from an enum value.

  The phases in a simulation will typically be a sum of these
  fundamental phases, and represented as an integer.
*/
enum class Phase { OIL = 1, GAS = 2, WATER = 4 };

int rd_filename_report_nr(const char *);
FileType rd_get_file_type(const char *filename, bool *fmt_file, int *report_nr);
time_t rd_get_start_date(const char *);
int rd_get_num_cpu(const char *data_file);
bool rd_fmt_file(const char *filename, bool *__fmt_file);
int rd_fname_report_cmp(const void *f1, const void *f2);
time_t rd_make_date(int mday, int month, int year);
time_t rd_make_date__(int mday, int month, int year, int *year_offset);
time_t rd_make_datetime(int sec, int min, int hour, int mday, int month,
                        int year);

const char *rd_get_phase_name(Phase phase);

std::vector<std::string> rd_select_filelist(const char *path,
                                            std::string_view base,
                                            FileType file_type, bool fmt_file);
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

/** Compares two strings in "natural" order: maximal runs of digits are
   compared by numeric value, so that "S9" sorts before "S10", while all
   other characters are compared bytewise.

   Returns a negative value if a < b, 0 if a == b and a positive value if
   a > b */
int natural_compare(std::string_view a, std::string_view b);

inline bool natural_less(std::string_view a, std::string_view b) {
    return natural_compare(a, b) < 0;
}
} // namespace rd
std::vector<std::string> select_matching_files(const std::string &path,
                                               const std::string &file_pattern);
