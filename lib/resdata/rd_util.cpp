#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include <ert/util/ert_api_config.hpp>

#include <ert/util/util.hpp>
#include <ert/util/hash.hpp>
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

const char *rd_get_phase_name(rd_phase_enum phase) {
    switch (phase) {
    case (RD_OIL_PHASE):
        return RD_PHASE_NAME_OIL;
        break;
    case (RD_WATER_PHASE):
        return RD_PHASE_NAME_WATER;
        break;
    case (RD_GAS_PHASE):
        return RD_PHASE_NAME_GAS;
        break;
    default:
        util_abort("%s: phase enum value:%d not recognized \n", __func__,
                   phase);
        return NULL;
    }
}

static char *rd_alloc_base_guess(const char *path) {
    char *base = NULL;
    stringlist_type *data_files = stringlist_alloc_new();
    stringlist_type *DATA_files = stringlist_alloc_new();
    stringlist_select_matching_files(data_files, path, "*.data");
    stringlist_select_matching_files(DATA_files, path, "*.DATA");

    if ((stringlist_get_size(data_files) + stringlist_get_size(DATA_files)) ==
        1) {
        const char *path_name;

        if (stringlist_get_size(data_files) == 1)
            path_name = stringlist_iget(data_files, 0);
        else
            path_name = stringlist_iget(DATA_files, 0);

        util_alloc_file_components(path_name, NULL, &base, NULL);
    } // Else - found either 0 or more than 1 file with extension DATA - impossible to guess.

    stringlist_free(data_files);
    stringlist_free(DATA_files);

    return base;
}

int rd_filename_report_nr(const char *filename) {
    int report_nr = -1;
    rd_get_file_type(filename, NULL, &report_nr);
    return report_nr;
}

/*
 We accept mixed lowercase/uppercase Eclipse file extensions even if Eclipse itself does not accept them.
*/
static rd_file_enum rd_inspect_extension(const char *ext, bool *_fmt_file,
                                         int *_report_nr) {
    rd_file_enum file_type = RD_OTHER_FILE;
    bool fmt_file = true;
    int report_nr = -1;
    char *upper_ext = util_alloc_strupr_copy(ext);
    if (strcmp(upper_ext, "UNRST") == 0) {
        file_type = RD_UNIFIED_RESTART_FILE;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FUNRST") == 0) {
        file_type = RD_UNIFIED_RESTART_FILE;
        fmt_file = true;
    } else if (strcmp(upper_ext, "UNSMRY") == 0) {
        file_type = RD_UNIFIED_SUMMARY_FILE;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FUNSMRY") == 0) {
        file_type = RD_UNIFIED_SUMMARY_FILE;
        fmt_file = true;
    } else if (strcmp(upper_ext, "SMSPEC") == 0) {
        file_type = RD_SUMMARY_HEADER_FILE;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FSMSPEC") == 0) {
        file_type = RD_SUMMARY_HEADER_FILE;
        fmt_file = true;
    } else if (strcmp(upper_ext, "GRID") == 0) {
        file_type = RD_GRID_FILE;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FGRID") == 0) {
        file_type = RD_GRID_FILE;
        fmt_file = true;
    } else if (strcmp(upper_ext, "EGRID") == 0) {
        file_type = RD_EGRID_FILE;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FEGRID") == 0) {
        file_type = RD_EGRID_FILE;
        fmt_file = true;
    } else if (strcmp(upper_ext, "INIT") == 0) {
        file_type = RD_INIT_FILE;
        fmt_file = false;
    } else if (strcmp(upper_ext, "FINIT") == 0) {
        file_type = RD_INIT_FILE;
        fmt_file = true;
    } else if (strcmp(upper_ext, "FRFT") == 0) {
        file_type = RD_RFT_FILE;
        fmt_file = true;
    } else if (strcmp(upper_ext, "RFT") == 0) {
        file_type = RD_RFT_FILE;
        fmt_file = false;
    } else if (strcmp(upper_ext, "DATA") == 0) {
        file_type = RD_DATA_FILE;
        fmt_file = true; /* Not really relevant ... */
    } else {
        switch (upper_ext[0]) {
        case ('X'):
            file_type = RD_RESTART_FILE;
            fmt_file = false;
            break;
        case ('F'):
            file_type = RD_RESTART_FILE;
            fmt_file = true;
            break;
        case ('S'):
            file_type = RD_SUMMARY_FILE;
            fmt_file = false;
            break;
        case ('A'):
            file_type = RD_SUMMARY_FILE;
            fmt_file = true;
            break;
        default:
            file_type = RD_OTHER_FILE;
        }
        if (file_type != RD_OTHER_FILE)
            if (!util_sscanf_int(&upper_ext[1], &report_nr))
                file_type = RD_OTHER_FILE;
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

rd_file_enum rd_get_file_type(const char *filename, bool *fmt_file,
                              int *report_nr) {
    char *ext = (char *)strrchr(filename, '.');
    if (ext == NULL)
        return RD_OTHER_FILE;

    return rd_inspect_extension(&ext[1], fmt_file, report_nr);
}

static const char *rd_get_file_pattern(rd_file_enum file_type, bool fmt_file) {
    if (fmt_file) {
        switch (file_type) {
        case (RD_OTHER_FILE):
            return RD_OTHER_FILE_FMT_PATTERN; /* '*' */
            break;
        case (RD_UNIFIED_RESTART_FILE):
            return RD_UNIFIED_RESTART_FMT_PATTERN;
            break;
        case (RD_UNIFIED_SUMMARY_FILE):
            return RD_UNIFIED_SUMMARY_FMT_PATTERN;
            break;
        case (RD_GRID_FILE):
            return RD_GRID_FMT_PATTERN;
            break;
        case (RD_EGRID_FILE):
            return RD_EGRID_FMT_PATTERN;
            break;
        case (RD_INIT_FILE):
            return RD_INIT_FMT_PATTERN;
            break;
        case (RD_RFT_FILE):
            return RD_RFT_FMT_PATTERN;
            break;
        case (RD_DATA_FILE):
            return RD_DATA_PATTERN;
            break;
        default:
            util_abort("%s: No pattern defined for til_type:%d \n", __func__,
                       file_type);
            return NULL;
        }
    } else {
        switch (file_type) {
        case (RD_OTHER_FILE):
            return RD_OTHER_FILE_UFMT_PATTERN; /* '*' */
            break;
        case (RD_UNIFIED_RESTART_FILE):
            return RD_UNIFIED_RESTART_UFMT_PATTERN;
            break;
        case (RD_UNIFIED_SUMMARY_FILE):
            return RD_UNIFIED_SUMMARY_UFMT_PATTERN;
            break;
        case (RD_GRID_FILE):
            return RD_GRID_UFMT_PATTERN;
            break;
        case (RD_EGRID_FILE):
            return RD_EGRID_UFMT_PATTERN;
            break;
        case (RD_INIT_FILE):
            return RD_INIT_UFMT_PATTERN;
            break;
        case (RD_RFT_FILE):
            return RD_RFT_UFMT_PATTERN;
            break;
        default:
            util_abort("%s: No pattern defined for til_type:%d \n", __func__,
                       file_type);
            return NULL;
        }
    }
}

/**
   Takes an rd_file_enum variable and returns string with a
   descriptive name of this file type.
*/
const char *rd_file_type_name(rd_file_enum file_type) {
    switch (file_type) {
    case (RD_OTHER_FILE):
        return "RD_OTHER_FILE";
        break;
    case (RD_RESTART_FILE):
        return "RD_RESTART_FILE";
        break;
    case (RD_UNIFIED_RESTART_FILE):
        return "RD_UNIFIED_RESTART_FILE";
        break;
    case (RD_SUMMARY_FILE):
        return "RD_SUMMARY_FILE";
        break;
    case (RD_UNIFIED_SUMMARY_FILE):
        return "RD_UNIFIED_SUMMARY_FILE";
        break;
    case (RD_SUMMARY_HEADER_FILE):
        return "RD_SUMMARY_HEADER_FILE";
        break;
    case (RD_GRID_FILE):
        return "RD_GRID_FILE";
        break;
    case (RD_EGRID_FILE):
        return "RD_EGRID_FILE";
        break;
    case (RD_INIT_FILE):
        return "RD_INIT_FILE";
        break;
    case (RD_RFT_FILE):
        return "RD_RFT_FILE";
        break;
    case (RD_DATA_FILE):
        return "RD_DATA_FILE";
        break;
    default:
        util_abort("%s: internal error type.%d not recognizxed \n", __func__,
                   file_type);
    }
    return NULL;
}

static bool base_has_upper(const char *input_base) {
    const char *base = strrchr(input_base, UTIL_PATH_SEP_CHAR);
    if (base == NULL)
        base = input_base;

    for (size_t i = 0; i < strlen(base); i++) {
        if (isupper(base[i]))
            return true;
    }

    return false;
}

/**
   This function takes a path, along with a filetype as input and
   allocates a new string with the filename. If path == NULL, the
   filename is allocated without a leading path component.

   If the flag 'must_exist' is set to true the function will check
   with the filesystem if the file actually exists; if the file does
   not exist NULL is returned.
*/

static char *rd_alloc_filename_static(const char *path, const char *base,
                                      rd_file_enum file_type, bool fmt_file,
                                      int report_nr, bool must_exist) {
    char *filename;
    char *ext;
    switch (file_type) {
    case (RD_RESTART_FILE):
        if (fmt_file)
            ext = util_alloc_sprintf("F%04d", report_nr);
        else
            ext = util_alloc_sprintf("X%04d", report_nr);
        break;

    case (RD_UNIFIED_RESTART_FILE):
        if (fmt_file)
            ext = util_alloc_string_copy("FUNRST");
        else
            ext = util_alloc_string_copy("UNRST");
        break;

    case (RD_SUMMARY_FILE):
        if (fmt_file)
            ext = util_alloc_sprintf("A%04d", report_nr);
        else
            ext = util_alloc_sprintf("S%04d", report_nr);
        break;

    case (RD_UNIFIED_SUMMARY_FILE):
        if (fmt_file)
            ext = util_alloc_string_copy("FUNSMRY");
        else
            ext = util_alloc_string_copy("UNSMRY");
        break;

    case (RD_SUMMARY_HEADER_FILE):
        if (fmt_file)
            ext = util_alloc_string_copy("FSMSPEC");
        else
            ext = util_alloc_string_copy("SMSPEC");
        break;

    case (RD_GRID_FILE):
        if (fmt_file)
            ext = util_alloc_string_copy("FGRID");
        else
            ext = util_alloc_string_copy("GRID");
        break;

    case (RD_EGRID_FILE):
        if (fmt_file)
            ext = util_alloc_string_copy("FEGRID");
        else
            ext = util_alloc_string_copy("EGRID");
        break;

    case (RD_INIT_FILE):
        if (fmt_file)
            ext = util_alloc_string_copy("FINIT");
        else
            ext = util_alloc_string_copy("INIT");
        break;

    case (RD_RFT_FILE):
        if (fmt_file)
            ext = util_alloc_string_copy("FRFT");
        else
            ext = util_alloc_string_copy("RFT");
        break;

    case (RD_DATA_FILE):
        ext = util_alloc_string_copy("DATA");
        break;

    default:
        util_abort("%s: Invalid input file_type to rd_alloc_filename - "
                   "aborting \n",
                   __func__);
        /* Dummy to shut up compiler */
        ext = NULL;
    }

    if (!base_has_upper(base)) {
        for (size_t i = 0; i < strlen(ext); i++)
            ext[i] = tolower(ext[i]);
    }

    filename = util_alloc_filename(path, base, ext);
    free(ext);

    if (must_exist) {
        if (!util_file_exists(filename)) {
            free(filename);
            filename = NULL;
        }
    }

    return filename;
}

char *rd_alloc_filename(const char *path, const char *base,
                        rd_file_enum file_type, bool fmt_file, int report_nr) {
    return rd_alloc_filename_static(path, base, file_type, fmt_file, report_nr,
                                    false);
}

char *rd_alloc_exfilename(const char *path, const char *base,
                          rd_file_enum file_type, bool fmt_file,
                          int report_nr) {
    return rd_alloc_filename_static(path, base, file_type, fmt_file, report_nr,
                                    true);
}

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

/**
   This function will scan the directory @path (or cwd if @path == NULL)
   for all files of type @file_type. If base == NULL it will use
   '*' as pattern for basename. If file_type == RD_OTHER_FILE it will
   use '*' as pattern for the extension (as a consequence files which do
   not originate from the simulator will also be included).

   The stringlist will be cleared before the actual matching process
   starts.
*/

static bool numeric_extension_predicate(const char *filename, const char *base,
                                        const char leading_char) {
    if (strncmp(filename, base, strlen(base)) != 0)
        return false;

    const char *ext_start = strrchr(filename, '.');
    if (!ext_start)
        return false;

    if (strlen(ext_start) != 6)
        return false;

    if (ext_start[1] != leading_char)
        return false;

    for (int i = 0; i < 4; i++)
        if (!isdigit(ext_start[i + 2]))
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

static int rd_select_predicate_filelist(const char *path, const char *base,
                                        rd_file_enum file_type, bool fmt_file,
                                        bool upper_case,
                                        stringlist_type *filelist) {
    file_pred_ftype *predicate = NULL;
    char *full_path = NULL;
    char *pure_base = NULL;
    {
        char *tmp = util_alloc_filename(path, base, NULL);
        util_alloc_file_components(tmp, &full_path, &pure_base, NULL);
        free(tmp);
    }

    if (file_type == RD_SUMMARY_FILE) {
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
    } else if (file_type == RD_RESTART_FILE) {
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
        util_abort(
            "%s: internal error - method called with wrong file type: %d\n",
            __func__, file_type);

    stringlist_select_files(filelist, full_path, predicate, pure_base);
    stringlist_sort(filelist, rd_fname_report_cmp);
    free(pure_base);
    free(full_path);
    return stringlist_get_size(filelist);
}

int rd_select_filelist(const char *path, const char *base,
                       rd_file_enum file_type, bool fmt_file,
                       stringlist_type *filelist) {
    stringlist_clear(filelist);

    bool upper_case = base_has_upper(base);
    if (file_type == RD_SUMMARY_FILE || file_type == RD_RESTART_FILE)
        return rd_select_predicate_filelist(path, base, file_type, fmt_file,
                                            upper_case, filelist);

    char *ext_pattern =
        util_alloc_string_copy(rd_get_file_pattern(file_type, fmt_file));

    if (!upper_case) {
        for (size_t i = 0; i < strlen(ext_pattern); i++)
            ext_pattern[i] = tolower(ext_pattern[i]);
    }

    char *file_pattern;
    if (base)
        file_pattern = util_alloc_filename(NULL, base, ext_pattern);
    else
        file_pattern = util_alloc_filename(NULL, "*", ext_pattern);

    stringlist_select_matching_files(filelist, path, file_pattern);
    free(file_pattern);
    free(ext_pattern);

    return stringlist_get_size(filelist);
}

bool rd_unified_file(const char *filename) {
    int report_nr;
    rd_file_enum file_type;
    bool fmt_file;
    file_type = rd_get_file_type(filename, &fmt_file, &report_nr);

    if ((file_type == RD_UNIFIED_RESTART_FILE) ||
        (file_type == RD_UNIFIED_SUMMARY_FILE))
        return true;
    else
        return false;
}

bool rd_fmt_file(const char *filename, bool *__fmt_file) {
    /*const int min_size = 32768;*/
    const int min_size = 256; /* Veeeery small */

    int report_nr;
    rd_file_enum file_type;
    bool status = true;
    bool fmt_file = 0;

    if (util_file_exists(filename)) {
        file_type = rd_get_file_type(filename, &fmt_file, &report_nr);
        if (file_type == RD_OTHER_FILE) {
            if (util_file_size(filename) > min_size)
                fmt_file = util_fmt_bit8(filename);
            else
                status = false; // Do not know ??
        }
    } else {
        file_type = rd_get_file_type(filename, &fmt_file, &report_nr);
        if (file_type == RD_OTHER_FILE)
            status = false; // Do not know ??
    }

    *__fmt_file = fmt_file;
    return status;
}

/**
  The stringlist will be cleared before the actual matching process
  starts. Observe that in addition to the @path input parameter the
  @base input can contain an embedded path component.
*/
static void rd_alloc_summary_data_files(const char *path, const char *base,
                                        bool fmt_file,
                                        stringlist_type *filelist) {
    char *unif_data_file =
        rd_alloc_exfilename(path, base, RD_UNIFIED_SUMMARY_FILE, fmt_file, -1);
    int files =
        rd_select_filelist(path, base, RD_SUMMARY_FILE, fmt_file, filelist);

    if ((files > 0) && (unif_data_file != NULL)) {
        /*
       We have both a unified file AND a list of files: BASE.S0000,
       BASE.S0001, BASE.S0002, ..., must check which is newest and
       load accordingly.
    */
        bool unified_newest = true;
        int file_nr = 0;
        while (unified_newest && (file_nr < files)) {
            if (util_file_difftime(stringlist_iget(filelist, file_nr),
                                   unif_data_file) > 0)
                unified_newest = false;
            file_nr++;
        }

        if (unified_newest) {
            stringlist_clear(
                filelist); /* Clear out all the BASE.Snnnn selections. */
            stringlist_append_copy(filelist, unif_data_file);
        }
    } else if (unif_data_file != NULL) {
        /* Found a unified summary file :  Clear out all the BASE.Snnnn selections. */
        stringlist_clear(
            filelist); /* Clear out all the BASE.Snnnn selections. */
        stringlist_append_copy(filelist, unif_data_file);
    }
    free(unif_data_file);
}

/**
   This routine allocates summary header and data files from a
   directory, and return them by reference; path and base are
   input. If the function can not find BOTH a summary header file and
   summary data it will return false and not update the reference
   variables.

   For the header file there are two possible files:

     1. X.FSMSPEC
     2. X.SMSPEEC

   For the data there are four different possibilities:

     1. X.A0001, X.A0002, X.A0003, ...
     2. X.FUNSMRY
     3. X.S0001, X.S0002, X.S0003, ...
     4. X.UNSMRY

   In principle a directory can contain all different (altough that is
   probably not typical). The algorithm is a a two step algorithm:

     1. Determine wether to use X.FSMSPEC or X.SMSPEC based on which
        is the newest. This also implies a decision of wether to use
        formatted, or unformatted filed.

     2. Use formatted or unformatted files according to 1. above, and
        then choose either a list of files or unified files according
        to which is the newest.

   This algorithm should work in most practical cases, but it is
   surely possible to fool it.
*/

bool rd_alloc_summary_files(const char *path, const char *_base,
                            const char *ext, char **_header_file,
                            stringlist_type *filelist) {
    bool fmt_input = false;
    bool fmt_set = false;
    bool fmt_file = true;
    bool unif_input = false;
    bool unif_set = false;

    char *header_file = NULL;
    char *base;

    *_header_file = NULL;

    /* 1: We start by inspecting the input extension and see if we can
     learn anything about formatted/unformatted and
     unified/non-unified from this. The input extension can be NULL,
     in which case we learn nothing.
  */

    if (_base == NULL)
        base = rd_alloc_base_guess(path);
    else
        base = (char *)_base;

    if (ext != NULL) {
        rd_file_enum input_type;

        {
            char *test_name = util_alloc_filename(NULL, base, ext);
            input_type = rd_get_file_type(test_name, &fmt_input, NULL);
            free(test_name);
        }

        if ((input_type != RD_OTHER_FILE) && (input_type != RD_DATA_FILE)) {
            /*
         The file has been recognized as a file type from which we can
         at least infer formatted/unformatted inforamtion.
      */
            fmt_set = true;
            switch (input_type) {
            case (RD_SUMMARY_FILE):
            case (RD_RESTART_FILE):
                unif_input = false;
                unif_set = true;
                break;
            case (RD_UNIFIED_SUMMARY_FILE):
            case (RD_UNIFIED_RESTART_FILE):
                unif_input = true;
                unif_set = true;
                break;
            default: /* Nothing wrong with this */
                break;
            }
        }
    }

    /*
    2: We continue by looking for header files.
  */

    {
        char *fsmspec_file =
            rd_alloc_exfilename(path, base, RD_SUMMARY_HEADER_FILE, true, -1);
        char *smspec_file =
            rd_alloc_exfilename(path, base, RD_SUMMARY_HEADER_FILE, false, -1);

        if ((fsmspec_file == NULL) &&
            (smspec_file == NULL)) /* Neither file exists */
            return false;

        if (fmt_set) /* The question of formatted|unformatted has already been settled based on the input filename. */
            fmt_file = fmt_input;
        else {
            if ((fsmspec_file != NULL) &&
                (smspec_file !=
                 NULL)) { /* Both fsmspec and smspec exist - we take the newest. */
                if (util_file_difftime(fsmspec_file, smspec_file) < 0)
                    fmt_file = true;
                else
                    fmt_file = false;
            } else { /* Only one of fsmspec / smspec exists */
                if (fsmspec_file != NULL)
                    fmt_file = true;
                else
                    fmt_file = false;
            }
        }

        if (fmt_file) {
            header_file = fsmspec_file;
            free(smspec_file);
        } else {
            header_file = smspec_file;
            free(fsmspec_file);
        }

        if (header_file == NULL)
            return false; /* If you insist on e.g. unformatted and only fsmspec exists - no results for you. */
    }

    /*
     3: OK - we have found a SMSPEC / FMSPEC file - continue to look for
     XXX.Snnnn / XXX.UNSMRY files.
  */

    if (unif_set) { /* Based on the input file we have inferred whether to look for unified or
                     non-unified input files. */

        if (unif_input) {
            char *unif_data_file = rd_alloc_exfilename(
                path, base, RD_UNIFIED_SUMMARY_FILE, fmt_file, -1);
            if (unif_data_file != NULL) {
                stringlist_append_copy(filelist, unif_data_file);
                free(unif_data_file);
            }
        } else
            rd_select_filelist(path, base, RD_SUMMARY_FILE, fmt_file, filelist);
    } else
        rd_alloc_summary_data_files(path, base, fmt_file, filelist);

    if (_base == NULL)
        free(base);

    *_header_file = header_file;

    return (stringlist_get_size(filelist) > 0) ? true : false;
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
        int buffer_size;

        /* Look for terminating '/' */
        if (!basic_parser_fseek_string(parser, stream, "/", false, true))
            util_abort("%s: sorry - could not find \"/\" termination of START "
                       "keyword in data_file: \n",
                       __func__, data_file);

        buffer_size = (util_ftell(stream) - start_pos);
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
    int buffer_size;

    /* Look for terminating '/' */
    if (!basic_parser_fseek_string(parser, stream, "/", false, true))
        util_abort("%s: sorry - could not find \"/\" termination of PARALLEL "
                   "keyword in data_file: \n",
                   __func__, data_file);

    buffer_size = (util_ftell(stream) - start_pos);
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
        util_fseek(stream, keyword_pos + strlen(keyword), SEEK_SET);

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

void rd_set_date_values(time_t t, int *mday, int *month, int *year) {
    return util_set_date_values_utc(t, mday, month, year);
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
