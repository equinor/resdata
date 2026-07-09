#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

#include <exception>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>
#include <memory>

#include <ert/util/int_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_util.hpp>

struct rd_file_view_struct {
    std::vector<std::shared_ptr<FileKW>> kw_list;
    std::map<std::string, std::vector<int>> kw_index;
    std::vector<std::string>
        distinct_kw; /* A list of the keywords occuring in the file - each string occurs ONLY ONCE. */
    ERT::FortIO *
        fortio; /* The same fortio instance pointer as in the rd_file styructure. */
    inv_map_type
        *inv_map; /* Shared reference owned by the rd_file structure. */
    std::vector<std::unique_ptr<rd_file_view_type>> child_list;
    int *flags;

    rd_file_view_struct(ERT::FortIO *fortio, int *flags, inv_map_type *inv_map)
        : fortio(fortio), inv_map(inv_map), flags(flags) {};
};

bool rd_file_view_check_flags(int state_flags, int query_flags) {
    return (state_flags & query_flags) == query_flags;
}

bool rd_file_view_flags_set(const rd_file_view_type *file_view,
                            int query_flags) {
    return rd_file_view_check_flags(*file_view->flags, query_flags);
}

const char *rd_file_view_get_src_file(const rd_file_view_type *file_view) {
    return file_view->fortio->filename_ref();
}

rd_file_view_type *rd_file_view_alloc(ERT::FortIO *fortio, int *flags,
                                      inv_map_type *inv_map) {
    return new rd_file_view_struct(fortio, flags, inv_map);
}

static int rd_file_view_get_global_index(const rd_file_view_type *rd_file_view,
                                         const char *kw, int ith) {
    const auto &index_vector = rd_file_view->kw_index.at(kw);
    return index_vector[ith];
}

/**
   This function iterates over the kw_list vector and builds the
   internal index fields 'kw_index' and 'distinct_kw'. This function
   must be called every time the content of the kw_list vector is
   modified (otherwise the rd_file instance will be in an
   inconsistent state).
*/

void rd_file_view_make_index(rd_file_view_type *rd_file_view) {
    rd_file_view->distinct_kw.clear();
    rd_file_view->kw_index.clear();
    {
        int global_index = 0;
        for (const auto &file_kw : rd_file_view->kw_list) {
            const std::string &header = file_kw->get_header();
            if (rd_file_view->kw_index.find(header) ==
                rd_file_view->kw_index.end())
                rd_file_view->distinct_kw.push_back(header);

            auto &index_vector = rd_file_view->kw_index[header];
            index_vector.push_back(global_index);
            global_index++;
        }
    }
}

bool rd_file_view_has_kw(const rd_file_view_type *rd_file_view,
                         const char *kw) {
    return (rd_file_view->kw_index.find(kw) != rd_file_view->kw_index.end());
}

std::shared_ptr<FileKW>
rd_file_view_iget_file_kw(const rd_file_view_type *rd_file_view,
                          int global_index) {
    return rd_file_view->kw_list[global_index];
}

static std::shared_ptr<FileKW>
rd_file_view_iget_named_file_kw(const rd_file_view_type *rd_file_view,
                                const char *kw, int ith) {
    int global_index = rd_file_view_get_global_index(rd_file_view, kw, ith);
    return rd_file_view_iget_file_kw(rd_file_view, global_index);
}

bool rd_file_view_drop_flag(rd_file_view_type *file_view, int flag) {
    bool flag_set = rd_file_view_flags_set(file_view, flag);
    if (flag_set)
        *file_view->flags -= flag;

    return flag_set;
}

void rd_file_view_add_flag(rd_file_view_type *file_view, int flag) {
    *file_view->flags |= flag;
}

static rd_kw_type *rd_file_view_get_kw(const rd_file_view_type *rd_file_view,
                                       const std::shared_ptr<FileKW> &file_kw) {
    rd_kw_type *rd_kw = file_kw->get_kw_ptr();
    if (!rd_kw) {
        if (rd_file_view->fortio->assert_stream_open()) {

            rd_kw = file_kw->get_kw(*rd_file_view->fortio);
            (*rd_file_view->inv_map)[rd_kw] = file_kw.get();

            if (rd_file_view_flags_set(rd_file_view, RD_FILE_CLOSE_STREAM))
                rd_file_view->fortio->fclose_stream();
        }
    }
    return rd_kw;
}

rd_kw_type *rd_file_view_iget_kw(const rd_file_view_type *rd_file_view,
                                 int index) {
    auto file_kw = rd_file_view_iget_file_kw(rd_file_view, index);
    return rd_file_view_get_kw(rd_file_view, file_kw);
}

void rd_file_view_index_fload_kw(const rd_file_view_type *rd_file_view,
                                 const char *kw, int index,
                                 const int_vector_type *index_map,
                                 char *io_buffer) {
    auto file_kw = rd_file_view_iget_named_file_kw(rd_file_view, kw, index);

    if (!file_kw)
        throw std::invalid_argument(std::string("Keyword '") + kw + "' index " +
                                    std::to_string(index) +
                                    " not found in file view");

    if (rd_file_view->fortio->assert_stream_open()) {
        offset_type offset = file_kw->get_offset();
        rd_data_type data_type = file_kw->get_data_type();
        int element_count = file_kw->get_size();

        rd_kw_fread_indexed_data(*rd_file_view->fortio, offset, data_type,
                                 element_count, index_map, io_buffer);
    }
}

static int rd_file_view_find_kw_value(const rd_file_view_type *rd_file_view,
                                      const char *kw, const void *value) {
    int global_index = -1;
    if (rd_file_view_has_kw(rd_file_view, kw)) {
        const auto &index_list = rd_file_view->kw_index.at(kw);
        size_t index = 0;
        while (index < index_list.size()) {
            const rd_kw_type *rd_kw =
                rd_file_view_iget_kw(rd_file_view, index_list[index]);
            if (rd_kw_data_equal(rd_kw, value)) {
                global_index = index_list[index];
                break;
            }
            index++;
        }
    }
    return global_index;
}

const std::string &
rd_file_view_iget_distinct_kw(const rd_file_view_type *rd_file_view,
                              int index) {
    return rd_file_view->distinct_kw[index];
}

int rd_file_view_get_num_distinct_kw(const rd_file_view_type *rd_file_view) {
    return rd_file_view->distinct_kw.size();
}

int rd_file_view_get_size(const rd_file_view_type *rd_file_view) {
    return rd_file_view->kw_list.size();
}

rd_kw_type *rd_file_view_iget_named_kw(const rd_file_view_type *rd_file_view,
                                       const char *kw, int ith) {
    auto file_kw = rd_file_view_iget_named_file_kw(rd_file_view, kw, ith);
    return rd_file_view_get_kw(rd_file_view, file_kw);
}

bool rd_file_view_load_all(rd_file_view_type *rd_file_view) {
    bool loadOK = false;

    if (rd_file_view->fortio->assert_stream_open()) {
        for (auto &file_kw : rd_file_view->kw_list) {
            auto rd_kw = file_kw->get_kw(*rd_file_view->fortio);
            (*rd_file_view->inv_map)[rd_kw] = file_kw.get();
        }
        loadOK = true;
    }

    if (rd_file_view_flags_set(rd_file_view, RD_FILE_CLOSE_STREAM))
        rd_file_view->fortio->fclose_stream();

    return loadOK;
}

void rd_file_view_add_kw(rd_file_view_type *rd_file_view,
                         std::shared_ptr<FileKW> file_kw) {
    rd_file_view->kw_list.push_back(file_kw);
}

void rd_file_view_free(rd_file_view_type *rd_file_view) { delete rd_file_view; }

int rd_file_view_get_num_named_kw(const rd_file_view_type *rd_file_view,
                                  const char *kw) {
    if (rd_file_view_has_kw(rd_file_view, kw)) {
        const auto &index_vector = rd_file_view->kw_index.at(kw);
        return index_vector.size();
    } else
        return 0;
}

void rd_file_view_fwrite(const rd_file_view_type *rd_file_view,
                         ERT::FortIO &target, int offset) {
    for (size_t index = offset; index < rd_file_view->kw_list.size(); index++) {
        rd_kw_type *rd_kw = rd_file_view_iget_kw(rd_file_view, index);
        rd_kw_fwrite(rd_kw, target);
    }
}

static int rd_file_view_iget_occurence(const rd_file_view_type *rd_file_view,
                                       int global_index) {
    const auto &file_kw = rd_file_view->kw_list[global_index];
    const std::string &header = file_kw->get_header();
    const auto &index_vector = rd_file_view->kw_index.at(header);

    int occurence = -1;
    {
        /* Manual reverse lookup. */
        for (size_t i = 0; i < index_vector.size(); i++) {
            if (index_vector[i] == global_index)
                occurence = i;
        }
    }
    if (occurence < 0)
        util_abort("%s: internal error ... \n", __func__);

    return occurence;
}

rd_file_view_type *
rd_file_view_alloc_blockview2(const rd_file_view_type *rd_file_view,
                              const char *start_kw, const char *end_kw,
                              int occurence) {
    if ((start_kw != NULL) &&
        rd_file_view_get_num_named_kw(rd_file_view, start_kw) <= occurence)
        return NULL;

    rd_file_view_type *block_map = rd_file_view_alloc(
        rd_file_view->fortio, rd_file_view->flags, rd_file_view->inv_map);
    size_t kw_index = 0;
    if (start_kw)
        kw_index =
            rd_file_view_get_global_index(rd_file_view, start_kw, occurence);

    {
        auto file_kw = rd_file_view->kw_list[kw_index];
        while (true) {
            rd_file_view_add_kw(block_map, file_kw);

            kw_index++;
            if (kw_index == rd_file_view->kw_list.size())
                break;
            else {
                if (end_kw) {
                    file_kw = rd_file_view->kw_list[kw_index];
                    if (strcmp(end_kw, file_kw->get_header().c_str()) == 0)
                        break;
                }
            }
        }
    }
    rd_file_view_make_index(block_map);
    return block_map;
}

/**
   Will return NULL if the block which is asked for is not present.
*/
static rd_file_view_type *
rd_file_view_alloc_blockview(const rd_file_view_type *rd_file_view,
                             const char *header, int occurence) {
    return rd_file_view_alloc_blockview2(rd_file_view, header, header,
                                         occurence);
}

rd_file_view_type *rd_file_view_add_blockview(rd_file_view_type *file_view,
                                              const char *header,
                                              int occurence) {
    rd_file_view_type *child =
        rd_file_view_alloc_blockview2(file_view, header, header, occurence);

    if (child)
        file_view->child_list.emplace_back(child);

    return child;
}

rd_file_view_type *rd_file_view_add_blockview2(rd_file_view_type *rd_file_view,
                                               const char *start_kw,
                                               const char *end_kw,
                                               int occurence) {
    rd_file_view_type *child = rd_file_view_alloc_blockview2(
        rd_file_view, start_kw, end_kw, occurence);

    if (child)
        rd_file_view->child_list.emplace_back(child);

    return child;
}

/*
   There is no special datastructure for working with restart files,
   they are 100% stock rd_file instances with the following limited
   structure:

   * They are organized in blocks; where each block starts with a
     SEQNUM keyword, which contains the report step.

   * Each block contains an INTEHEAD keyword, immediately after the
     SEQNUM keyword, which contains the true simulation date of of the
     block, and also some other data. Observe that also INIT files and
     GRID files contain an INTEHEAD keyword; and that for files with
     LGRs there is one INTEHEAD keyword for each LGR. This creates an
     extra level of mess.

   The natural time ordering when working with the file data is just
   the running index in the file; however from a user perspective the
   natural way to indicate time coordinate is through the report step
   or the true simulation time (i.e. 22.th of October 2009). This file
   is all about converting the natural input unit time and report_step
   to the internal indexing. This is achieved by consulting the value
   of the INTEHEAD and SEQNUM keywords respectively.
*/

/*
About the time-direction
========================

For the following discussion we will focus on the following simplified
unified restart file. The leading number is the global index of the
keyword, the value in [] corresponds to the relevant part of the
content of the keyword on the line, the labels A,B,C,D,E are used for
references in the text below.

 0 | SEQNUM   [0]           \  A
 1 | INTEHEAD [01.01.2005]  |
 2 | PRESSURE [... ]        |
 3 | SWAT     [...]         |
   | -----------------------+
 4 | SEQNUM   [5]           |  B
 5 | INTEHEAD [01.06.2005]  |
 6 | PRESSURE [... ]        |
 7 | SWAT     [...]         |
   |------------------------+
 8 | SEQNUM   [10]          |  C
 9 | INTEHEAD [01.12.2006]  |
10 | PRESSURE [...]         |
11 | SWAT     [...]         |
   |------------------------+
12 | SEQNUM   [20]          |  D
13 | INTEHEAD [01.12.2007]  |
14 | PRESSURE [...]         |
15 | SWAT     [...]         |
16 | OIL_DEN  [...]         |
   |------------------------+
17 | SEQNUM   [40]          |  E
18 | INTEHEAD [01.12.2009]  |
19 | PRESSURE [...]         |
20 | SWAT     [...]         /


This restart file has the following features:

 o It contains in total 16 keywords.

 o It contains 5 blocks of collected keywords corresponding to one
   time instant, each of these blocks is called a report_step,
   typically coming from one DATES keyword in the ECLIPSE
   datafile. Observe that the file does not have the block structure
   visualized on this figure; the only thing separating the blocks in
   the file is the occurrence of a SEQNUM keyword.

 o Only a few of the report steps are present, namely 0, 5, 10, 20 and
   40.

 o The different blocks are not equally long; the fourth block has an
   extra keyword OIL_DEN.

To address these keywords and blocks using different time coordinates
we have introduced the following concepts:

 report_step: This corresponds to the value of the SEQNUM keyword;
    i.e. to perform queries based on the report_step we must load the
    SEQNUM keyword and read its value.

        rd_file_get_unrstmap_report_step( rd_file , 0 ) => A
        rd_file_get_unrstmap_report_step( rd_file , 1 ) => NULL

        rd_file_has_report_step( rd_file , 5 ) => True
        rd_file_has_report_step( rd_file , 2 ) => False

 sim_time: This corresponds to the true simulation time of the report
    step. The simulation time is stored as integers DAY, MONTH, YEAR
    in the INTEHEAD keyword; the function INTEHEAD_date() will extract
    the DAY, MONTH and YEAR values from an INTEHEAD keyword instance
    and convert to a time_t instance. The functions:

     rd_file_get_unrstmap_sim_time() and rd_file_has_has_sim_time()

    can be used to query for simulation times and get the
    corresponding block maps.

 index/global_index : This is typically the global running index of
    the keyword in the file; this is the unique address of the keyword
    which is used for the final lookup.

 occurrence: The nth time a particular keyword has occurred in the
    file, i.e. the SEQNUM keyword in block C is the third occurrence of
    SEQNUM. Instead of occurrence, xxxx_index is also used to indicate
    the occurrence of keyword xxxx. The occurrence number is the integer
    argument to the xxx_iget_named_kw() function, and also the final
    call to create blockmaps.

*/

bool rd_file_view_has_report_step(const rd_file_view_type *rd_file_view,
                                  int report_step) {
    int global_index =
        rd_file_view_find_kw_value(rd_file_view, SEQNUM_KW, &report_step);
    if (global_index >= 0)
        return true;
    else
        return false;
}

static time_t rsthead_date(int day, int month, int year) {
    return rd_make_date(day, month, year);
}

static time_t rd_rsthead_date(const rd_kw_type *intehead_kw) {
    return rsthead_date(rd_kw_iget_int(intehead_kw, INTEHEAD_DAY_INDEX),
                        rd_kw_iget_int(intehead_kw, INTEHEAD_MONTH_INDEX),
                        rd_kw_iget_int(intehead_kw, INTEHEAD_YEAR_INDEX));
}

time_t rd_file_view_iget_restart_sim_date(const rd_file_view_type *rd_file_view,
                                          int seqnum_index) {
    time_t sim_time = -1;
    std::unique_ptr<rd_file_view_type, decltype(&rd_file_view_free)> seqnum_map(
        rd_file_view_alloc_blockview(rd_file_view, SEQNUM_KW, seqnum_index),
        &rd_file_view_free);

    if (seqnum_map) {
        rd_kw_type *intehead_kw =
            rd_file_view_iget_named_kw(seqnum_map.get(), INTEHEAD_KW, 0);
        sim_time = rd_rsthead_date(intehead_kw);
    }

    return sim_time;
}

double rd_file_view_iget_restart_sim_days(const rd_file_view_type *rd_file_view,
                                          int seqnum_index) {
    double sim_days = 0;
    std::unique_ptr<rd_file_view_type, decltype(&rd_file_view_free)> seqnum_map(
        rd_file_view_alloc_blockview(rd_file_view, SEQNUM_KW, seqnum_index),
        &rd_file_view_free);

    if (seqnum_map) {
        rd_kw_type *doubhead_kw =
            rd_file_view_iget_named_kw(seqnum_map.get(), DOUBHEAD_KW, 0);
        sim_days = rd_kw_iget_double(doubhead_kw, DOUBHEAD_DAYS_INDEX);
    }

    return sim_days;
}

int rd_file_view_find_sim_time(const rd_file_view_type *rd_file_view,
                               time_t sim_time) {
    int seqnum_index = -1;
    if (rd_file_view_has_kw(rd_file_view, INTEHEAD_KW)) {
        const auto &intehead_index_list =
            rd_file_view->kw_index.at(INTEHEAD_KW);
        size_t index = 0;
        while (index < intehead_index_list.size()) {
            const rd_kw_type *intehead_kw =
                rd_file_view_iget_kw(rd_file_view, intehead_index_list[index]);
            if (rd_rsthead_date(intehead_kw) == sim_time) {
                seqnum_index = index;
                break;
            }
            index++;
        }
    }
    return seqnum_index;
}

/**
   This function will scan through the rd_file looking for INTEHEAD
   headers corresponding to sim_time. If sim_time is found the
   function will return the INTEHEAD occurence number, i.e. for a
   unified restart file like:

   INTEHEAD  /  01.01.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.03.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.05.2000
   ...
   PRESSURE
   SWAT
   ....

   The function call:

   rd_file_get_restart_index( restart_file , (time_t) "01.03.2000")

   will return 1. Observe that this will in general NOT agree with the
   DATES step number.

   If the sim_time can not be found the function will return -1, that
   includes the case when the file in question is not a restart file
   at all, and no INTEHEAD headers can be found.

   Observe that the function requires on-the-second-equality; which is
   of course quite strict.

   Each report step only has one occurence of SEQNUM, but one INTEHEAD
   for each LGR; i.e. one should call iselect_rstblock() prior to
   calling this function.
*/

bool rd_file_view_has_sim_time(const rd_file_view_type *rd_file_view,
                               time_t sim_time) {
    int num_INTEHEAD = rd_file_view_get_num_named_kw(rd_file_view, INTEHEAD_KW);
    if (num_INTEHEAD == 0)
        return false; /* We have no INTEHEAD headers - probably not a restart file at all. */
    else {
        int intehead_index = 0;
        while (true) {
            time_t itime = rd_file_view_iget_restart_sim_date(rd_file_view,
                                                              intehead_index);

            if (itime == sim_time) /* Perfect hit. */
                return true;

            if (itime >
                sim_time) /* We have gone past the target_time - i.e. we do not have it. */
                return false;

            intehead_index++;
            if (intehead_index ==
                num_INTEHEAD) /* We have iterated through the whole thing without finding sim_time. */
                return false;
        }
    }
}

static bool rd_file_view_has_sim_days(const rd_file_view_type *rd_file_view,
                                      double sim_days) {
    int num_DOUBHEAD = rd_file_view_get_num_named_kw(rd_file_view, DOUBHEAD_KW);
    if (num_DOUBHEAD == 0)
        return false; /* We have no DOUBHEAD headers - probably not a restart file at all. */
    else {
        int doubhead_index = 0;
        while (true) {
            double file_sim_days = rd_file_view_iget_restart_sim_days(
                rd_file_view, doubhead_index);

            if (util_double_approx_equal(sim_days,
                                         file_sim_days)) /* Perfect hit. */
                return true;

            if (file_sim_days >
                sim_days) /* We have gone past the target_time - i.e. we do not have it. */
                return false;

            doubhead_index++;
            if (doubhead_index ==
                num_DOUBHEAD) /* We have iterated through the whole thing without finding sim_time. */
                return false;
        }
    }
}

static int
rd_file_view_seqnum_index_from_sim_time(rd_file_view_type *parent_map,
                                        time_t sim_time) {
    int num_seqnum = rd_file_view_get_num_named_kw(parent_map, SEQNUM_KW);

    for (int s_idx = 0; s_idx < num_seqnum; s_idx++) {
        std::unique_ptr<rd_file_view_type, decltype(&rd_file_view_free)>
            seqnum_map(
                rd_file_view_alloc_blockview(parent_map, SEQNUM_KW, s_idx),
                &rd_file_view_free);

        if (!seqnum_map)
            continue;

        bool sim = rd_file_view_has_sim_time(seqnum_map.get(), sim_time);
        if (sim)
            return s_idx;
    }
    return -1;
}

static int rd_file_view_seqnum_index_from_sim_days(rd_file_view_type *file_view,
                                                   double sim_days) {
    int num_seqnum = rd_file_view_get_num_named_kw(file_view, SEQNUM_KW);
    int seqnum_index = 0;

    while (true) {
        std::unique_ptr<rd_file_view_type, decltype(&rd_file_view_free)>
            seqnum_map(rd_file_view_alloc_blockview(file_view, SEQNUM_KW,
                                                    seqnum_index),
                       &rd_file_view_free);

        if (seqnum_map) {
            if (rd_file_view_has_sim_days(seqnum_map.get(), sim_days)) {
                return seqnum_index;
            } else {
                seqnum_index++;
            }
        }

        if (num_seqnum == seqnum_index)
            return -1;
    }
}

/*
  Will mulitplex on the four input arguments.
*/
rd_file_view_type *rd_file_view_add_restart_view(rd_file_view_type *file_view,
                                                 int input_index,
                                                 int report_step,
                                                 time_t sim_time,
                                                 double sim_days) {
    rd_file_view_type *child = NULL;
    int seqnum_index = -1;

    if (input_index >= 0)
        seqnum_index = input_index;
    else if (report_step >= 0) {
        int global_index =
            rd_file_view_find_kw_value(file_view, SEQNUM_KW, &report_step);
        if (global_index >= 0)
            seqnum_index = rd_file_view_iget_occurence(file_view, global_index);
    } else if (sim_time != -1)
        seqnum_index =
            rd_file_view_seqnum_index_from_sim_time(file_view, sim_time);
    else if (sim_days >= 0)
        seqnum_index =
            rd_file_view_seqnum_index_from_sim_days(file_view, sim_days);

    if (seqnum_index >= 0)
        child = rd_file_view_add_blockview(file_view, SEQNUM_KW, seqnum_index);

    return child;
}

rd_file_view_type *rd_file_view_add_summary_view(rd_file_view_type *file_view,
                                                 int report_step) {
    rd_file_view_type *child =
        rd_file_view_add_blockview(file_view, SEQHDR_KW, report_step);
    return child;
}

void rd_file_view_fclose_stream(rd_file_view_type *file_view) {
    file_view->fortio->fclose_stream();
}

void rd_file_view_write_index(const rd_file_view_type *file_view,
                              FILE *ostream) {
    int size = rd_file_view_get_size(file_view);
    util_fwrite_int(size, ostream);

    for (const auto &file_kw : file_view->kw_list) {
        file_kw->write_header(ostream);
    }
}

rd_file_view_type *rd_file_view_fread_alloc(ERT::FortIO *fortio, int *flags,
                                            inv_map_type *inv_map,
                                            FILE *istream) {

    int index_size = util_fread_int(istream);
    if (index_size < 0)
        return NULL;
    rd_file_view_ptr file_view(rd_file_view_alloc(fortio, flags, inv_map),
                               &rd_file_view_free);

    try {
        file_view->kw_list =
            FileKW::read(istream, static_cast<size_t>(index_size));
    } catch (const std::exception &e) {
        fprintf(stderr, "%s\n", e.what());
        return NULL;
    }

    rd_file_view_make_index(file_view.get());
    return file_view.release();
}

void rd_file_view_clear(rd_file_view_type *file_view) {
    for (const auto &file_kw : file_view->kw_list) {
        if (auto *rd_kw = file_kw->get_kw_ptr())
            file_view->inv_map->erase(rd_kw);
        file_kw->clear();
    }
}
