#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <new>
#include <system_error>
#include <stdexcept>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <ert/util/ert_api_config.hpp>

#ifdef ERT_HAVE_GLOB
#include <glob.h>
#else
#include <Windows.h>
#endif

#include <fmt/format.h>

#include <ert/util/util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/parser.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_type.hpp>

#define RD_PHASE_NAME_OIL                                                      \
    "SOIL" // SHould match the keywords found in restart file
#define RD_PHASE_NAME_WATER "SWAT"
#define RD_PHASE_NAME_GAS "SGAS"

#define RD_OTHER_FILE_FMT_PATTERN "*"
#define RD_UNIFIED_RESTART_FMT_PATTERN "FUNRST"
#define RD_UNIFIED_SUMMARY_FMT_PATTERN "FUNSMRY"
#define RD_GRID_FMT_PATTERN "FGRID"
#define RD_EGRID_FMT_PATTERN "FEGRID"
#define RD_INIT_FMT_PATTERN "FINIT"
#define RD_RFT_FMT_PATTERN "FRFT"
#define RD_DATA_PATTERN "DATA"

#define RD_OTHER_FILE_UFMT_PATTERN "*"
#define RD_UNIFIED_RESTART_UFMT_PATTERN "UNRST"
#define RD_UNIFIED_SUMMARY_UFMT_PATTERN "UNSMRY"
#define RD_GRID_UFMT_PATTERN "GRID"
#define RD_EGRID_UFMT_PATTERN "EGRID"
#define RD_INIT_UFMT_PATTERN "INIT"
#define RD_RFT_UFMT_PATTERN "RFT"

namespace fs = std::filesystem;

const char *rd_get_phase_name(Phase phase) {
    switch (phase) {
    case (Phase::OIL):
        return RD_PHASE_NAME_OIL;
        break;
    case (Phase::WATER):
        return RD_PHASE_NAME_WATER;
        break;
    case (Phase::GAS):
        return RD_PHASE_NAME_GAS;
        break;
    default:
        throw std::invalid_argument(
            std::string(__func__) + ": phase enum value: " +
            std::to_string(static_cast<int>(phase)) + " not recognized");
    }
}

int rd_filename_report_nr(const char *filename) {
    int report_nr = -1;
    rd_get_file_type(filename, NULL, &report_nr);
    return report_nr;
}

/*
 We accept mixed lowercase/uppercase Eclipse file extensions even if Eclipse itself does not accept them.
*/
static FileType rd_inspect_extension(const char *ext, bool *_fmt_file,
                                     int *_report_nr) {
    FileType file_type = FileType::OTHER;
    bool fmt_file = true;
    int report_nr = -1;
    char *upper_ext = util_alloc_strupr_copy(ext);
    if (strcmp(upper_ext, "UNRST") == 0) {
        file_type = FileType::UNIFIED_RESTART;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FUNRST") == 0) {
        file_type = FileType::UNIFIED_RESTART;
        fmt_file = true;
    } else if (strcmp(upper_ext, "UNSMRY") == 0) {
        file_type = FileType::UNIFIED_SUMMARY;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FUNSMRY") == 0) {
        file_type = FileType::UNIFIED_SUMMARY;
        fmt_file = true;
    } else if (strcmp(upper_ext, "SMSPEC") == 0) {
        file_type = FileType::SUMMARY_HEADER;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FSMSPEC") == 0) {
        file_type = FileType::SUMMARY_HEADER;
        fmt_file = true;
    } else if (strcmp(upper_ext, "GRID") == 0) {
        file_type = FileType::GRID;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FGRID") == 0) {
        file_type = FileType::GRID;
        fmt_file = true;
    } else if (strcmp(upper_ext, "EGRID") == 0) {
        file_type = FileType::EGRID;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FEGRID") == 0) {
        file_type = FileType::EGRID;
        fmt_file = true;
    } else if (strcmp(upper_ext, "INIT") == 0) {
        file_type = FileType::INIT;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FINIT") == 0) {
        file_type = FileType::INIT;
        fmt_file = true;
    } else if (strcmp(upper_ext, "FRFT") == 0) {
        file_type = FileType::RFT;
        fmt_file = true;
    } else if (strcmp(upper_ext, "RFT") == 0) {
        file_type = FileType::RFT;
        fmt_file = false;
    } else if (strcmp(upper_ext, "DATA") == 0) {
        file_type = FileType::DATA;
        fmt_file = true; /* Not really relevant ... */
    } else {
        switch (upper_ext[0]) {
        case ('X'):
            file_type = FileType::RESTART;
            fmt_file = false;
            break;
        case ('F'):
            file_type = FileType::RESTART;
            fmt_file = true;
            break;
        case ('S'):
            file_type = FileType::SUMMARY;
            fmt_file = false;
            break;
        case ('A'):
            file_type = FileType::SUMMARY;
            fmt_file = true;
            break;
        default:
            file_type = FileType::OTHER;
        }
        if (file_type != FileType::OTHER)
            if (!util_sscanf_int(&upper_ext[1], &report_nr))
                file_type = FileType::OTHER;
    }

    if (_fmt_file != NULL)
        *_fmt_file = fmt_file;

    if (_report_nr != NULL)
        *_report_nr = report_nr;

    free(upper_ext);
    return file_type;
}

/**
  This function takes an eclipse filename as input - looks at the
  extension, and uses that to determine the type of file. In addition
  to the fundamental type, it is also determined whether the file is
  formatted or not, and in the case of summary/restart files, which
  report number this corresponds to.
*/

FileType rd_get_file_type(const char *filename, bool *fmt_file,
                          int *report_nr) {
    char *ext = (char *)strrchr(filename, '.');
    if (ext == NULL)
        return FileType::OTHER;

    return rd_inspect_extension(&ext[1], fmt_file, report_nr);
}

static const char *rd_get_file_pattern(FileType file_type, bool fmt_file) {
    if (fmt_file) {
        switch (file_type) {
        case (FileType::OTHER):
            return RD_OTHER_FILE_FMT_PATTERN; /* '*' */
            break;
        case (FileType::UNIFIED_RESTART):
            return RD_UNIFIED_RESTART_FMT_PATTERN;
            break;
        case (FileType::UNIFIED_SUMMARY):
            return RD_UNIFIED_SUMMARY_FMT_PATTERN;
            break;
        case (FileType::GRID):
            return RD_GRID_FMT_PATTERN;
            break;
        case (FileType::EGRID):
            return RD_EGRID_FMT_PATTERN;
            break;
        case (FileType::INIT):
            return RD_INIT_FMT_PATTERN;
            break;
        case (FileType::RFT):
            return RD_RFT_FMT_PATTERN;
            break;
        case (FileType::DATA):
            return RD_DATA_PATTERN;
            break;
        default:
            util_abort("%s: No pattern defined for til_type:%d \n", __func__,
                       file_type);
        }
    } else {
        switch (file_type) {
        case (FileType::OTHER):
            return RD_OTHER_FILE_UFMT_PATTERN; /* '*' */
            break;
        case (FileType::UNIFIED_RESTART):
            return RD_UNIFIED_RESTART_UFMT_PATTERN;
            break;
        case (FileType::UNIFIED_SUMMARY):
            return RD_UNIFIED_SUMMARY_UFMT_PATTERN;
            break;
        case (FileType::GRID):
            return RD_GRID_UFMT_PATTERN;
            break;
        case (FileType::EGRID):
            return RD_EGRID_UFMT_PATTERN;
            break;
        case (FileType::INIT):
            return RD_INIT_UFMT_PATTERN;
            break;
        case (FileType::RFT):
            return RD_RFT_UFMT_PATTERN;
            break;
        case (FileType::DATA):
            return RD_DATA_PATTERN;
            break;
        default:
            util_abort("%s: No pattern defined for til_type:%d \n", __func__,
                       file_type);
        }
    }
}

namespace rd {
int natural_compare(std::string_view a, std::string_view b) {
    size_t i = 0;
    size_t j = 0;

    while (i < a.size() && j < b.size()) {
        unsigned char ca = static_cast<unsigned char>(a[i]);
        unsigned char cb = static_cast<unsigned char>(b[j]);

        if (std::isdigit(ca) && std::isdigit(cb)) {
            /* Skip leading zeros. */
            size_t start_a = a.find_first_not_of('0', i);
            if (start_a == std::string_view::npos)
                start_a = a.size();
            size_t start_b = b.find_first_not_of('0', j);
            if (start_b == std::string_view::npos)
                start_b = b.size();

            size_t end_a = start_a;
            while (end_a < a.size() &&
                   std::isdigit(static_cast<unsigned char>(a[end_a])))
                end_a++;

            size_t end_b = start_b;
            while (end_b < b.size() &&
                   std::isdigit(static_cast<unsigned char>(b[end_b])))
                end_b++;

            std::string_view num_a = a.substr(start_a, end_a - start_a);
            std::string_view num_b = b.substr(start_b, end_b - start_b);

            if (num_a.size() != num_b.size())
                return num_a.size() < num_b.size() ? -1 : 1;

            int cmp = num_a.compare(num_b);
            if (cmp != 0)
                return cmp < 0 ? -1 : 1;

            /* Numbers are equal - continue comparing after end.*/
            i = end_a;
            j = end_b;
            continue;
        }

        if (ca != cb)
            return ca < cb ? -1 : 1;

        i++;
        j++;
    }

    if (i < a.size())
        return 1;
    if (j < b.size())
        return -1;

    /* Equal under the natural ordering. Fall back to a plain bytewise
       comparison so that strings differing only in leading zeros (e.g.
       "S007" and "S7") still get a stable, total order. */
    int cmp = a.compare(b);
    if (cmp == 0)
        return 0;
    return cmp < 0 ? -1 : 1;
}
} // namespace rd

/** True if the file name part of @input_base is written in lower case, that
   is: it holds at least one lower case letter and no upper case letter. */
static bool base_is_lower_case(std::string_view input_base) {
    size_t last_sep = input_base.rfind(UTIL_PATH_SEP_CHAR);

    std::string_view base = (last_sep != std::string_view::npos)
                                ? input_base.substr(last_sep + 1)
                                : input_base;

    bool has_lower = false;
    for (char c : base) {
        if (std::isupper(static_cast<unsigned char>(c)))
            return false;
        if (std::islower(static_cast<unsigned char>(c)))
            has_lower = true;
    }

    return has_lower;
}

namespace rd {
/**
 * Given the path to a case, eg. test-data/local/eclipse/SIMPLE,
 * get path to the file of the given type, e.g for FileType::EGRID
 * you get test-data/local/eclipse/SIMPLE.EGRID.
 *
 * @report_nr is only used by the file types which carry a report number in
 * the extension. The extension is lower cased whenever the base name is, so
 * that a case named 'simple' gets 'simple.egrid'.
 */
fs::path filename(fs::path casepath, FileType file_type, bool fmt_file,
                  int report_nr) {
    const std::string base = casepath.filename().string();

    std::string ext;
    switch (file_type) {
    case (FileType::RESTART):
        ext = fmt::format("{}{:04d}", fmt_file ? 'F' : 'X', report_nr);
        break;

    case (FileType::UNIFIED_RESTART):
        ext = fmt_file ? "FUNRST" : "UNRST";
        break;

    case (FileType::SUMMARY):
        ext = fmt::format("{}{:04d}", fmt_file ? 'A' : 'S', report_nr);
        break;

    case (FileType::UNIFIED_SUMMARY):
        ext = fmt_file ? "FUNSMRY" : "UNSMRY";
        break;

    case (FileType::SUMMARY_HEADER):
        ext = fmt_file ? "FSMSPEC" : "SMSPEC";
        break;

    case (FileType::GRID):
        ext = fmt_file ? "FGRID" : "GRID";
        break;

    case (FileType::EGRID):
        ext = fmt_file ? "FEGRID" : "EGRID";
        break;

    case (FileType::INIT):
        ext = fmt_file ? "FINIT" : "INIT";
        break;

    case (FileType::RFT):
        ext = fmt_file ? "FRFT" : "RFT";
        break;

    case (FileType::DATA):
        ext = "DATA";
        break;

    default:
        throw std::invalid_argument("Invalid input file_type to filename");
    }

    if (base_is_lower_case(base))
        for (char &c : ext)
            c = tolower(static_cast<unsigned char>(c));

    return casepath.parent_path() / (base + "." + ext);
}
}; // namespace rd

/**
   This function assumes that:

    o Both files are of the same type (i.e. both summary files) (this
      is not checked for).

    o Both files are of type WITH a nnnn number at the end, the
      function will fail hard in rd_filename_report_nr() if
      this is not the case.

*/

int rd_fname_report_cmp(const void *f1, const void *f2) {

    int t1 = rd_filename_report_nr((const char *)f1);
    int t2 = rd_filename_report_nr((const char *)f2);

    if (t1 < t2)
        return -1;
    else if (t1 > t2)
        return 1;
    else
        return 0;
}

/** True if @filename is exactly "@base.<leading_char>NNNN", where NNNN is the
   four digit report number.*/
static bool numeric_extension_predicate(const char *filename, const char *base,
                                        const char leading_char) {
    const size_t base_length = strlen(base);

    /* The extension is a '.', the leading character and four digits. */
    if (strlen(filename) != base_length + 6)
        return false;

    if (strncmp(filename, base, base_length) != 0)
        return false;

    const char *ext_start = filename + base_length;
    if (ext_start[0] != '.')
        return false;

    if (ext_start[1] != leading_char)
        return false;

    for (int i = 0; i < 4; i++)
        if (!isdigit(static_cast<unsigned char>(ext_start[i + 2])))
            return false;

    return true;
}

static bool summary_UPPERCASE_ASCII(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 'A');
}

static bool summary_UPPERCASE_BINARY(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 'S');
}

static bool summary_lowercase_ASCII(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 'a');
}

static bool summary_lowercase_BINARY(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 's');
}

static bool restart_UPPERCASE_ASCII(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 'F');
}

static bool restart_UPPERCASE_BINARY(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 'X');
}

static bool restart_lowercase_ASCII(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 'f');
}

static bool restart_lowercase_BINARY(const char *filename, const void *base) {
    return numeric_extension_predicate(filename, (const char *)base, 'x');
}

/** Lists the entries of @path (or the current directory if @path is empty)
   for which @predicate holds.

   The returned names are prefixed with @path, so an empty @path yields bare
   file names relative to the current directory.

   A @path which does not exist, or is not a directory, yields an empty
   result. Any other error - a permission or I/O failure - raises
   fs::filesystem_error. */
static std::vector<std::string> select_files(const fs::path &path,
                                             file_pred_ftype *predicate,
                                             const void *pred_arg) {
    const fs::path scan_dir = path.empty() ? fs::current_path() : path;

    std::error_code ec;
    if (!fs::is_directory(scan_dir, ec))
        return {};

    std::vector<std::string> result;
    for (const auto &entry : fs::directory_iterator(scan_dir)) {
        const std::string name = entry.path().filename().string();
        if (predicate && !predicate(name.c_str(), pred_arg))
            continue;

        result.push_back((path / name).string());
    }

    return result;
}

#ifdef ERT_HAVE_GLOB
namespace {
/* Calls globfree() on scope exit.*/
class GlobGuard {
public:
    explicit GlobGuard(glob_t &glob_result) : m_glob(glob_result) {}
    ~GlobGuard() { globfree(&m_glob); }

    GlobGuard(const GlobGuard &) = delete;
    GlobGuard &operator=(const GlobGuard &) = delete;

private:
    glob_t &m_glob;
};
} // namespace

/** Select file/path names matching a pattern. */
static std::vector<std::string> select_matching(const char *pattern) {
    glob_t glob_result{};
    GlobGuard guard(glob_result);

    int glob_status = glob(pattern, 0, NULL, &glob_result);

    /* No match is a normal outcome and yields an empty result. */
    if (glob_status == GLOB_NOMATCH)
        return {};

    if (glob_status == GLOB_NOSPACE)
        throw std::bad_alloc{};

    if (glob_status != 0)
        throw std::runtime_error(
            fmt::format("glob() failed with status {} for pattern '{}'",
                        glob_status, pattern));

    std::vector<std::string> names;
    names.reserve(glob_result.gl_pathc);
    for (size_t i = 0; i < glob_result.gl_pathc; i++)
        names.emplace_back(glob_result.gl_pathv[i]);

    return names;
}
#else
namespace {
/* Calls FindClose() on scope exit. */
class FindHandleGuard {
public:
    explicit FindHandleGuard(HANDLE handle) : m_handle(handle) {}
    ~FindHandleGuard() { FindClose(m_handle); }

    FindHandleGuard(const FindHandleGuard &) = delete;
    FindHandleGuard &operator=(const FindHandleGuard &) = delete;

private:
    HANDLE m_handle;
};
} // namespace
#endif

std::vector<std::string>
select_matching_files(const std::string &path,
                      const std::string &file_pattern) {
    const fs::path dir{path};

#ifdef ERT_HAVE_GLOB
    return select_matching((dir / file_pattern).string().c_str());
#else
    WIN32_FIND_DATA file_data;
    HANDLE file_handle =
        FindFirstFile((dir / file_pattern).string().c_str(), &file_data);

    if (file_handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
            return {};

        throw std::runtime_error(
            fmt::format("FindFirstFile() failed with error {} for pattern '{}'",
                        error, (dir / file_pattern).string()));
    }

    FindHandleGuard guard(file_handle);

    std::vector<std::string> names;
    do {
        names.push_back((dir / file_data.cFileName).string());
    } while (FindNextFile(file_handle, &file_data) != 0);

    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_FILES)
        throw std::runtime_error(fmt::format(
            "FindNextFile() failed with error {} while listing '{}'", error,
            dir.string()));

    return names;
#endif
}

static std::vector<std::string>
rd_select_predicate_filelist(const char *path, std::string_view base,
                             FileType file_type, bool fmt_file,
                             bool upper_case) {
    file_pred_ftype *predicate = NULL;
    fs::path tmp;
    if (path && strlen(path))
        tmp = std::string(path) + UTIL_PATH_SEP_STRING + std::string(base);
    else
        tmp = base;

    std::string pure_base = tmp.stem().string();
    fs::path full_path = tmp.parent_path();

    if (file_type == FileType::SUMMARY) {
        if (fmt_file) {
            if (upper_case)
                predicate = summary_UPPERCASE_ASCII;
            else
                predicate = summary_lowercase_ASCII;
        } else {
            if (upper_case)
                predicate = summary_UPPERCASE_BINARY;
            else
                predicate = summary_lowercase_BINARY;
        }
    } else if (file_type == FileType::RESTART) {
        if (fmt_file) {
            if (upper_case)
                predicate = restart_UPPERCASE_ASCII;
            else
                predicate = restart_lowercase_ASCII;
        } else {
            if (upper_case)
                predicate = restart_UPPERCASE_BINARY;
            else
                predicate = restart_lowercase_BINARY;
        }
    } else
        throw std::invalid_argument(
            "select_predicate_filelist called with wrong file type");

    auto filelist = select_files(full_path, predicate, pure_base.c_str());
    std::sort(filelist.begin(), filelist.end(),
              [](const std::string &a, const std::string &b) {
                  return rd_fname_report_cmp(a.c_str(), b.c_str()) < 0;
              });
    return filelist;
}

/** Scans the directory @path (or the current directory if @path == NULL) for
   the files of type @file_type belonging to the case @base.

   The @base is matched literally and may contain shell wildcards, so the
   pattern applied is simply "@base.<extension>":

     base = "CASE"  file_type = FileType::EGRID  ->  "CASE.EGRID"
     base = "*"     file_type = FileType::EGRID  ->  "*.EGRID"
     base = ""      file_type = FileType::EGRID  ->  ".EGRID"

   FileType::OTHER uses '*' as its extension, so base = "CASE" gives "CASE.*"
   and files which do not originate from the simulator are then included too.

   FileType::SUMMARY and FileType::RESTART are matched without a glob, since
   their extension is the report number rather than a fixed string. The
   pattern is the same, but @base is then matched literally and wildcards in
   it are matched exactly against the filename.*/
std::vector<std::string> rd_select_filelist(const char *path,
                                            std::string_view base,
                                            FileType file_type, bool fmt_file) {

    bool lower_case = base_is_lower_case(base);
    if (file_type == FileType::SUMMARY || file_type == FileType::RESTART)
        return rd_select_predicate_filelist(path, base, file_type, fmt_file,
                                            !lower_case);

    std::string ext_pattern{rd_get_file_pattern(file_type, fmt_file)};
    if (lower_case)
        for (char &c : ext_pattern)
            c = tolower(static_cast<unsigned char>(c));

    std::string pattern = std::string(base) + "." + ext_pattern;

    return select_matching_files(path ? path : "", pattern);
}

bool rd_fmt_file(const char *filename, bool *__fmt_file) {
    /*const int min_size = 32768;*/
    const int min_size = 256; /* Very small */

    int report_nr;
    FileType file_type;
    bool status = true;
    bool fmt_file = 0;

    if (util_file_exists(filename)) {
        file_type = rd_get_file_type(filename, &fmt_file, &report_nr);
        if (file_type == FileType::OTHER) {
            if (util_file_size(filename) > min_size)
                fmt_file = util_fmt_bit8(filename);
            else
                status = false; // Do not know ??
        }
    } else {
        file_type = rd_get_file_type(filename, &fmt_file, &report_nr);
        if (file_type == FileType::OTHER)
            status = false; // Do not know ??
    }

    *__fmt_file = fmt_file;
    return status;
}

/**
   Will return -1 for an unrecognized month name.
*/

static int rd_get_month_nr__(const char *_month_name) {
    int month_nr = -1;
    char *month_name = util_alloc_string_copy(_month_name);
    util_strupr(month_name);

    if (strncmp(month_name, "JAN", 3) == 0)
        month_nr = 1;
    else if (strncmp(month_name, "FEB", 3) == 0)
        month_nr = 2;
    else if (strncmp(month_name, "MAR", 3) == 0)
        month_nr = 3;
    else if (strncmp(month_name, "APR", 3) == 0)
        month_nr = 4;
    else if (strncmp(month_name, "MAI", 3) == 0)
        month_nr = 5;
    else if (strncmp(month_name, "MAY", 3) == 0)
        month_nr = 5;
    else if (strncmp(month_name, "JUN", 3) == 0)
        month_nr = 6;
    else if (strncmp(month_name, "JUL", 3) == 0)
        month_nr = 7;
    else if (strncmp(month_name, "JLY", 3) == 0) /* ECLIPSE ambigus on July. */
        month_nr = 7;
    else if (strncmp(month_name, "AUG", 3) == 0)
        month_nr = 8;
    else if (strncmp(month_name, "SEP", 3) == 0)
        month_nr = 9;
    else if (strncmp(month_name, "OCT", 3) == 0)
        month_nr = 10;
    else if (strncmp(month_name, "OKT", 3) == 0)
        month_nr = 10;
    else if (strncmp(month_name, "NOV", 3) == 0)
        month_nr = 11;
    else if (strncmp(month_name, "DEC", 3) == 0)
        month_nr = 12;
    else if (strncmp(month_name, "DES", 3) == 0)
        month_nr = 12;
    free(month_name);
    return month_nr;
}

static int rd_get_month_nr(const char *month_name) {
    int month_nr = rd_get_month_nr__(month_name);
    if (month_nr < 0)
        util_abort("%s: %s not a valid month name - aborting \n", __func__,
                   month_name);

    return month_nr;
}

/*
    The parsing of the data file has room for improvement, (or should
    be removed?).

    ECLIPSE100 has default date: 1. of january 1983.
    ECLIPSE300 has default date: 1. of january 1990.

*/

time_t rd_get_start_date(const char *data_file) {
    basic_parser_type *parser =
        basic_parser_alloc(" \t\r\n", "\"\'", NULL, NULL, "--", "\n");
    time_t start_date = -1;
    FILE *stream = util_fopen(data_file, "r");
    char *buffer;

    if (!basic_parser_fseek_string(parser, stream, "START", true,
                                   true)) /* Seeks case insensitive. */
        util_abort("%s: sorry - could not find START in DATA file %s \n",
                   __func__, data_file);

    {
        long int start_pos = util_ftell(stream);
        size_t buffer_size;

        /* Look for terminating '/' */
        if (!basic_parser_fseek_string(parser, stream, "/", false, true))
            util_abort("%s: sorry - could not find \"/\" termination of START "
                       "keyword in data_file: \n",
                       __func__, data_file);

        /* fseek_string succeeded, so the stream has advanced past
           start_pos. */
        buffer_size = static_cast<size_t>(util_ftell(stream) - start_pos);
        buffer = (char *)util_calloc(buffer_size + 1, sizeof *buffer);
        util_fseek(stream, start_pos, SEEK_SET);
        util_fread(buffer, sizeof *buffer, buffer_size, stream, __func__);
        buffer[buffer_size] = '\0';
    }

    {
        stringlist_type *tokens =
            basic_parser_tokenize_buffer(parser, buffer, true);
        int day, year, month_nr;
        if (util_sscanf_int(stringlist_iget(tokens, 0), &day) &&
            util_sscanf_int(stringlist_iget(tokens, 2), &year)) {
            month_nr = rd_get_month_nr(stringlist_iget(tokens, 1));
            start_date = rd_make_date(day, month_nr, year);
        } else
            util_abort("%s: failed to parse DAY MONTH YEAR from : \"%s\" \n",
                       __func__, buffer);
        stringlist_free(tokens);
    }

    free(buffer);
    basic_parser_free(parser);
    fclose(stream);

    return start_date;
}

static int rd_get_num_parallel_cpu__(basic_parser_type *parser, FILE *stream,
                                     const char *data_file) {
    int num_cpu = 1;
    char *buffer;
    long int start_pos = util_ftell(stream);
    size_t buffer_size;

    /* Look for terminating '/' */
    if (!basic_parser_fseek_string(parser, stream, "/", false, true))
        util_abort("%s: sorry - could not find \"/\" termination of PARALLEL "
                   "keyword in data_file: \n",
                   __func__, data_file);

    /* fseek_string succeeded, so the stream has advanced past start_pos. */
    buffer_size = static_cast<size_t>(util_ftell(stream) - start_pos);
    buffer = (char *)util_calloc(buffer_size + 1, sizeof *buffer);
    util_fseek(stream, start_pos, SEEK_SET);
    util_fread(buffer, sizeof *buffer, buffer_size, stream, __func__);
    buffer[buffer_size] = '\0';

    {
        stringlist_type *tokens =
            basic_parser_tokenize_buffer(parser, buffer, true);

        if (stringlist_get_size(tokens) > 0) {
            const char *num_cpu_string = stringlist_iget(tokens, 0);
            if (!util_sscanf_int(num_cpu_string, &num_cpu))
                fprintf(stderr,
                        "** Warning: failed to interpret:%s as integer - "
                        "assuming one CPU\n",
                        num_cpu_string);
        } else
            fprintf(stderr, "** Warning: failed to load data for PARALLEL "
                            "keyword - assuming one CPU\n");

        stringlist_free(tokens);
    }
    free(buffer);
    return num_cpu;
}

static int rd_get_num_slave_cpu__(basic_parser_type *parser, FILE *stream,
                                  const char *data_file) {
    int num_cpu = 0;
    int linecount = 0;

    basic_parser_fseek_string(
        parser, stream, "\n", true,
        true); /* Go to next line after the SLAVES keyword*/

    while (true) {
        char *buffer = util_fscanf_alloc_line(stream, NULL);
        ++linecount;
        if (linecount > 10)
            util_abort("%s: Did not find ending \"/\" character after SLAVES "
                       "keyword, aborting \n",
                       __func__);

        {
            stringlist_type *tokens =
                basic_parser_tokenize_buffer(parser, buffer, true);
            if (stringlist_get_size(tokens) > 0) {

                const char *first_item = stringlist_iget(tokens, 0);

                if (first_item[0] == '/') {
                    stringlist_free(tokens);
                    free(buffer);
                    break;
                } else {
                    int no_of_tokens = stringlist_get_size(tokens);
                    int no_of_slaves = 0;
                    if (no_of_tokens == 6 &&
                        util_sscanf_int(stringlist_iget(tokens, 4),
                                        &no_of_slaves)) {
                        num_cpu += no_of_slaves;
                    } else {
                        ++num_cpu;
                    }
                }
            }
            stringlist_free(tokens);
        }

        free(buffer);
    }

    if (0 == num_cpu)
        util_abort("%s: Did not any CPUs after SLAVES keyword, aborting \n",
                   __func__);
    return num_cpu;
}

/* Finding keywords requires skipping comments, which is done by the basic
   parsers and skipping titles, which it does not. This function searches for
   the first occurence of a keyword outside of titles and comments. This code is
   somewhat complicated since it seems that the spec allows for multiple titles
   and (possibly) blank lines between the title keyword and the title. */
static bool rd_find_keyword__(basic_parser_type *parser, FILE *stream,
                              const char *keyword) {
    long int title_pos = -1;

    /* Find the first occurenced of TITLE, if any. */
    if (basic_parser_fseek_string(parser, stream, "TITLE", false, true)) {
        title_pos = util_ftell(stream);
        util_rewind(stream);
    }

    /* Find all keyword occurences, returning the first that is valid. */
    while (basic_parser_fseek_string(parser, stream, keyword, false, true)) {
        long int keyword_pos = util_ftell(stream);

        /* Starting with last title found, find all titles that start before
           this keyword occurence, to see if they contain the keyword: */
        while (title_pos >= 0 && keyword_pos > title_pos) {
            /* Find the end of this title. */
            int lines_to_skip = 2; /* Two non-blank lines to skip in a title. */
            bool blank_line = true;

            util_fseek(stream, title_pos, SEEK_SET);
            while (lines_to_skip > 0) {
                int c = fgetc(stream);
                if (c == EOF)
                    return false;
                if (!blank_line && c == '\n') {
                    --lines_to_skip;
                    blank_line = true;
                } else {
                    blank_line = blank_line && isblank(c);
                }
            }

            /* If within this title: break, this keyword fails. */
            if (keyword_pos < util_ftell(stream))
                break;

            /* Find the next occurence of TITLE, if any. */
            if (basic_parser_fseek_string(parser, stream, "TITLE", false, true))
                title_pos = util_ftell(stream);
            else
                title_pos = -1;
        }

        /* Position to the end of the keyword, we either are succesful, or we
           need to continue looking for the next keyword. */
        util_fseek(stream, keyword_pos + static_cast<long int>(strlen(keyword)),
                   SEEK_SET);

        /* If we are not within a title: success. */
        if (title_pos < 0 || keyword_pos < title_pos)
            return true;
    }

    return false;
}

int rd_get_num_cpu(const char *data_file) {
    int num_cpu = 1;
    basic_parser_type *parser =
        basic_parser_alloc(" \t\r\n", "\"\'", NULL, NULL, "--", "\n");
    FILE *stream = util_fopen(data_file, "r");

    if (rd_find_keyword__(parser, stream, "PARALLEL")) {
        num_cpu = rd_get_num_parallel_cpu__(parser, stream, data_file);
    } else if (rd_find_keyword__(parser, stream, "SLAVES")) {
        num_cpu = rd_get_num_slave_cpu__(parser, stream, data_file) + 1;
        fprintf(stderr,
                "Information: \"SLAVES\" option found, returning %d number "
                "of CPUs",
                num_cpu);
    }

    basic_parser_free(parser);
    fclose(stream);
    return num_cpu;
}

static time_t rd_make_datetime__(int sec, int min, int hour, int mday,
                                 int month, int year, int *__year_offset) {
    time_t date;

#ifdef ERT_TIME_T_64BIT_ACCEPT_PRE1970
    *__year_offset = 0;
    date = util_make_date_utc(mday, month, year);
#else
    static bool offset_initialized = false;
    static int year_offset = 0;

    if (!offset_initialized) {
        if (year < 1970) {
            year_offset = 2000 - year;
            fprintf(
                stderr,
                "Warning: all year values will be shifted %d years forward. \n",
                year_offset);
        }
        offset_initialized = true;
    }
    *__year_offset = year_offset;
    date =
        util_make_datetime_utc(sec, min, hour, mday, month, year + year_offset);
#endif

    return date;
}

time_t rd_make_date__(int mday, int month, int year, int *__year_offset) {
    return rd_make_datetime__(0, 0, 0, mday, month, year, __year_offset);
}

time_t rd_make_date(int mday, int month, int year) {
    int year_offset;
    return rd_make_date__(mday, month, year, &year_offset);
}

time_t rd_make_datetime(int sec, int min, int hour, int mday, int month,
                        int year) {
    int year_offset;
    return rd_make_datetime__(sec, min, hour, mday, month, year, &year_offset);
}

void rd_set_datetime_values(time_t t, int *sec, int *min, int *hour, int *mday,
                            int *month, int *year) {
    return util_set_datetime_values_utc(t, sec, min, hour, mday, month, year);
}

#ifdef ERT_HAVE_UNISTD
#include <unistd.h>
#endif

/*
  This is a small function which tries to give a sensible answer to the
  question: Do I have read access to this eclipse simulation? The rd_case
  argument can either be a directory or the full path to a file, the filename
  need not exists. The approach is as follows:

  1. If @rd_case corresponds to an existing filesystem entry - just return
     access(rd_case, R_OK).

  2. If @rd_case corresponds to a non-existing entry:

       a) If there is a directory part - return access(dir, R_OK).
       b) No directory part - return access(cwd, R_OK);

      For the case 2b) the situation is that we test for read access to CWD,
      that could in principle be denied - but that is a highly contrived
      situation and we just return true.

  rd_access_path("PATH")                     ->   access("PATH", R_OK);
  rd_access_path("PATH/FILE_EXISTS")         ->   access("PATH/FILE_EXISTS", R_OK);
  rd_access_path("PATH/FILE_DOES_NOT_EXIST") ->   access("PATH", R_OK);
  rd_access_path("PATH_DOES_NOT_EXIST")      ->   true
*/

bool rd_path_access(const char *rd_case) {
    if (util_access(rd_case, R_OK))
        return true;

    if (util_access(rd_case, F_OK))
        return false;

    /* Check if the input argument corresponds to an existing directory and one
     additional element, in that case we do an access check on the directory part. */

    {
        bool path_access;
        char *dir_name;
        const char *path_sep = strrchr(rd_case, UTIL_PATH_SEP_CHAR);

        if (!path_sep)
            /* We are trying to access CWD - we return true without actually checking
         access. */
            return true;

        dir_name = util_alloc_substring_copy(rd_case, 0, path_sep - rd_case);
        path_access = util_access(dir_name, R_OK);
        free(dir_name);
        return path_access;
    }
    return false;
}
