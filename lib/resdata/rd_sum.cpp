#include <stdexcept>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <locale.h>

#include <ert/util/hash.hpp>
#include <ert/util/util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_sum.hpp>

#include <resdata/rd_sum_data.hpp>
#include <resdata/rd_smspec.hpp>
#include <resdata/rd_sum_data.hpp>
#include <resdata/smspec_node.hpp>

#include "detail/util/path.hpp"

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
    rd_smspec_type *smspec; /* Internalized version of the SMSPEC file. */
    rd_sum_data_type *data; /* The data - can be NULL. */
    rd_sum_type *restart_case;

    bool fmt_case;
    bool unified;
    char *key_join_string;
    char *
        path; /* The path - as given for the case input. Can be NULL for cwd. */
    char *abs_path; /* Absolute path. */
    char *base;     /* Only the basename. */
    char *
        rd_case; /* This is the current case, with optional path component. == path + base*/
    char *
        ext; /* Only to support selective loading of formatted|unformatted and unified|multiple. (can be NULL) */
};

UTIL_SAFE_CAST_FUNCTION(rd_sum, RD_SUM_ID);
UTIL_IS_INSTANCE_FUNCTION(rd_sum, RD_SUM_ID);

/**
   Reads the data from summary files, can either be a list of
   files BASE.S0000, BASE.S0001, BASE.S0002,.. or one unified
   file. Formatted/unformatted is detected automagically.

   The actual loading is implemented in the rd_sum_data.c file.
*/

void rd_sum_set_case(rd_sum_type *rd_sum, const char *input_arg) {
    free(rd_sum->rd_case);
    free(rd_sum->path);
    free(rd_sum->abs_path);
    free(rd_sum->base);
    free(rd_sum->ext);
    {
        std::string path = rd::util::path::dirname(input_arg);
        std::string base = rd::util::path::basename(input_arg);
        std::string ext = rd::util::path::extension(input_arg);

        rd_sum->rd_case = util_alloc_filename(path.c_str(), base.c_str(), NULL);
        if (path.size())
            rd_sum->path = util_alloc_string_copy(path.c_str());
        else
            rd_sum->path = NULL;

        if (base.size())
            rd_sum->base = util_alloc_string_copy(base.c_str());
        else
            rd_sum->base = NULL;

        if (ext.size())
            rd_sum->ext = util_alloc_string_copy(ext.c_str());
        else
            rd_sum->ext = NULL;

        if (path.size() > 0)
            rd_sum->abs_path = util_alloc_abs_path(path.c_str());
        else
            rd_sum->abs_path = util_alloc_cwd();
    }
}

static rd_sum_type *rd_sum_alloc__(const char *input_arg,
                                   const char *key_join_string) {
    if (!rd_path_access(input_arg))
        return NULL;

    rd_sum_type *rd_sum = (rd_sum_type *)util_malloc(sizeof *rd_sum);
    UTIL_TYPE_ID_INIT(rd_sum, RD_SUM_ID);

    rd_sum->rd_case = NULL;
    rd_sum->path = NULL;
    rd_sum->base = NULL;
    rd_sum->ext = NULL;
    rd_sum->abs_path = NULL;
    rd_sum_set_case(rd_sum, input_arg);
    rd_sum->key_join_string = util_alloc_string_copy(key_join_string);

    rd_sum->smspec = NULL;
    rd_sum->data = NULL;
    rd_sum->restart_case = NULL;

    return rd_sum;
}

static bool rd_sum_fread_data(rd_sum_type *rd_sum,
                              const stringlist_type *data_files,
                              bool include_restart, bool lazy_load,
                              int file_options) {
    if (rd_sum->data != NULL)
        rd_sum_free_data(rd_sum);

    rd_sum->data = rd_sum_data_alloc(rd_sum->smspec);
    return rd_sum_data_fread(rd_sum->data, data_files, lazy_load, file_options);
}

static void rd_sum_fread_history(rd_sum_type *rd_sum, bool lazy_load,
                                 int file_options) {
    char *restart_header = rd_alloc_filename(
        NULL, rd_smspec_get_restart_case(rd_sum->smspec),
        RD_SUMMARY_HEADER_FILE, rd_smspec_get_formatted(rd_sum->smspec), -1);
    rd_sum_type *restart_case = rd_sum_fread_alloc_case2__(
        restart_header, ":", true, lazy_load, file_options);
    if (restart_case) {
        rd_sum->restart_case = restart_case;
        rd_sum_data_add_case(rd_sum->data, restart_case->data);
    }
    free(restart_header);
}

static bool rd_sum_fread(rd_sum_type *rd_sum, const char *header_file,
                         const stringlist_type *data_files,
                         bool include_restart, bool lazy_load,
                         int file_options) {
    rd_sum->smspec = rd_smspec_fread_alloc(header_file, rd_sum->key_join_string,
                                           include_restart);
    if (rd_sum->smspec) {
        bool fmt_file;
        rd_get_file_type(header_file, &fmt_file, NULL);
        rd_sum_set_fmt_case(rd_sum, fmt_file);
    } else
        return false;

    if (rd_sum_fread_data(rd_sum, data_files, include_restart, lazy_load,
                          file_options)) {
        rd_file_enum file_type =
            rd_get_file_type(stringlist_iget(data_files, 0), NULL, NULL);

        if (file_type == RD_SUMMARY_FILE)
            rd_sum_set_unified(rd_sum, false);
        else if (file_type == RD_UNIFIED_SUMMARY_FILE)
            rd_sum_set_unified(rd_sum, true);
        else
            util_abort("%s: FileTypeError, should be RD_SUMMARY_FILE OR "
                       "RD_UNIFIED_SUMMARY_FILE \n",
                       __func__);
    } else
        return false;

    if (include_restart && rd_smspec_get_restart_case(rd_sum->smspec))
        rd_sum_fread_history(rd_sum, lazy_load, file_options);

    return true;
}

static bool rd_sum_fread_case(rd_sum_type *rd_sum, bool include_restart,
                              bool lazy_load, int file_options) {
    char *header_file;
    stringlist_type *summary_file_list = stringlist_alloc_new();

    bool caseOK = false;

    rd_alloc_summary_files(rd_sum->path, rd_sum->base, rd_sum->ext,
                           &header_file, summary_file_list);
    if ((header_file != NULL) && (stringlist_get_size(summary_file_list) > 0)) {
        caseOK = rd_sum_fread(rd_sum, header_file, summary_file_list,
                              include_restart, lazy_load, file_options);
    }
    free(header_file);
    stringlist_free(summary_file_list);

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
    rd_sum_type *rd_sum = rd_sum_alloc__(header_file, key_join_string);
    if (rd_sum) {
        if (!rd_sum_fread(rd_sum, header_file, data_files, include_restart,
                          lazy_load, file_options)) {
            rd_sum_free(rd_sum);
            rd_sum = NULL;
        }
    }
    return rd_sum;
}

void rd_sum_set_unified(rd_sum_type *rd_sum, bool unified) {
    rd_sum->unified = unified;
}

void rd_sum_set_fmt_case(rd_sum_type *rd_sum, bool fmt_case) {
    rd_sum->fmt_case = fmt_case;
}

const rd::smspec_node *rd_sum_add_var(rd_sum_type *rd_sum, const char *keyword,
                                      const char *wgname, int num,
                                      const char *unit, float default_value) {
    if (rd_sum_data_get_length(rd_sum->data) > 0)
        throw std::invalid_argument(
            "Can not interchange variable adding and timesteps.\n");

    return rd_smspec_add_node(rd_sum->smspec, keyword, wgname, num, unit,
                              default_value);
}

const rd::smspec_node *rd_sum_add_local_var(rd_sum_type *rd_sum,
                                            const char *keyword,
                                            const char *wgname, int num,
                                            const char *unit, const char *lgr,
                                            int lgr_i, int lgr_j, int lgr_k,
                                            float default_value) {
    if (rd_sum_data_get_length(rd_sum->data) > 0)
        throw std::invalid_argument(
            "Can not interchange variable adding and timesteps.\n");

    int params_index = rd_smspec_num_nodes(rd_sum->smspec);
    return rd_smspec_add_node(rd_sum->smspec, params_index, keyword, wgname,
                              num, unit, lgr, lgr_i, lgr_j, lgr_k,
                              default_value);
}

const rd::smspec_node *rd_sum_add_smspec_node(rd_sum_type *rd_sum,
                                              const rd::smspec_node *node) {
    return rd_smspec_add_node(rd_sum->smspec, *node);
}

/*
  Observe the time argument in rd_sum_add_tstep() and the bool flag
  time_in_days in rd_sum_alloc_writer() can be misleading:

  - The time argument 'sim_seconds' to rd_sum_add_tstep() should
    *ALWAYS* be in seconds.

  - The 'sim_in_days' argument to the rd_sum_alloc_writer( ) is just
    a very very basic unit support in the output. If sim_in_days ==
    true the output time unit will be days, otherwise it will be hours.
*/

rd_sum_tstep_type *rd_sum_add_tstep(rd_sum_type *rd_sum, int report_step,
                                    double sim_seconds) {
    rd_sum_tstep_type *new_tstep =
        rd_sum_data_add_new_tstep(rd_sum->data, report_step, sim_seconds);
    return new_tstep;
}

static rd_sum_type *
rd_sum_alloc_writer__(const char *rd_case, const char *restart_case,
                      int restart_step, bool fmt_output, bool unified,
                      const char *key_join_string, time_t sim_start,
                      bool time_in_days, int nx, int ny, int nz) {

    rd_sum_type *rd_sum = rd_sum_alloc__(rd_case, key_join_string);
    if (rd_sum) {
        rd_sum_set_unified(rd_sum, unified);
        rd_sum_set_fmt_case(rd_sum, fmt_output);

        if (restart_case)
            rd_sum->smspec = rd_smspec_alloc_restart_writer(
                key_join_string, restart_case, restart_step, sim_start,
                time_in_days, nx, ny, nz);
        else
            rd_sum->smspec = rd_smspec_alloc_writer(key_join_string, sim_start,
                                                    time_in_days, nx, ny, nz);

        rd_sum->data = rd_sum_data_alloc_writer(rd_sum->smspec);
    }
    return rd_sum;
}

rd_sum_type *
rd_sum_alloc_restart_writer2(const char *rd_case, const char *restart_case,
                             int restart_step, bool fmt_output, bool unified,
                             const char *key_join_string, time_t sim_start,
                             bool time_in_days, int nx, int ny, int nz) {
    return rd_sum_alloc_writer__(rd_case, restart_case, restart_step,
                                 fmt_output, unified, key_join_string,
                                 sim_start, time_in_days, nx, ny, nz);
}

/*
  This does not take in the restart_step argument is depcrecated. You should use the
  rd_sum_alloc_restart_writer2() function.
*/

rd_sum_type *rd_sum_alloc_restart_writer(const char *rd_case,
                                         const char *restart_case,
                                         bool fmt_output, bool unified,
                                         const char *key_join_string,
                                         time_t sim_start, bool time_in_days,
                                         int nx, int ny, int nz) {
    int restart_step = 0;
    return rd_sum_alloc_writer__(rd_case, restart_case, restart_step,
                                 fmt_output, unified, key_join_string,
                                 sim_start, time_in_days, nx, ny, nz);
}

rd_sum_type *rd_sum_alloc_writer(const char *rd_case, bool fmt_output,
                                 bool unified, const char *key_join_string,
                                 time_t sim_start, bool time_in_days, int nx,
                                 int ny, int nz) {
    return rd_sum_alloc_writer__(rd_case, NULL, 0, fmt_output, unified,
                                 key_join_string, sim_start, time_in_days, nx,
                                 ny, nz);
}

void rd_sum_fwrite(const rd_sum_type *rd_sum) {
    rd_sum_fwrite_smspec(rd_sum);
    rd_sum_data_fwrite(rd_sum->data, rd_sum->rd_case, rd_sum->fmt_case,
                       rd_sum->unified);
}

bool rd_sum_can_write(const rd_sum_type *rd_sum) {
    return rd_sum_data_can_write(rd_sum->data);
}

void rd_sum_fwrite_smspec(const rd_sum_type *rd_sum) {
    rd_smspec_fwrite(rd_sum->smspec, rd_sum->rd_case, rd_sum->fmt_case);
}

/**
   This function frees the data from the rd_sum instance and sets the
   data pointer to NULL. The SMSPEC data is still valid, and can be
   reused with calls to rd_sum_fread_realloc_data().
*/

void rd_sum_free_data(rd_sum_type *rd_sum) {
    rd_sum_data_free(rd_sum->data);
    rd_sum->data = NULL;
}

void rd_sum_free(rd_sum_type *rd_sum) {
    if (rd_sum->restart_case)
        rd_sum_free(rd_sum->restart_case);

    if (rd_sum->data)
        rd_sum_free_data(rd_sum);

    if (rd_sum->smspec)
        rd_smspec_free(rd_sum->smspec);

    free(rd_sum->path);
    free(rd_sum->ext);
    free(rd_sum->abs_path);

    free(rd_sum->base);
    free(rd_sum->rd_case);

    free(rd_sum->key_join_string);
    free(rd_sum);
}

void rd_sum_free__(void *__rd_sum) {
    rd_sum_type *rd_sum = rd_sum_safe_cast(__rd_sum);
    rd_sum_free(rd_sum);
}

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

rd_sum_type *rd_sum_fread_alloc_case2__(const char *input_file,
                                        const char *key_join_string,
                                        bool include_restart, bool lazy_load,
                                        int file_options) {
    rd_sum_type *rd_sum = rd_sum_alloc__(input_file, key_join_string);
    if (!rd_sum)
        return NULL;

    if (rd_sum_fread_case(rd_sum, include_restart, lazy_load, file_options))
        return rd_sum;
    else {
        /*
      Loading a case failed - we discard the partly initialized
      rd_sum structure and jump ship.
    */
        rd_sum_free(rd_sum);
        return NULL;
    }
}

rd_sum_type *rd_sum_fread_alloc_case__(const char *input_file,
                                       const char *key_join_string,
                                       bool include_restart) {
    bool lazy_load = true;
    int file_options = 0;
    return rd_sum_fread_alloc_case2__(input_file, key_join_string,
                                      include_restart, lazy_load, file_options);
}

rd_sum_type *rd_sum_fread_alloc_case(const char *input_file,
                                     const char *key_join_string) {
    bool include_restart = true;
    bool lazy_load = true;
    int file_options = 0;
    return rd_sum_fread_alloc_case2__(input_file, key_join_string,
                                      include_restart, lazy_load, file_options);
}

bool rd_sum_case_exists(const char *input_file) {
    char *smspec_file = NULL;
    stringlist_type *data_files = stringlist_alloc_new();
    char *path;
    char *basename;
    char *extension;
    bool case_exists;

    util_alloc_file_components(input_file, &path, &basename, &extension);
    case_exists = rd_alloc_summary_files(path, basename, extension,
                                         &smspec_file, data_files);

    free(path);
    free(basename);
    free(extension);
    free(smspec_file);
    stringlist_free(data_files);

    return case_exists;
}

double rd_sum_get_from_sim_time(const rd_sum_type *rd_sum, time_t sim_time,
                                const rd::smspec_node *node) {
    return rd_sum_data_get_from_sim_time(rd_sum->data, sim_time, *node);
}

double rd_sum_get_from_sim_days(const rd_sum_type *rd_sum, double sim_days,
                                const rd::smspec_node *node) {
    return rd_sum_data_get_from_sim_days(rd_sum->data, sim_days, *node);
}

double rd_sum_time2days(const rd_sum_type *rd_sum, time_t sim_time) {
    return rd_sum_data_time2days(rd_sum->data, sim_time);
}

bool rd_sum_has_well_var(const rd_sum_type *rd_sum, const char *well,
                         const char *var) {
    return rd_smspec_has_well_var(rd_sum->smspec, well, var);
}

double rd_sum_get_well_var(const rd_sum_type *rd_sum, int time_index,
                           const char *well, const char *var) {
    int params_index =
        rd_smspec_get_well_var_params_index(rd_sum->smspec, well, var);
    return rd_sum_data_iget(rd_sum->data, time_index, params_index);
}

double rd_sum_get_well_var_from_sim_time(const rd_sum_type *rd_sum,
                                         time_t sim_time, const char *well,
                                         const char *var) {
    const rd::smspec_node &node =
        rd_smspec_get_well_var_node(rd_sum->smspec, well, var);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, &node);
}

double rd_sum_get_well_var_from_sim_days(const rd_sum_type *rd_sum,
                                         double sim_days, const char *well,
                                         const char *var) {
    const rd::smspec_node &node =
        rd_smspec_get_well_var_node(rd_sum->smspec, well, var);
    return rd_sum_get_from_sim_days(rd_sum, sim_days, &node);
}

bool rd_sum_has_group_var(const rd_sum_type *rd_sum, const char *group,
                          const char *var) {
    return rd_smspec_has_group_var(rd_sum->smspec, group, var);
}

double rd_sum_get_group_var(const rd_sum_type *rd_sum, int time_index,
                            const char *group, const char *var) {
    int params_index =
        rd_smspec_get_group_var_params_index(rd_sum->smspec, group, var);
    return rd_sum_data_iget(rd_sum->data, time_index, params_index);
}

double rd_sum_get_group_var_from_sim_time(const rd_sum_type *rd_sum,
                                          time_t sim_time, const char *group,
                                          const char *var) {
    const rd::smspec_node &node =
        rd_smspec_get_group_var_node(rd_sum->smspec, group, var);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, &node);
}

double rd_sum_get_group_var_from_sim_days(const rd_sum_type *rd_sum,
                                          double sim_days, const char *group,
                                          const char *var) {
    const rd::smspec_node &node =
        rd_smspec_get_group_var_node(rd_sum->smspec, group, var);
    return rd_sum_get_from_sim_days(rd_sum, sim_days, &node);
}

bool rd_sum_has_field_var(const rd_sum_type *rd_sum, const char *var) {
    return rd_smspec_has_field_var(rd_sum->smspec, var);
}

double rd_sum_get_field_var(const rd_sum_type *rd_sum, int time_index,
                            const char *var) {
    int params_index =
        rd_smspec_get_field_var_params_index(rd_sum->smspec, var);
    return rd_sum_data_iget(rd_sum->data, time_index, params_index);
}

double rd_sum_get_field_var_from_sim_time(const rd_sum_type *rd_sum,
                                          time_t sim_time, const char *var) {
    const rd::smspec_node &node =
        rd_smspec_get_field_var_node(rd_sum->smspec, var);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, &node);
}

double rd_sum_get_field_var_from_sim_days(const rd_sum_type *rd_sum,
                                          double sim_days, const char *var) {
    const rd::smspec_node &node =
        rd_smspec_get_field_var_node(rd_sum->smspec, var);
    return rd_sum_get_from_sim_days(rd_sum, sim_days, &node);
}

bool rd_sum_has_block_var(const rd_sum_type *rd_sum, const char *block_var,
                          int block_nr) {
    return rd_smspec_has_block_var(rd_sum->smspec, block_var, block_nr);
}

double rd_sum_get_block_var(const rd_sum_type *rd_sum, int time_index,
                            const char *block_var, int block_nr) {
    int params_index = rd_smspec_get_block_var_params_index(
        rd_sum->smspec, block_var, block_nr);
    return rd_sum_data_iget(rd_sum->data, time_index, params_index);
}

int rd_sum_get_block_var_index_ijk(const rd_sum_type *rd_sum,
                                   const char *block_var, int i, int j, int k) {
    return rd_smspec_get_block_var_params_index_ijk(rd_sum->smspec, block_var,
                                                    i, j, k);
}

bool rd_sum_has_block_var_ijk(const rd_sum_type *rd_sum, const char *block_var,
                              int i, int j, int k) {
    return rd_smspec_has_block_var_ijk(rd_sum->smspec, block_var, i, j, k);
}

double rd_sum_get_block_var_ijk(const rd_sum_type *rd_sum, int time_index,
                                const char *block_var, int i, int j, int k) {
    int index = rd_sum_get_block_var_index_ijk(rd_sum, block_var, i, j, k);
    return rd_sum_data_iget(rd_sum->data, time_index, index);
}

double rd_sum_get_block_var_ijk_from_sim_time(const rd_sum_type *rd_sum,
                                              time_t sim_time,
                                              const char *block_var, int i,
                                              int j, int k) {
    const rd::smspec_node &node =
        rd_smspec_get_block_var_node_ijk(rd_sum->smspec, block_var, i, j, k);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, &node);
}

double rd_sum_get_block_var_ijk_from_sim_days(const rd_sum_type *rd_sum,
                                              double sim_days,
                                              const char *block_var, int i,
                                              int j, int k) {
    const rd::smspec_node &node =
        rd_smspec_get_block_var_node_ijk(rd_sum->smspec, block_var, i, j, k);
    return rd_sum_get_from_sim_days(rd_sum, sim_days, &node);
}

/**
   region_nr: [1...num_regions] (NOT C-based indexing)
*/

bool rd_sum_has_region_var(const rd_sum_type *rd_sum, const char *var,
                           int region_nr) {
    return rd_smspec_has_region_var(rd_sum->smspec, var, region_nr);
}

double rd_sum_get_region_var(const rd_sum_type *rd_sum, int time_index,
                             const char *var, int region_nr) {
    int params_index =
        rd_smspec_get_region_var_params_index(rd_sum->smspec, var, region_nr);
    return rd_sum_data_iget(rd_sum->data, time_index, params_index);
}

double rd_sum_get_region_var_from_sim_time(const rd_sum_type *rd_sum,
                                           time_t sim_time, const char *var,
                                           int region_nr) {
    const rd::smspec_node &node =
        rd_smspec_get_region_var_node(rd_sum->smspec, var, region_nr);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, &node);
}

double rd_sum_get_region_var_from_sim_days(const rd_sum_type *rd_sum,
                                           double sim_days, const char *var,
                                           int region_nr) {
    const rd::smspec_node &node =
        rd_smspec_get_region_var_node(rd_sum->smspec, var, region_nr);
    return rd_sum_get_from_sim_days(rd_sum, sim_days, &node);
}

int rd_sum_get_misc_var_index(const rd_sum_type *rd_sum, const char *var) {
    return rd_smspec_get_misc_var_params_index(rd_sum->smspec, var);
}

bool rd_sum_has_misc_var(const rd_sum_type *rd_sum, const char *var) {
    return rd_smspec_has_misc_var(rd_sum->smspec, var);
}

double rd_sum_get_misc_var(const rd_sum_type *rd_sum, int time_index,
                           const char *var) {
    int index = rd_sum_get_misc_var_index(rd_sum, var);
    return rd_sum_data_iget(rd_sum->data, time_index, index);
}

int rd_sum_get_well_completion_var_index(const rd_sum_type *rd_sum,
                                         const char *well, const char *var,
                                         int cell_nr) {
    return rd_smspec_get_well_completion_var_params_index(rd_sum->smspec, well,
                                                          var, cell_nr);
}

bool rd_sum_has_well_completion_var(const rd_sum_type *rd_sum, const char *well,
                                    const char *var, int cell_nr) {
    return rd_smspec_has_well_completion_var(rd_sum->smspec, well, var,
                                             cell_nr);
}

double rd_sum_get_well_completion_var(const rd_sum_type *rd_sum, int time_index,
                                      const char *well, const char *var,
                                      int cell_nr) {
    int index =
        rd_sum_get_well_completion_var_index(rd_sum, well, var, cell_nr);
    return rd_sum_data_iget(rd_sum->data, time_index, index);
}

const rd::smspec_node *rd_sum_get_general_var_node(const rd_sum_type *rd_sum,
                                                   const char *lookup_kw) {
    const rd::smspec_node &node =
        rd_smspec_get_general_var_node(rd_sum->smspec, lookup_kw);
    return &node;
}

int rd_sum_get_general_var_params_index(const rd_sum_type *rd_sum,
                                        const char *lookup_kw) {
    return rd_smspec_get_general_var_params_index(rd_sum->smspec, lookup_kw);
}

bool rd_sum_has_general_var(const rd_sum_type *rd_sum, const char *lookup_kw) {
    return rd_smspec_has_general_var(rd_sum->smspec, lookup_kw);
}

bool rd_sum_has_key(const rd_sum_type *rd_sum, const char *lookup_kw) {
    return rd_sum_has_general_var(rd_sum, lookup_kw);
}

double rd_sum_get_general_var(const rd_sum_type *rd_sum, int time_index,
                              const char *lookup_kw) {
    int params_index = rd_sum_get_general_var_params_index(rd_sum, lookup_kw);
    return rd_sum_data_iget(rd_sum->data, time_index, params_index);
}

#ifdef __cplusplus
extern "C" {

void rd_sum_get_interp_vector(const rd_sum_type *rd_sum, time_t sim_time,
                              const rd_sum_vector_type *key_words,
                              double_vector_type *data) {
    rd_sum_data_get_interp_vector(rd_sum->data, sim_time, key_words, data);
}

void rd_sum_fwrite_interp_csv_line(const rd_sum_type *rd_sum, time_t sim_time,
                                   const rd_sum_vector_type *key_words,
                                   FILE *fp) {
    rd_sum_data_fwrite_interp_csv_line(rd_sum->data, sim_time, key_words, fp);
}
}
#endif

double rd_sum_get_general_var_from_sim_time(const rd_sum_type *rd_sum,
                                            time_t sim_time, const char *var) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, var);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, node);
}

double rd_sum_get_general_var_from_sim_days(const rd_sum_type *rd_sum,
                                            double sim_days, const char *var) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, var);
    return rd_sum_data_get_from_sim_days(rd_sum->data, sim_days, *node);
}

const char *rd_sum_get_general_var_unit(const rd_sum_type *rd_sum,
                                        const char *var) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, var);
    return smspec_node_get_unit(node);
}

rd_sum_type *rd_sum_alloc_resample(const rd_sum_type *rd_sum,
                                   const char *rd_case,
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
        return NULL;
    if (!upper_extrapolation && input_end > end_time)
        return NULL;
    if (!time_t_vector_is_sorted(times, false))
        return NULL;

    const int *grid_dims = rd_smspec_get_grid_dims(rd_sum->smspec);

    bool time_in_days = false;
    const rd::smspec_node &node =
        rd_smspec_iget_node_w_node_index(rd_sum->smspec, 0);
    if (util_string_equal(smspec_node_get_unit(&node), "DAYS"))
        time_in_days = true;

    //create elc_sum_resampled with TIME node only
    rd_sum_type *rd_sum_resampled = rd_sum_alloc_writer(
        rd_case, rd_sum->fmt_case, rd_sum->unified, rd_sum->key_join_string,
        input_start, time_in_days, grid_dims[0], grid_dims[1], grid_dims[2]);

    //add remaining nodes
    for (int i = 0; i < rd_smspec_num_nodes(rd_sum->smspec); i++) {
        const rd::smspec_node &node =
            rd_smspec_iget_node_w_node_index(rd_sum->smspec, i);
        if (util_string_equal(smspec_node_get_gen_key1(&node), "TIME"))
            continue;

        rd_sum_add_smspec_node(rd_sum_resampled, &node);
    }

    /*
    The SMSPEC header structure has been completely initialized, it is time to
    start filling it up with data.

  */
    rd_sum_vector_type *rd_sum_vector = rd_sum_vector_alloc(rd_sum, true);
    double_vector_type *data =
        double_vector_alloc(rd_sum_vector_get_size(rd_sum_vector), 0);

    for (int report_step = 0; report_step < time_t_vector_size(times);
         report_step++) {
        time_t input_t = time_t_vector_iget(times, report_step);
        if (input_t < start_time) {
            //clamping to the first value for t < start_time or if it is a rate than derivative is 0
            for (int i = 1; i < rd_smspec_num_nodes(rd_sum->smspec); i++) {
                double value = 0;
                const rd::smspec_node &node =
                    rd_smspec_iget_node_w_node_index(rd_sum->smspec, i);
                if (!node.is_rate())
                    value = rd_sum_iget_first_value(rd_sum,
                                                    node.get_params_index());
                double_vector_iset(data, i - 1, value);
            }
        } else if (input_t > end_time) {
            //clamping to the last value for t > end_time or if it is a rate than derivative is 0
            for (int i = 1; i < rd_smspec_num_nodes(rd_sum->smspec); i++) {
                double value = 0;
                const rd::smspec_node &node =
                    rd_smspec_iget_node_w_node_index(rd_sum->smspec, i);
                if (!node.is_rate())
                    value =
                        rd_sum_iget_last_value(rd_sum, node.get_params_index());
                double_vector_iset(data, i - 1, value);
            }
        } else {
            /* Look up interpolated data in the original case. */
            rd_sum_get_interp_vector(rd_sum, input_t, rd_sum_vector, data);
        }

        /* Add timestep corresponding to the interpolated data in the resampled case. */
        rd_sum_tstep_type *tstep = rd_sum_add_tstep(
            rd_sum_resampled, report_step, input_t - input_start);
        for (int data_index = 0;
             data_index < rd_sum_vector_get_size(rd_sum_vector); data_index++) {
            double value = double_vector_iget(data, data_index);
            int params_index =
                data_index +
                1; // The +1 shift is because the first element in the tstep is time value.
            rd_sum_tstep_iset(tstep, params_index, value);
        }
    }
    double_vector_free(data);
    rd_sum_vector_free(rd_sum_vector);
    return rd_sum_resampled;
}

double rd_sum_iget(const rd_sum_type *rd_sum, int time_index, int param_index) {
    return rd_sum_data_iget(rd_sum->data, time_index, param_index);
}

bool rd_sum_var_is_rate(const rd_sum_type *rd_sum, const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_is_rate(node);
}

bool rd_sum_var_is_total(const rd_sum_type *rd_sum, const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_is_total(node);
}

rd_smspec_var_type rd_sum_identify_var_type(const char *var) {
    return rd_smspec_identify_var_type(var);
}

rd_smspec_var_type rd_sum_get_var_type(const rd_sum_type *rd_sum,
                                       const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_get_var_type(node);
}

const char *rd_sum_get_unit(const rd_sum_type *rd_sum, const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_get_unit(node);
}

int rd_sum_get_num(const rd_sum_type *rd_sum, const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_get_num(node);
}

const char *rd_sum_get_wgname(const rd_sum_type *rd_sum, const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_get_wgname(node);
}

const char *rd_sum_get_keyword(const rd_sum_type *rd_sum, const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return smspec_node_get_keyword(node);
}

bool rd_sum_has_report_step(const rd_sum_type *rd_sum, int report_step) {
    return rd_sum_data_has_report_step(rd_sum->data, report_step);
}

int rd_sum_get_last_report_step(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_last_report_step(rd_sum->data);
}

int rd_sum_get_first_report_step(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_first_report_step(rd_sum->data);
}

int rd_sum_iget_report_end(const rd_sum_type *rd_sum, int report_step) {
    return rd_sum_data_iget_report_end(rd_sum->data, report_step);
}

int rd_sum_iget_report_step(const rd_sum_type *rd_sum, int internal_index) {
    return rd_sum_data_iget_report_step(rd_sum->data, internal_index);
}

time_t_vector_type *rd_sum_alloc_time_vector(const rd_sum_type *rd_sum,
                                             bool report_only) {
    return rd_sum_data_alloc_time_vector(rd_sum->data, report_only);
}

void rd_sum_init_double_vector__(const rd_sum_type *rd_sum, int params_index,
                                 double *data) {
    rd_sum_data_init_double_vector(rd_sum->data, params_index, data);
}

void rd_sum_init_double_vector(const rd_sum_type *rd_sum, const char *gen_key,
                               double *data) {
    int params_index = rd_sum_get_general_var_params_index(rd_sum, gen_key);
    rd_sum_init_double_vector__(rd_sum, params_index, data);
}

void rd_sum_init_double_vector_interp(const rd_sum_type *rd_sum,
                                      const char *gen_key,
                                      const time_t_vector_type *time_points,
                                      double *data) {
    const rd::smspec_node &node =
        rd_smspec_get_general_var_node(rd_sum->smspec, gen_key);
    rd_sum_data_init_double_vector_interp(rd_sum->data, node, time_points,
                                          data);
}

void rd_sum_init_datetime64_vector(const rd_sum_type *rd_sum, int64_t *data,
                                   int multiplier) {
    rd_sum_data_init_datetime64_vector(rd_sum->data, data, multiplier);
}

void rd_sum_init_double_frame(const rd_sum_type *rd_sum,
                              const rd_sum_vector_type *keywords,
                              double *data) {
    rd_sum_data_init_double_frame(rd_sum->data, keywords, data);
}

void rd_sum_init_double_frame_interp(const rd_sum_type *rd_sum,
                                     const rd_sum_vector_type *keywords,
                                     const time_t_vector_type *time_points,
                                     double *data) {
    rd_sum_data_init_double_frame_interp(rd_sum->data, keywords, time_points,
                                         data);
}

double_vector_type *rd_sum_alloc_data_vector(const rd_sum_type *rd_sum,
                                             int data_index, bool report_only) {
    return rd_sum_data_alloc_data_vector(rd_sum->data, data_index, report_only);
}

void rd_sum_summarize(const rd_sum_type *rd_sum, FILE *stream) {
    rd_sum_data_summarize(rd_sum->data, stream);
}

/**
   Returns the first internal index where a limiting value is
   reached. If the limiting value is never reached, -1 is
   returned. The smspec_index should be calculated first with one of
   the

      rd_sum_get_XXXX_index()

   functions. I.e. the following code will give the first index where
   the water wut in well PX exceeds 0.25:

   {
      int smspec_index   = rd_sum_get_well_var( rd_sum , "PX" , "WWCT" );
      int first_index    = rd_sum_get_first_gt( rd_sum , smspec_index , 0.25);
   }

*/

static int rd_sum_get_limiting(const rd_sum_type *rd_sum, int smspec_index,
                               double limit, bool gt) {
    const int length = rd_sum_data_get_length(rd_sum->data);
    int internal_index = 0;
    do {
        double value =
            rd_sum_data_iget(rd_sum->data, internal_index, smspec_index);
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
    return rd_sum_data_get_report_time(rd_sum->data, report_step);
}

time_t rd_sum_iget_sim_time(const rd_sum_type *rd_sum, int index) {
    return rd_sum_data_iget_sim_time(rd_sum->data, index);
}

time_t rd_sum_get_data_start(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_data_start(rd_sum->data);
}

time_t rd_sum_get_start_time(const rd_sum_type *rd_sum) {
    return rd_smspec_get_start_time(rd_sum->smspec);
}

time_t rd_sum_get_end_time(const rd_sum_type *rd_sum) {
    try {
        return rd_sum_data_get_sim_end(rd_sum->data);
    } catch (std::out_of_range) {
        return rd_smspec_get_start_time(rd_sum->smspec);
    }
}

double rd_sum_iget_sim_days(const rd_sum_type *rd_sum, int index) {
    return rd_sum_data_iget_sim_days(rd_sum->data, index);
}

void rd_sum_fmt_init_summary_x(const rd_sum_type *rd_sum,
                               rd_sum_fmt_type *fmt) {
    fmt->locale = NULL;
    fmt->sep = "";
    fmt->date_fmt = "%d/%m/%Y   ";
    fmt->value_fmt = " %15.6g ";

    if (util_string_equal(rd_sum_get_unit(rd_sum, "TIME"), "DAYS"))
        fmt->days_fmt = "%7.2f   ";
    else
        fmt->days_fmt = "%7.4f   ";

    fmt->header_fmt = " %15s ";

    fmt->newline = "\n";
    fmt->print_header = true;
    fmt->print_dash = true;
    fmt->date_dash = "-----------------------";
    fmt->value_dash = "-----------------";
    fmt->date_header = "-- Days   dd/mm/yyyy   ";
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
                                  char *date_string,
                                  const rd_sum_fmt_type *fmt) {
    fprintf(stream, fmt->days_fmt,
            rd_sum_iget_sim_days(rd_sum, internal_index));
    fprintf(stream, "%s", fmt->sep);

    {
        struct tm ts;
        time_t sim_time = rd_sum_iget_sim_time(rd_sum, internal_index);
        util_time_utc(&sim_time, &ts);
        strftime(date_string, DATE_STRING_LENGTH - 1, fmt->date_fmt, &ts);
        fprintf(stream, "%s", date_string);
    }

    {
        int ivar;
        for (ivar = 0; ivar < int_vector_size(var_index); ivar++) {
            if (bool_vector_iget(has_var, ivar)) {
                fprintf(stream, "%s", fmt->sep);
                fprintf(stream, fmt->value_fmt,
                        rd_sum_iget(rd_sum, internal_index,
                                    int_vector_iget(var_index, ivar)));
            }
        }
    }

    fprintf(stream, "%s", fmt->newline);
}

static void rd_sum_fprintf_header(const rd_sum_type *rd_sum,
                                  const stringlist_type *key_list,
                                  const bool_vector_type *has_var, FILE *stream,
                                  const rd_sum_fmt_type *fmt) {
    fprintf(stream, "%s", fmt->date_header);
    {
        int i;
        for (i = 0; i < stringlist_get_size(key_list); i++)
            if (bool_vector_iget(has_var, i)) {
                fprintf(stream, "%s", fmt->sep);
                fprintf(stream, fmt->header_fmt, stringlist_iget(key_list, i));
            }
    }

    fprintf(stream, "%s", fmt->newline);
    if (fmt->print_dash) {
        fprintf(stream, "%s", fmt->date_dash);

        {
            int i;
            for (i = 0; i < stringlist_get_size(key_list); i++)
                if (bool_vector_iget(has_var, i))
                    fprintf(stream, "%s", fmt->value_dash);
        }
        fprintf(stream, "%s", fmt->newline);
    }
}

void rd_sum_fprintf(const rd_sum_type *rd_sum, FILE *stream,
                    const stringlist_type *var_list, bool report_only,
                    const rd_sum_fmt_type *fmt) {
    bool_vector_type *has_var =
        bool_vector_alloc(stringlist_get_size(var_list), false);
    int_vector_type *var_index =
        int_vector_alloc(stringlist_get_size(var_list), -1);
    char *date_string =
        (char *)util_malloc(DATE_STRING_LENGTH * sizeof *date_string);

    char *current_locale = NULL;
    if (fmt->locale != NULL)
        current_locale = setlocale(LC_NUMERIC, fmt->locale);

    {
        int ivar;
        for (ivar = 0; ivar < stringlist_get_size(var_list); ivar++) {
            if (rd_sum_has_general_var(rd_sum,
                                       stringlist_iget(var_list, ivar))) {
                bool_vector_iset(has_var, ivar, true);
                int_vector_iset(var_index, ivar,
                                rd_sum_get_general_var_params_index(
                                    rd_sum, stringlist_iget(var_list, ivar)));
            } else {
                fprintf(stderr,
                        "** Warning: could not find variable: \'%s\' in "
                        "summary file \n",
                        stringlist_iget(var_list, ivar));
                bool_vector_iset(has_var, ivar, false);
            }
        }
    }

    if (fmt->print_header)
        rd_sum_fprintf_header(rd_sum, var_list, has_var, stream, fmt);

    if (report_only) {
        int first_report = rd_sum_get_first_report_step(rd_sum);
        int last_report = rd_sum_get_last_report_step(rd_sum);
        int report;

        for (report = first_report; report <= last_report; report++) {
            if (rd_sum_data_has_report_step(rd_sum->data, report)) {
                int time_index;
                time_index = rd_sum_data_iget_report_end(rd_sum->data, report);
                __rd_sum_fprintf_line(rd_sum, stream, time_index, has_var,
                                      var_index, date_string, fmt);
            }
        }
    } else {
        int time_index;
        for (time_index = 0; time_index < rd_sum_get_data_length(rd_sum);
             time_index++)
            __rd_sum_fprintf_line(rd_sum, stream, time_index, has_var,
                                  var_index, date_string, fmt);
    }

    int_vector_free(var_index);
    bool_vector_free(has_var);
    if (current_locale != NULL)
        setlocale(LC_NUMERIC, current_locale);
    free(date_string);
}
#undef DATE_STRING_LENGTH

static void rd_sum_fmt_init_csv(rd_sum_fmt_type *fmt, const char *date_format,
                                const char *date_header, const char *sep) {
    fmt->locale = NULL; //"Norwegian";
    fmt->sep = sep;
    fmt->date_fmt = date_format;
    fmt->value_fmt = "%g";
    fmt->days_fmt = "%7.2f";
    fmt->header_fmt = "%s";

    fmt->newline = "\r\n";
    fmt->date_header = date_header;
    fmt->print_header = true;
    fmt->print_dash = false;
}

void rd_sum_export_csv(const rd_sum_type *rd_sum, const char *filename,
                       const stringlist_type *var_list, const char *date_format,
                       const char *sep) {
    FILE *stream = util_mkdir_fopen(filename, "w");
    char *date_header = util_alloc_sprintf("DAYS%sDATE", sep);
    bool report_only = false;
    rd_sum_fmt_type fmt;
    rd_sum_fmt_init_csv(&fmt, date_format, date_header, sep);
    rd_sum_fprintf(rd_sum, stream, var_list, report_only, &fmt);
    fclose(stream);
    free(date_header);
}

const rd_sum_type *rd_sum_get_restart_case(const rd_sum_type *rd_sum) {
    return rd_sum->restart_case;
}

int rd_sum_get_restart_step(const rd_sum_type *rd_sum) {
    return rd_smspec_get_restart_step(rd_sum->smspec);
}

const char *rd_sum_get_case(const rd_sum_type *rd_sum) {
    return rd_sum->rd_case;
}

const char *rd_sum_get_path(const rd_sum_type *rd_sum) { return rd_sum->path; }

const char *rd_sum_get_abs_path(const rd_sum_type *rd_sum) {
    return rd_sum->abs_path;
}

const char *rd_sum_get_base(const rd_sum_type *rd_sum) { return rd_sum->base; }

/**
   This function will check if the currently loaded case corresponds
   to the case specified by @input_file. The extension of @input file
   can be arbitrary (or nonexistent) and will be ignored (this can
   lead to errors with formatted/unformatted mixup if the simulation
   directory has been changed after the rd_sum instance has been
   loaded).
*/

bool rd_sum_same_case(const rd_sum_type *rd_sum, const char *input_file) {
    bool same_case = false;
    {
        char *path;
        char *base;

        util_alloc_file_components(input_file, &path, &base, NULL);
        {
            bool fmt_file = rd_smspec_get_formatted(rd_sum->smspec);
            char *header_file = rd_alloc_exfilename(
                path, base, RD_SUMMARY_HEADER_FILE, fmt_file, -1);
            if (header_file != NULL) {
                same_case = util_same_file(
                    header_file, rd_smspec_get_header_file(rd_sum->smspec));
                free(header_file);
            }
        }

        free(path);
        free(base);
    }
    return same_case;
}

stringlist_type *
rd_sum_alloc_matching_general_var_list(const rd_sum_type *rd_sum,
                                       const char *pattern) {
    return rd_smspec_alloc_matching_general_var_list(rd_sum->smspec, pattern);
}

void rd_sum_select_matching_general_var_list(const rd_sum_type *rd_sum,
                                             const char *pattern,
                                             stringlist_type *keys) {
    rd_smspec_select_matching_general_var_list(rd_sum->smspec, pattern, keys);
}

stringlist_type *rd_sum_alloc_well_list(const rd_sum_type *rd_sum,
                                        const char *pattern) {
    return rd_smspec_alloc_well_list(rd_sum->smspec, pattern);
}

stringlist_type *rd_sum_alloc_group_list(const rd_sum_type *rd_sum,
                                         const char *pattern) {
    return rd_smspec_alloc_group_list(rd_sum->smspec, pattern);
}

stringlist_type *rd_sum_alloc_well_var_list(const rd_sum_type *rd_sum) {
    return rd_smspec_alloc_well_var_list(rd_sum->smspec);
}

void rd_sum_resample_from_sim_time(const rd_sum_type *rd_sum,
                                   const time_t_vector_type *sim_time,
                                   double_vector_type *value,
                                   const char *gen_key) {
    const rd::smspec_node &node =
        rd_smspec_get_general_var_node(rd_sum->smspec, gen_key);
    double_vector_reset(value);
    {
        int i;
        for (i = 0; i < time_t_vector_size(sim_time); i++)
            double_vector_iset(
                value, i,
                rd_sum_data_get_from_sim_time(
                    rd_sum->data, time_t_vector_iget(sim_time, i), node));
    }
}

void rd_sum_resample_from_sim_days(const rd_sum_type *rd_sum,
                                   const double_vector_type *sim_days,
                                   double_vector_type *value,
                                   const char *gen_key) {
    const rd::smspec_node &node =
        rd_smspec_get_general_var_node(rd_sum->smspec, gen_key);
    double_vector_reset(value);
    {
        int i;
        for (i = 0; i < double_vector_size(sim_days); i++)
            double_vector_iset(
                value, i,
                rd_sum_data_get_from_sim_days(
                    rd_sum->data, double_vector_iget(sim_days, i), node));
    }
}

time_t rd_sum_time_from_days(const rd_sum_type *rd_sum, double sim_days) {
    time_t t = rd_smspec_get_start_time(rd_sum->smspec);
    util_inplace_forward_days_utc(&t, sim_days);
    return t;
}

double rd_sum_days_from_time(const rd_sum_type *rd_sum, time_t sim_time) {
    double seconds_diff =
        util_difftime(rd_smspec_get_start_time(rd_sum->smspec), sim_time, NULL,
                      NULL, NULL, NULL);
    return seconds_diff * 1.0 / (3600 * 24.0);
}

double rd_sum_get_first_day(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_first_day(rd_sum->data);
}

double rd_sum_get_sim_length(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_sim_length(rd_sum->data);
}

/**
   Will return the number of data blocks.
*/
int rd_sum_get_data_length(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_length(rd_sum->data);
}

bool rd_sum_check_sim_time(const rd_sum_type *sum, time_t sim_time) {
    return rd_sum_data_check_sim_time(sum->data, sim_time);
}

bool rd_sum_check_sim_days(const rd_sum_type *sum, double sim_days) {
    return rd_sum_data_check_sim_days(sum->data, sim_days);
}

int rd_sum_get_report_step_from_time(const rd_sum_type *sum, time_t sim_time) {
    return rd_sum_data_get_report_step_from_time(sum->data, sim_time);
}

int rd_sum_get_report_step_from_days(const rd_sum_type *sum, double sim_days) {
    return rd_sum_data_get_report_step_from_days(sum->data, sim_days);
}

rd_smspec_type *rd_sum_get_smspec(const rd_sum_type *rd_sum) {
    return rd_sum->smspec;
}

/*
   The functions below are extremly simple functions which only serve
   as an easy access to the smspec_alloc_xxx_key() functions which
   know how to create the various composite keys.
*/

char *rd_sum_alloc_well_key(const rd_sum_type *rd_sum, const char *keyword,
                            const char *wgname) {
    return rd_smspec_alloc_well_key(rd_sum->smspec, keyword, wgname);
}

bool rd_sum_is_oil_producer(const rd_sum_type *rd_sum, const char *well) {
    const char *WOPT_KEY = "WOPT";
    bool oil_producer = false;

    if (rd_sum_has_well_var(rd_sum, well, WOPT_KEY)) {
        int last_step = rd_sum_get_data_length(rd_sum) - 1;
        double wopt = rd_sum_get_well_var(rd_sum, last_step, well, WOPT_KEY);

        if (wopt > 0)
            oil_producer = true;
    }

    return oil_producer;
}

bool rd_sum_report_step_equal(const rd_sum_type *rd_sum1,
                              const rd_sum_type *rd_sum2) {
    if (rd_sum1 == rd_sum2)
        return true;
    else
        return rd_sum_data_report_step_equal(rd_sum1->data, rd_sum2->data);
}

bool rd_sum_report_step_compatible(const rd_sum_type *rd_sum1,
                                   const rd_sum_type *rd_sum2) {
    if (rd_sum1 == rd_sum2)
        return true;
    else
        return rd_sum_data_report_step_compatible(rd_sum1->data, rd_sum2->data);
}

double_vector_type *rd_sum_alloc_seconds_solution(const rd_sum_type *rd_sum,
                                                  const char *gen_key,
                                                  double cmp_value,
                                                  bool rates_clamp_lower) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return rd_sum_data_alloc_seconds_solution(rd_sum->data, *node, cmp_value,
                                              rates_clamp_lower);
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
    time_t_vector_type *solution = time_t_vector_alloc(0, 0);
    {
        double_vector_type *seconds = rd_sum_alloc_seconds_solution(
            rd_sum, gen_key, cmp_value, rates_clamp_lower);
        time_t start_time = rd_sum_get_start_time(rd_sum);
        for (int i = 0; i < double_vector_size(seconds); i++) {
            time_t t = start_time;
            util_inplace_forward_seconds_utc(&t,
                                             double_vector_iget(seconds, i));
            time_t_vector_append(solution, t);
        }
        double_vector_free(seconds);
    }
    return solution;
}

ert_rd_unit_enum rd_sum_get_unit_system(const rd_sum_type *rd_sum) {
    return rd_smspec_get_unit_system(rd_sum->smspec);
}

double rd_sum_iget_last_value(const rd_sum_type *rd_sum, int param_index) {
    return rd_sum_data_iget_last_value(rd_sum->data, param_index);
}

double rd_sum_get_last_value_gen_key(const rd_sum_type *rd_sum,
                                     const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return rd_sum_iget_last_value(rd_sum, smspec_node_get_params_index(node));
}

double rd_sum_get_last_value_node(const rd_sum_type *rd_sum,
                                  const rd::smspec_node *node) {
    return rd_sum_iget_last_value(rd_sum, smspec_node_get_params_index(node));
}

double rd_sum_iget_first_value(const rd_sum_type *rd_sum, int param_index) {
    return rd_sum_data_iget_first_value(rd_sum->data, param_index);
}

double rd_sum_get_first_value_gen_key(const rd_sum_type *rd_sum,
                                      const char *gen_key) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, gen_key);
    return rd_sum_iget_first_value(rd_sum, smspec_node_get_params_index(node));
}

double rd_sum_get_first_value_node(const rd_sum_type *rd_sum,
                                   const rd::smspec_node *node) {
    return rd_sum_iget_first_value(rd_sum, smspec_node_get_params_index(node));
}
