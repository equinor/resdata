#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <filesystem>

#include <cstring>
#include <cmath>
#include <ctime>
#include <locale.h>
#include <string>
#include <optional>
#include <utility>
#include <fmt/format.h>

#include <ert/util/hash.hpp>
#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/double_vector.hpp>
#include <ert/util/type_macros.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/rd_sum_vector.hpp>
#include <resdata/rd_sum_data.hpp>
#include <resdata/rd_smspec.hpp>
#include <resdata/smspec_node.hpp>

#include <detail/util/path.hpp>

namespace fs = std::filesystem;

/**
   The summary data is organised in a header file (.SMSPEC)
   and the actual summary data. This file implements a data structure
   rd_sum_type which holds summary data. Most of the actual
   implementation is in separate files rd_smspec.c for the SMSPEC
   header, and rd_sum_data for the actual data.

   Observe that this datastructure is built up around internalizing
   summary data, the code has NO AMBITION of being able to
   write summary data.


   Header in CASE.SMSPEC file                  Actual data; many 'PARAMS' blocks in .Snnnn or .UNSMRY file

   ------------------------------.........       -----------   -----------   -----------   -----------   -----------
   | WGNAMES    KEYWORDS   NUMS | INDEX  :       | PARAMS  |   | PARAMS  |   | PARAMS  |   | PARAMS  |   | PARAMS  |
   |----------------------------|........:       |---------|   |---------|   |---------|   |---------|   |---------|
   | OP-1       WOPR       X    |   0    :       |  652    |   |   752   |   |  862    |   |   852   |   |    962  |
   | OP-1       WWPR       X    |   1    :       |   45    |   |    47   |   |   55    |   |    59   |   |     62  |
   | GI-1       WGIR       X    |   2    :       |  500    |   |   500   |   |  786    |   |   786   |   |    486  |
   | :+:+:+:+   FOPT       X    |   3    :       | 7666    |   |  7666   |   | 8811    |   |  7688   |   |   8649  |
   | :+:+:+:+   RPR        5    |   4    :       |  255    |   |   255   |   |  266    |   |   257   |   |    277  |
   | :+:+:+:+   BPR        3457 |   5    :       |  167    |   |   167   |   |  189    |   |   201   |   |    166  |
   ------------------------------.........       -----------   -----------   -----------   -----------   -----------

                                                 <------------------------ Time direction ------------------------->

   As illustrated in the figure above header information is stored in
   the SMSPEC file; the header information is organised in several
   keywords; at least the WGNAMES and KEYWORDS arrays, and often also
   the NUMS array. Together these three arrays uniquely specify a
   summary vector.

   The INDEX column in the header information is NOT part of the
   SMSPEC file, but an important part of the rd_smspec
   implementation. The the values from WGNAMES/KEYWORDS/NUMS are
   combined to create a unique key; and the corresponding index is
   used to lookup a numerical value from the PARAMS vector with actual
   data.

   These matters are documented further in the rd_smspec.c and
   rd_sum_data.c files.
*/

#define RD_SUM_ID 89067

struct rd_sum_struct {
    UTIL_TYPE_ID_DECLARATION;
    rd_smspec_ptr smspec{
        nullptr,
        &rd_smspec_free}; /* Internalized version of the SMSPEC file. */
    rd_sum_data_ptr data{nullptr, &rd_sum_data_free};
    rd_sum_ptr restart_case{nullptr, &rd_sum_free};

    bool fmt_case;
    bool unified;
    std::string key_join_string;
    std::string path;     /* Path as given for the case input */
    std::string abs_path; /* Absolute path. */
    std::string base;     /* Only the basename. */
    std::string
        rd_case; /* This is the current case, with optional path component. == path + base*/
    std::string
        ext; /* Only to support selective loading of formatted|unformatted and unified|multiple. */
};

UTIL_SAFE_CAST_FUNCTION(rd_sum, RD_SUM_ID);
UTIL_IS_INSTANCE_FUNCTION(rd_sum, RD_SUM_ID);

/**
   Reads the data from summary files, can either be a list of
   files BASE.S0000, BASE.S0001, BASE.S0002,.. or one unified
   file. Formatted/unformatted is detected automagically.

   The actual loading is implemented in the rd_sum_data.c file.
*/

void rd_sum_set_case(rd_sum_type *rd_sum, const std::string &input_arg) {
    fs::path input_path(input_arg);
    fs::path path = input_path.parent_path();
    rd_sum->path = path.string();
    rd_sum->base = input_path.stem().string();
    rd_sum->rd_case = (path / rd_sum->base).string();
    rd_sum->ext = input_path.extension().string();
    if (path.empty())
        rd_sum->abs_path = fs::current_path().string();
    else
        rd_sum->abs_path = fs::absolute(path).string();
}

static rd_sum_ptr rd_sum_alloc__(std::string input_arg,
                                 const std::string &key_join_string) {
    if (!rd_path_access(input_arg.c_str()))
        return {nullptr, &rd_sum_free};

    rd_sum_ptr rd_sum{nullptr, &rd_sum_free};
    rd_sum.reset(new rd_sum_type);
    UTIL_TYPE_ID_INIT(rd_sum.get(), RD_SUM_ID);

    rd_sum_set_case(rd_sum.get(), input_arg);
    rd_sum->key_join_string = key_join_string;

    return rd_sum;
}

/**
   This function frees the data from the rd_sum instance and sets the
   data pointer to NULL. The SMSPEC data is still valid, and can be
   reused with calls to rd_sum_fread_realloc_data().
*/

static bool rd_sum_fread_data(rd_sum_type *rd_sum,
                              const stringlist_type *data_files, bool lazy_load,
                              int file_options) {
    rd_sum->data.reset(rd_sum_data_alloc(rd_sum->smspec.get()));
    return rd_sum_data_fread(rd_sum->data.get(), data_files, lazy_load,
                             file_options);
}

static void rd_sum_fread_history(rd_sum_type *rd_sum, bool lazy_load,
                                 int file_options) {
    const char *restart_header =
        rd_smspec_get_restart_case(rd_sum->smspec.get());
    if (restart_header == nullptr)
        return;

    fs::path restart_path =
        rd::filename(fs::path(restart_header), RD_SUMMARY_HEADER_FILE,
                     rd_smspec_get_formatted(rd_sum->smspec.get()), -1);
    rd_sum_ptr restart_case =
        read_summary(restart_path.string(), ":", lazy_load, true, file_options);
    if (restart_case) {
        rd_sum->restart_case = std::move(restart_case);
        rd_sum_data_add_case(rd_sum->data.get(),
                             rd_sum->restart_case->data.get());
    }
}

static bool rd_sum_fread(rd_sum_type *rd_sum, const std::string &header_file,
                         const stringlist_type *data_files,
                         bool include_restart, bool lazy_load,
                         int file_options) {
    rd_sum->smspec =
        read_smspec(header_file, rd_sum->key_join_string, include_restart);
    if (rd_sum->smspec) {
        bool fmt_file;
        rd_get_file_type(header_file.c_str(), &fmt_file, NULL);
        rd_sum->fmt_case = fmt_file;
    } else
        return false;

    if (rd_sum_fread_data(rd_sum, data_files, lazy_load, file_options)) {
        rd_file_enum file_type =
            rd_get_file_type(stringlist_iget(data_files, 0), NULL, NULL);

        if (file_type == RD_SUMMARY_FILE)
            rd_sum->unified = false;
        else if (file_type == RD_UNIFIED_SUMMARY_FILE)
            rd_sum->unified = true;
        else
            throw std::invalid_argument(
                "FileTypeError, should be RD_SUMMARY_FILE OR "
                "RD_UNIFIED_SUMMARY_FILE");
    } else
        return false;

    if (include_restart && rd_smspec_get_restart_case(rd_sum->smspec.get()))
        rd_sum_fread_history(rd_sum, lazy_load, file_options);

    return true;
}

static std::optional<fs::path> base_guess(std::string path) {
    stringlist_ptr data_files = make_stringlist();
    stringlist_ptr DATA_files = make_stringlist();
    stringlist_select_matching_files(data_files.get(), path.c_str(), "*.data");
    stringlist_select_matching_files(DATA_files.get(), path.c_str(), "*.DATA");

    if ((stringlist_get_size(data_files.get()) +
         stringlist_get_size(DATA_files.get())) == 1) {
        const char *path_name{};

        if (stringlist_get_size(data_files.get()) == 1)
            path_name = stringlist_iget(data_files.get(), 0);
        else
            path_name = stringlist_iget(DATA_files.get(), 0);

        return fs::path(path_name).stem();
    } // Else - found either 0 or more than 1 file with extension DATA - impossible to guess.
    return std::nullopt;
}

/**
  The stringlist will be cleared before the actual matching process
  starts. Observe that in addition to the @path input parameter the
  @base input can contain an embedded path component.
*/
static void rd_alloc_summary_data_files(const fs::path &path,
                                        const fs::path &base, bool fmt_file,
                                        stringlist_type *filelist) {
    std::string unif_data_file =
        rd::filename(path / base, RD_UNIFIED_SUMMARY_FILE, fmt_file, -1)
            .string();
    std::string pstr = path.string();
    std::string bstr = base.string();
    int files = rd_select_filelist(pstr.c_str(), bstr.c_str(), RD_SUMMARY_FILE,
                                   fmt_file, filelist);

    if ((files > 0) && fs::exists(unif_data_file)) {
        /*
         We have both a unified file AND a list of files: BASE.S0000,
         BASE.S0001, BASE.S0002, ..., must check which is newest and
         load accordingly.
        */
        bool unified_newest = true;
        int file_nr = 0;
        while (unified_newest && (file_nr < files)) {
            if (util_file_difftime(stringlist_iget(filelist, file_nr),
                                   unif_data_file.c_str()) > 0)
                unified_newest = false;
            file_nr++;
        }

        if (unified_newest) {
            stringlist_clear(
                filelist); /* Clear out all the BASE.Snnnn selections. */
            stringlist_append_copy(filelist, unif_data_file.c_str());
        }
    } else if (fs::exists(unif_data_file)) {
        /* Found a unified summary file :  Clear out all the BASE.Snnnn selections. */
        stringlist_clear(
            filelist); /* Clear out all the BASE.Snnnn selections. */
        stringlist_append_copy(filelist, unif_data_file.c_str());
    }
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
static std::optional<fs::path>
rd_alloc_summary_files(fs::path path, fs::path _base, std::string ext,
                       stringlist_type *filelist) {
    bool fmt_input = false;
    bool fmt_set = false;
    bool fmt_file = true;
    bool unif_input = false;
    bool unif_set = false;

    fs::path header_file;
    fs::path base;

    /* 1: We start by inspecting the input extension and see if we can
     learn anything about formatted/unformatted and
     unified/non-unified from this. The input extension can be NULL,
     in which case we learn nothing.
  */

    if (_base.empty()) {
        auto maybe_base = base_guess(path.string());
        if (!maybe_base)
            return std::nullopt;
        base = *maybe_base;
    } else
        base = _base;

    if (!ext.empty()) {
        rd_file_enum input_type;

        std::string filename = base.string() + ext;
        input_type = rd_get_file_type(filename.c_str(), &fmt_input, NULL);

        if ((input_type != RD_OTHER_FILE) && (input_type != RD_DATA_FILE)) {
            /*
             The file has been recognized as a file type from which we can
             at least infer formatted/unformatted information.
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

    std::string fsmspec_file =
        rd::filename(path / base, RD_SUMMARY_HEADER_FILE, true, -1).string();
    std::string smspec_file =
        rd::filename(path / base, RD_SUMMARY_HEADER_FILE, false, -1).string();

    /* Neither file exists */
    if (!rd::try_exists(fsmspec_file) && !rd::try_exists(smspec_file))
        return std::nullopt;

    if (fmt_set) /* The question of formatted|unformatted has already been settled based on the input filename. */
        fmt_file = fmt_input;
    else {
        /* Both fsmspec and smspec exist - we take the newest. */
        if (rd::try_exists(fsmspec_file) && rd::try_exists(smspec_file)) {
            if (util_file_difftime(fsmspec_file.c_str(), smspec_file.c_str()) <
                0)
                fmt_file = true;
            else
                fmt_file = false;
        } else { /* Only one of fsmspec / smspec exists */
            if (rd::try_exists(fsmspec_file))
                fmt_file = true;
            else
                fmt_file = false;
        }
    }

    if (fmt_file) {
        header_file = fsmspec_file;
    } else {
        header_file = smspec_file;
    }
    /* If you insist on e.g. unformatted and only fsmspec exists
     * no results for you. */
    if (!fs::exists(header_file))
        return std::nullopt;

    /*
     3: OK - we have found a SMSPEC / FMSPEC file - continue to look for
     XXX.Snnnn / XXX.UNSMRY files.
  */

    if (unif_set) { /* Based on the input file we have inferred whether to look for unified or
                     non-unified input files. */

        if (unif_input) {
            fs::path unif_data_file = rd::filename(
                path / base, RD_UNIFIED_SUMMARY_FILE, fmt_file, -1);
            if (fs::exists(unif_data_file)) {
                stringlist_append_copy(filelist,
                                       unif_data_file.string().c_str());
            }
        } else {
            std::string pstr = path.string();
            std::string bstr = base.string();
            rd_select_filelist(pstr.c_str(), bstr.c_str(), RD_SUMMARY_FILE,
                               fmt_file, filelist);
        }
    } else
        rd_alloc_summary_data_files(path, base, fmt_file, filelist);

    return header_file;
}

static bool rd_sum_fread_case(rd_sum_type *rd_sum, bool include_restart,
                              bool lazy_load, int file_options) {
    stringlist_ptr summary_file_list = make_stringlist();

    bool caseOK = false;

    auto header_file = rd_alloc_summary_files(
        rd_sum->path, rd_sum->base, rd_sum->ext, summary_file_list.get());
    if (header_file && (stringlist_get_size(summary_file_list.get()) > 0)) {
        caseOK =
            rd_sum_fread(rd_sum, header_file->string(), summary_file_list.get(),
                         include_restart, lazy_load, file_options);
    }

    return caseOK;
}

/**
   This will explicitly load the summary specified by @header_file and
   @data_files, i.e. if the case has been restarted from another case,
   it will NOT look for old summary information - that functionality
   is only invoked when using rd_sum_fread_alloc_case() function;
   however the list of data_files could in principle be achieved by
   initializing the data_files list with files from the restarted
   case.
*/

rd_sum_type *rd_sum_fread_alloc(const char *header_file,
                                const stringlist_type *data_files,
                                const char *key_join_string,
                                bool include_restart, bool lazy_load,
                                int file_options) {
    rd_sum_ptr rd_sum = rd_sum_alloc__(header_file, key_join_string);
    if (rd_sum) {
        if (!rd_sum_fread(rd_sum.get(), header_file, data_files,
                          include_restart, lazy_load, file_options)) {
            return nullptr;
        }
    }
    return rd_sum.release();
}

const rd::smspec_node *rd_sum_add_var(rd_sum_type *rd_sum, const char *keyword,
                                      const char *wgname, int num,
                                      const char *unit, float default_value) {
    if (rd_sum_data_get_length(rd_sum->data.get()) > 0)
        throw std::invalid_argument(
            "Can not interchange variable adding and timesteps.\n");

    return rd_smspec_add_node(rd_sum->smspec.get(), keyword, wgname, num, unit,
                              default_value);
}

const rd::smspec_node *rd_sum_add_local_var(rd_sum_type *rd_sum,
                                            const char *keyword,
                                            const char *wgname, int num,
                                            const char *unit, const char *lgr,
                                            int lgr_i, int lgr_j, int lgr_k,
                                            float default_value) {
    if (rd_sum_data_get_length(rd_sum->data.get()) > 0)
        throw std::invalid_argument(
            "Can not interchange variable adding and timesteps.\n");

    int params_index = rd_smspec_num_nodes(rd_sum->smspec.get());
    return rd_smspec_add_node(rd_sum->smspec.get(), params_index, keyword,
                              wgname, num, unit, lgr, lgr_i, lgr_j, lgr_k,
                              default_value);
}

const rd::smspec_node *rd_sum_add_smspec_node(rd_sum_type *rd_sum,
                                              const rd::smspec_node *node) {
    return rd_smspec_add_node(rd_sum->smspec.get(), *node);
}

/*
  Observe the time argument in rd_sum_add_tstep() and the bool flag
  time_in_days in make_summary_writer() can be misleading:

  - The time argument 'sim_seconds' to rd_sum_add_tstep() should
    *ALWAYS* be in seconds.

  - The 'sim_in_days' argument to the make_summary_writer() is just
    a very very basic unit support in the output. If sim_in_days ==
    true the output time unit will be days, otherwise it will be hours.
*/

rd_sum_tstep_type *rd_sum_add_tstep(rd_sum_type *rd_sum, int report_step,
                                    double sim_seconds) {
    rd_sum_tstep_type *new_tstep =
        rd_sum_data_add_new_tstep(rd_sum->data.get(), report_step, sim_seconds);
    return new_tstep;
}

rd_sum_ptr make_summary_writer(std::string rd_case, bool fmt_output,
                               bool unified, std::string key_join_string,
                               time_t sim_start, bool time_in_days, int nx,
                               int ny, int nz,
                               std::optional<std::string> restart_case,
                               int restart_step) {
    rd_sum_ptr rd_sum = rd_sum_alloc__(rd_case, key_join_string);
    if (rd_sum) {
        rd_sum->unified = unified;
        rd_sum->fmt_case = fmt_output;

        if (restart_case)
            rd_sum->smspec.reset(rd_smspec_alloc_restart_writer(
                key_join_string.c_str(), restart_case->c_str(), restart_step,
                sim_start, time_in_days, nx, ny, nz));
        else
            rd_sum->smspec.reset(rd_smspec_alloc_writer(
                key_join_string.c_str(), sim_start, time_in_days, nx, ny, nz));

        rd_sum->data.reset(rd_sum_data_alloc_writer(rd_sum->smspec.get()));
    }
    return rd_sum;
}

void rd_sum_fwrite(const rd_sum_type *rd_sum) {
    rd_smspec_fwrite(rd_sum->smspec.get(), rd_sum->rd_case.c_str(),
                     rd_sum->fmt_case);
    rd_sum_data_fwrite(rd_sum->data.get(), rd_sum->rd_case.c_str(),
                       rd_sum->fmt_case, rd_sum->unified);
}

bool rd_sum_can_write(const rd_sum_type *rd_sum) {
    return rd_sum_data_can_write(rd_sum->data.get());
}

void rd_sum_free(rd_sum_type *rd_sum) { delete rd_sum; }

/**
   This function takes an input file, and loads the corresponding
   summary. The function extracts the path part, and the basename from
   the input file. The extension is not considered (the input need not
   even be a valid file). In principle a simulation directory with a
   given basename can contain four different simulation cases:

    * Formatted and unformatted.
    * Unified and not unified.

   The program will load the most recent dataset, by looking at the
   modification time stamps of the files; if no simulation case is
   found the function will return NULL.

   If the SMSPEC file contains the RESTART keyword the function will
   iterate backwards to load summary information from previous runs
   (this is goverened by the local variable include_restart).
*/
rd_sum_type *rd_sum_fread_alloc_case(const char *input_file,
                                     const char *key_join_string,
                                     bool include_restart, bool lazy_load,
                                     int file_options) {
    rd_sum_ptr rd_sum = rd_sum_alloc__(input_file, key_join_string);
    if (!rd_sum)
        return nullptr;

    if (rd_sum_fread_case(rd_sum.get(), include_restart, lazy_load,
                          file_options))
        return rd_sum.release();
    else {
        return nullptr;
    }
}

double rd_sum_get_from_sim_time(const rd_sum_type *rd_sum, time_t sim_time,
                                const rd::smspec_node *node) {
    return rd_sum_data_get_from_sim_time(rd_sum->data.get(), sim_time, *node);
}

double rd_sum_time2days(const rd_sum_type *rd_sum, time_t sim_time) {
    return rd_sum_data_time2days(rd_sum->data.get(), sim_time);
}

const rd::smspec_node *rd_sum_get_general_var_node(const rd_sum_type *rd_sum,
                                                   const char *lookup_kw) {
    const rd::smspec_node &node =
        rd_smspec_get_general_var_node(rd_sum->smspec.get(), lookup_kw);
    return &node;
}

int rd_sum_get_general_var_params_index(const rd_sum_type *rd_sum,
                                        const char *lookup_kw) {
    return rd_smspec_get_general_var_params_index(rd_sum->smspec.get(),
                                                  lookup_kw);
}

bool rd_sum_has_general_var(const rd_sum_type *rd_sum, const char *lookup_kw) {
    return rd_smspec_has_general_var(rd_sum->smspec.get(), lookup_kw);
}

bool rd_sum_has_key(const rd_sum_type *rd_sum, const char *lookup_kw) {
    return rd_sum_has_general_var(rd_sum, lookup_kw);
}

double rd_sum_get_general_var(const rd_sum_type *rd_sum, int time_index,
                              const char *lookup_kw) {
    int params_index = rd_sum_get_general_var_params_index(rd_sum, lookup_kw);
    return rd_sum_data_iget(rd_sum->data.get(), time_index, params_index);
}

void rd_sum_get_interp_vector(const rd_sum_type *rd_sum, time_t sim_time,
                              const rd_sum_vector_type *key_words,
                              double_vector_type *data) {
    rd_sum_data_get_interp_vector(rd_sum->data.get(), sim_time, key_words,
                                  data);
}

void rd_sum_fwrite_interp_csv_line(const rd_sum_type *rd_sum, time_t sim_time,
                                   const rd_sum_vector_type *key_words,
                                   FILE *fp) {
    rd_sum_data_fwrite_interp_csv_line(rd_sum->data.get(), sim_time, key_words,
                                       fp);
}

double rd_sum_get_general_var_from_sim_time(const rd_sum_type *rd_sum,
                                            time_t sim_time, const char *var) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, var);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, node);
}

double rd_sum_get_general_var_from_sim_days(const rd_sum_type *rd_sum,
                                            double sim_days, const char *var) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, var);
    return rd_sum_data_get_from_sim_days(rd_sum->data.get(), sim_days, *node);
}

rd_sum_ptr rd_sum_alloc_resample(const rd_sum_type *rd_sum, const char *rd_case,
                                 const time_t_vector_type *times,
                                 bool lower_extrapolation,
                                 bool upper_extrapolation) {
    /*
    If lower and  / or upper extrapolation is set to true it makes sure that resampling returns the first / last value of the simulation
    or in the case of derivate / rate then it gets zero. if these are set to false, we jus throw exception
  */

    time_t start_time = rd_sum_get_data_start(rd_sum);
    time_t end_time = rd_sum_get_end_time(rd_sum);
    time_t input_start = time_t_vector_get_first(times);
    time_t input_end = time_t_vector_get_last(times);

    if (!lower_extrapolation && input_start < start_time)
        return {nullptr, &rd_sum_free};
    if (!upper_extrapolation && input_end > end_time)
        return {nullptr, &rd_sum_free};
    if (!time_t_vector_is_sorted(times, false))
        return {nullptr, &rd_sum_free};

    const int *grid_dims = rd_smspec_get_grid_dims(rd_sum->smspec.get());

    bool time_in_days = false;
    const rd::smspec_node &node =
        rd_smspec_iget_node_w_node_index(rd_sum->smspec.get(), 0);
    if (util_string_equal(smspec_node_get_unit(&node), "DAYS"))
        time_in_days = true;

    //create elc_sum_resampled with TIME node only
    rd_sum_ptr rd_sum_resampled = make_summary_writer(
        rd_case, rd_sum->fmt_case, rd_sum->unified,
        rd_sum->key_join_string.c_str(), input_start, time_in_days,
        grid_dims[0], grid_dims[1], grid_dims[2]);

    //add remaining nodes
    for (int i = 0; i < rd_smspec_num_nodes(rd_sum->smspec.get()); i++) {
        const rd::smspec_node &node =
            rd_smspec_iget_node_w_node_index(rd_sum->smspec.get(), i);
        if (util_string_equal(smspec_node_get_gen_key1(&node), "TIME"))
            continue;

        rd_sum_add_smspec_node(rd_sum_resampled.get(), &node);
    }

    /*
    The SMSPEC header structure has been completely initialized, it is time to
    start filling it up with data.

  */
    rd_sum_vector_ptr rd_sum_vector = make_sum_vector(rd_sum, true);
    auto data =
        make_double_vector(rd_sum_vector_get_size(rd_sum_vector.get()), 0);

    for (int report_step = 0; report_step < time_t_vector_size(times);
         report_step++) {
        time_t input_t = time_t_vector_iget(times, report_step);
        if (input_t < start_time) {
            //clamping to the first value for t < start_time or if it is a rate than derivative is 0
            for (int i = 1; i < rd_smspec_num_nodes(rd_sum->smspec.get());
                 i++) {
                double value = 0;
                const rd::smspec_node &node =
                    rd_smspec_iget_node_w_node_index(rd_sum->smspec.get(), i);
                if (!node.is_rate())
                    value = rd_sum_data_iget_first_value(
                        rd_sum->data.get(), node.get_params_index());
                double_vector_iset(data.get(), i - 1, value);
            }
        } else if (input_t > end_time) {
            //clamping to the last value for t > end_time or if it is a rate than derivative is 0
            for (int i = 1; i < rd_smspec_num_nodes(rd_sum->smspec.get());
                 i++) {
                double value = 0;
                const rd::smspec_node &node =
                    rd_smspec_iget_node_w_node_index(rd_sum->smspec.get(), i);
                if (!node.is_rate())
                    value = rd_sum_data_iget_last_value(
                        rd_sum->data.get(), node.get_params_index());
                double_vector_iset(data.get(), i - 1, value);
            }
        } else {
            /* Look up interpolated data in the original case. */
            rd_sum_get_interp_vector(rd_sum, input_t, rd_sum_vector.get(),
                                     data.get());
        }

        /* Add timestep corresponding to the interpolated data in the resampled case. */
        rd_sum_tstep_type *tstep = rd_sum_add_tstep(
            rd_sum_resampled.get(), report_step, input_t - input_start);
        for (int data_index = 0;
             data_index < rd_sum_vector_get_size(rd_sum_vector.get());
             data_index++) {
            double value = double_vector_iget(data.get(), data_index);
            int params_index =
                data_index +
                1; // The +1 shift is because the first element in the tstep is time value.
            rd_sum_tstep_iset(tstep, params_index, value);
        }
    }
    return rd_sum_resampled;
}

double rd_sum_iget(const rd_sum_type *rd_sum, int time_index, int param_index) {
    return rd_sum_data_iget(rd_sum->data.get(), time_index, param_index);
}

rd_smspec_var_type rd_sum_identify_var_type(const char *var) {
    return rd_smspec_identify_var_type(var);
}

const char *rd_sum_get_unit(const rd_sum_type *rd_sum, const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_get_unit(node);
}

int rd_sum_get_last_report_step(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_last_report_step(rd_sum->data.get());
}

int rd_sum_get_first_report_step(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_first_report_step(rd_sum->data.get());
}

int rd_sum_iget_report_end(const rd_sum_type *rd_sum, int report_step) {
    return rd_sum_data_iget_report_end(rd_sum->data.get(), report_step);
}

int rd_sum_iget_report_step(const rd_sum_type *rd_sum, int internal_index) {
    return rd_sum_data_iget_report_step(rd_sum->data.get(), internal_index);
}

time_t_vector_type *rd_sum_alloc_time_vector(const rd_sum_type *rd_sum,
                                             bool report_only) {
    return rd_sum_data_alloc_time_vector(rd_sum->data.get(), report_only);
}

void rd_sum_init_double_vector(const rd_sum_type *rd_sum, const char *gen_key,
                               double *data) {
    int params_index = rd_sum_get_general_var_params_index(rd_sum, gen_key);
    rd_sum_data_init_double_vector(rd_sum->data.get(), params_index, data);
}

void rd_sum_init_double_vector_interp(const rd_sum_type *rd_sum,
                                      const char *gen_key,
                                      const time_t_vector_type *time_points,
                                      double *data) {
    const rd::smspec_node &node =
        rd_smspec_get_general_var_node(rd_sum->smspec.get(), gen_key);
    rd_sum_data_init_double_vector_interp(rd_sum->data.get(), node, time_points,
                                          data);
}

void rd_sum_init_datetime64_vector(const rd_sum_type *rd_sum, int64_t *data,
                                   int multiplier) {
    rd_sum_data_init_datetime64_vector(rd_sum->data.get(), data, multiplier);
}

void rd_sum_init_double_frame(const rd_sum_type *rd_sum,
                              const rd_sum_vector_type *keywords,
                              double *data) {
    rd_sum_data_init_double_frame(rd_sum->data.get(), keywords, data);
}

void rd_sum_init_double_frame_interp(const rd_sum_type *rd_sum,
                                     const rd_sum_vector_type *keywords,
                                     const time_t_vector_type *time_points,
                                     double *data) {
    rd_sum_data_init_double_frame_interp(rd_sum->data.get(), keywords,
                                         time_points, data);
}

double_vector_type *rd_sum_alloc_data_vector(const rd_sum_type *rd_sum,
                                             int data_index, bool report_only) {
    return rd_sum_data_alloc_data_vector(rd_sum->data.get(), data_index,
                                         report_only);
}

/**
   Returns the first internal index where a limiting value is
   reached. If the limiting value is never reached, -1 is
   returned. The smspec_index should be calculated first with one of
   the

      rd_sum_get_XXXX_index()

   functions.

*/

static int rd_sum_get_limiting(const rd_sum_type *rd_sum, int smspec_index,
                               double limit, bool gt) {
    const int length = rd_sum_data_get_length(rd_sum->data.get());
    int internal_index = 0;
    do {
        double value =
            rd_sum_data_iget(rd_sum->data.get(), internal_index, smspec_index);
        if (gt) {
            if (value > limit)
                break;
        } else {
            if (value < limit)
                break;
        }
        internal_index++;
    } while (internal_index < length);

    if (internal_index == length) /* Did not find it */
        internal_index = -1;

    return internal_index;
}

int rd_sum_get_first_gt(const rd_sum_type *rd_sum, int param_index,
                        double limit) {
    return rd_sum_get_limiting(rd_sum, param_index, limit, true);
}

int rd_sum_get_first_lt(const rd_sum_type *rd_sum, int param_index,
                        double limit) {
    return rd_sum_get_limiting(rd_sum, param_index, limit, false);
}

time_t rd_sum_get_report_time(const rd_sum_type *rd_sum, int report_step) {
    return rd_sum_data_get_report_time(rd_sum->data.get(), report_step);
}

time_t rd_sum_iget_sim_time(const rd_sum_type *rd_sum, int index) {
    return rd_sum_data_iget_sim_time(rd_sum->data.get(), index);
}

time_t rd_sum_get_data_start(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_data_start(rd_sum->data.get());
}

time_t rd_sum_get_start_time(const rd_sum_type *rd_sum) {
    return rd_smspec_get_start_time(rd_sum->smspec.get());
}

time_t rd_sum_get_end_time(const rd_sum_type *rd_sum) {
    try {
        return rd_sum_data_get_sim_end(rd_sum->data.get());
    } catch (std::out_of_range const &) {
        return rd_smspec_get_start_time(rd_sum->smspec.get());
    }
}

double rd_sum_iget_sim_days(const rd_sum_type *rd_sum, int index) {
    return rd_sum_data_iget_sim_days(rd_sum->data.get(), index);
}

/*#define DAYS_DATE_FORMAT    "%7.2f   %02d/%02d/%04d   "
#define FLOAT_FORMAT        " %15.6g "
#define HEADER_FORMAT       " %15s "
#define DATE_DASH           "-----------------------"
#define FLOAT_DASH          "-----------------"
*/

#define DATE_HEADER "-- Days   dd/mm/yyyy   "
#define DATE_STRING_LENGTH 128

static void __rd_sum_fprintf_line(const rd_sum_type *rd_sum, FILE *stream,
                                  int internal_index,
                                  const bool_vector_type *has_var,
                                  const int_vector_type *var_index,
                                  char *date_string, const char *date_format,
                                  const char *sep) {
    fprintf(stream, "%7.2f", rd_sum_iget_sim_days(rd_sum, internal_index));
    fprintf(stream, "%s", sep);

    {
        struct tm ts;
        time_t sim_time = rd_sum_iget_sim_time(rd_sum, internal_index);
        util_time_utc(&sim_time, &ts);
        strftime(date_string, DATE_STRING_LENGTH - 1, date_format, &ts);
        fprintf(stream, "%s", date_string);
    }

    for (int ivar = 0; ivar < int_vector_size(var_index); ivar++) {
        if (bool_vector_iget(has_var, ivar)) {
            fprintf(stream, "%s", sep);
            fprintf(stream, "%g",
                    rd_sum_iget(rd_sum, internal_index,
                                int_vector_iget(var_index, ivar)));
        }
    }

    fprintf(stream, "\r\n");
}

static void rd_sum_fprintf_header(const rd_sum_type *rd_sum,
                                  const stringlist_type *key_list,
                                  const bool_vector_type *has_var, FILE *stream,
                                  const char *sep) {
    fprintf(stream, "DAYS%sDATE", sep);
    for (int i = 0; i < stringlist_get_size(key_list); i++)
        if (bool_vector_iget(has_var, i)) {
            fprintf(stream, "%s", sep);
            fprintf(stream, "%s", stringlist_iget(key_list, i));
        }
    fprintf(stream, "\r\n");
}

static void rd_sum_fprintf(const rd_sum_type *rd_sum, FILE *stream,
                           const stringlist_type *var_list,
                           const char *date_format, const char *sep) {
    auto has_var = make_bool_vector(stringlist_get_size(var_list), false);
    auto var_index = make_int_vector(stringlist_get_size(var_list), -1);
    auto date_string = rd::checked_calloc<char>(DATE_STRING_LENGTH);

    for (int ivar = 0; ivar < stringlist_get_size(var_list); ivar++) {
        if (rd_sum_has_general_var(rd_sum, stringlist_iget(var_list, ivar))) {
            bool_vector_iset(has_var.get(), ivar, true);
            int_vector_iset(var_index.get(), ivar,
                            rd_sum_get_general_var_params_index(
                                rd_sum, stringlist_iget(var_list, ivar)));
        } else {
            fprintf(stderr,
                    "** Warning: could not find variable: \'%s\' in "
                    "summary file \n",
                    stringlist_iget(var_list, ivar));
            bool_vector_iset(has_var.get(), ivar, false);
        }
    }

    rd_sum_fprintf_header(rd_sum, var_list, has_var.get(), stream, sep);

    for (int time_index = 0; time_index < rd_sum_get_data_length(rd_sum);
         time_index++)
        __rd_sum_fprintf_line(rd_sum, stream, time_index, has_var.get(),
                              var_index.get(), date_string.get(), date_format,
                              sep);
}
#undef DATE_STRING_LENGTH

void rd_sum_export_csv(const rd_sum_type *rd_sum, const char *filename,
                       const stringlist_type *var_list, const char *date_format,
                       const char *sep) {
    FILE *stream = util_mkdir_fopen(filename, "w");
    rd_sum_fprintf(rd_sum, stream, var_list, date_format, sep);
    fclose(stream);
}

const rd_sum_type *rd_sum_get_restart_case(const rd_sum_type *rd_sum) {
    return rd_sum->restart_case.get();
}

int rd_sum_get_restart_step(const rd_sum_type *rd_sum) {
    return rd_smspec_get_restart_step(rd_sum->smspec.get());
}

const char *rd_sum_get_case(const rd_sum_type *rd_sum) {
    return rd_sum->rd_case.c_str();
}

const char *rd_sum_get_path(const rd_sum_type *rd_sum) {
    if (rd_sum->path.empty())
        return nullptr;
    return rd_sum->path.c_str();
}

const char *rd_sum_get_abs_path(const rd_sum_type *rd_sum) {
    if (rd_sum->abs_path.empty())
        return nullptr;
    return rd_sum->abs_path.c_str();
}

const char *rd_sum_get_base(const rd_sum_type *rd_sum) {
    if (rd_sum->base.empty())
        return nullptr;
    return rd_sum->base.c_str();
}

stringlist_type *
rd_sum_alloc_matching_general_var_list(const rd_sum_type *rd_sum,
                                       const char *pattern) {
    return rd_smspec_alloc_matching_general_var_list(rd_sum->smspec.get(),
                                                     pattern);
}

void rd_sum_select_matching_general_var_list(const rd_sum_type *rd_sum,
                                             const char *pattern,
                                             stringlist_type *keys) {
    rd_smspec_select_matching_general_var_list(rd_sum->smspec.get(), pattern,
                                               keys);
}

stringlist_type *rd_sum_alloc_well_list(const rd_sum_type *rd_sum,
                                        const char *pattern) {
    return rd_smspec_alloc_well_list(rd_sum->smspec.get(), pattern);
}

stringlist_type *rd_sum_alloc_group_list(const rd_sum_type *rd_sum,
                                         const char *pattern) {
    return rd_smspec_alloc_group_list(rd_sum->smspec.get(), pattern);
}

double rd_sum_get_first_day(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_first_day(rd_sum->data.get());
}

double rd_sum_get_sim_length(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_sim_length(rd_sum->data.get());
}

/**
   Will return the number of data blocks.
*/
int rd_sum_get_data_length(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_length(rd_sum->data.get());
}

bool rd_sum_check_sim_time(const rd_sum_type *sum, time_t sim_time) {
    return rd_sum_data_check_sim_time(sum->data.get(), sim_time);
}

bool rd_sum_check_sim_days(const rd_sum_type *sum, double sim_days) {
    return rd_sum_data_check_sim_days(sum->data.get(), sim_days);
}

int rd_sum_get_report_step_from_time(const rd_sum_type *sum, time_t sim_time) {
    return rd_sum_data_get_report_step_from_time(sum->data.get(), sim_time);
}

int rd_sum_get_report_step_from_days(const rd_sum_type *sum, double sim_days) {
    return rd_sum_data_get_report_step_from_days(sum->data.get(), sim_days);
}

rd_smspec_type *rd_sum_get_smspec(const rd_sum_type *rd_sum) {
    return rd_sum->smspec.get();
}

/*
   The functions below are extremly simple functions which only serve
   as an easy access to the smspec_alloc_xxx_key() functions which
   know how to create the various composite keys.
*/

static double_vector_type *
rd_sum_alloc_seconds_solution(const rd_sum_type *rd_sum, const char *gen_key,
                              double cmp_value, bool rates_clamp_lower) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return rd_sum_data_alloc_seconds_solution(rd_sum->data.get(), *node,
                                              cmp_value, rates_clamp_lower);
}

double_vector_type *rd_sum_alloc_days_solution(const rd_sum_type *rd_sum,
                                               const char *gen_key,
                                               double cmp_value,
                                               bool rates_clamp_lower) {
    double_vector_type *solution = rd_sum_alloc_seconds_solution(
        rd_sum, gen_key, cmp_value, rates_clamp_lower);
    double_vector_scale(solution, 1.0 / 86400);
    return solution;
}

time_t_vector_type *rd_sum_alloc_time_solution(const rd_sum_type *rd_sum,
                                               const char *gen_key,
                                               double cmp_value,
                                               bool rates_clamp_lower) {
    auto solution = make_time_t_vector(0, 0);
    {
        std::unique_ptr<double_vector_type, decltype(&double_vector_free)>
            seconds{rd_sum_alloc_seconds_solution(rd_sum, gen_key, cmp_value,
                                                  rates_clamp_lower),
                    &double_vector_free};
        time_t start_time = rd_sum_get_start_time(rd_sum);
        for (int i = 0; i < double_vector_size(seconds.get()); i++) {
            time_t t = start_time;
            util_inplace_forward_seconds_utc(
                &t, double_vector_iget(seconds.get(), i));
            time_t_vector_append(solution.get(), t);
        }
    }
    return solution.release();
}

ert_rd_unit_enum rd_sum_get_unit_system(const rd_sum_type *rd_sum) {
    return rd_smspec_get_unit_system(rd_sum->smspec.get());
}

double rd_sum_get_last_value_gen_key(const rd_sum_type *rd_sum,
                                     const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return rd_sum_data_iget_last_value(rd_sum->data.get(),
                                       smspec_node_get_params_index(node));
}

double rd_sum_get_first_value_gen_key(const rd_sum_type *rd_sum,
                                      const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return rd_sum_data_iget_first_value(rd_sum->data.get(),
                                        smspec_node_get_params_index(node));
}

rd_sum_ptr read_summary(const std::string &filename,
                        const std::string &key_join_string, bool lazy_load,
                        bool include_restart, int file_options) {
    return {rd_sum_fread_alloc_case(filename.c_str(), key_join_string.c_str(),
                                    include_restart, lazy_load, file_options),
            &rd_sum_free};
}
