#include <vector>
#include <string>
#include <map>
#include <cstring>

#include <resdata/fortio.h>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_type.hpp>

struct rd_file_view_struct {
    std::vector<rd_file_kw_type *> kw_list;
    std::map<std::string, std::vector<int>> kw_index;
    std::vector<std::string>
        distinct_kw; /* A list of the keywords occuring in the file - each string occurs ONLY ONCE. */
    fortio_type *
        fortio; /* The same fortio instance pointer as in the rd_file styructure. */
    bool
        owner; /* Is this map the owner of the rd_file_kw instances; only true for the global_map. */
    inv_map_type
        *inv_map; /* Shared reference owned by the rd_file structure. */
    std::vector<rd_file_view_type *> child_list;
    int *flags;
};

struct rd_file_transaction_struct {
    const rd_file_view_type *file_view;
    int *ref_count;
};

bool rd_file_view_check_flags(int state_flags, int query_flags) {
    if ((state_flags & query_flags) == query_flags)
        return true;
    else
        return false;
}

bool rd_file_view_flags_set(const rd_file_view_type *file_view,
                            int query_flags) {
    return rd_file_view_check_flags(*file_view->flags, query_flags);
}

const char *rd_file_view_get_src_file(const rd_file_view_type *file_view) {
    return fortio_filename_ref(file_view->fortio);
}

rd_file_view_type *rd_file_view_alloc(fortio_type *fortio, int *flags,
                                      inv_map_type *inv_map, bool owner) {
    rd_file_view_type *rd_file_view = new rd_file_view_type();

    rd_file_view->owner = owner;
    rd_file_view->fortio = fortio;
    rd_file_view->inv_map = inv_map;
    rd_file_view->flags = flags;

    return rd_file_view;
}

int rd_file_view_get_global_index(const rd_file_view_type *rd_file_view,
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
            const std::string &header = rd_file_kw_get_header(file_kw);
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

rd_file_kw_type *
rd_file_view_iget_file_kw(const rd_file_view_type *rd_file_view,
                          int global_index) {
    return rd_file_view->kw_list[global_index];
}

rd_file_kw_type *
rd_file_view_iget_named_file_kw(const rd_file_view_type *rd_file_view,
                                const char *kw, int ith) {
    int global_index = rd_file_view_get_global_index(rd_file_view, kw, ith);
    rd_file_kw_type *file_kw =
        rd_file_view_iget_file_kw(rd_file_view, global_index);
    return file_kw;
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
                                       rd_file_kw_type *file_kw) {
    rd_kw_type *rd_kw = rd_file_kw_get_kw_ptr(file_kw);
    if (!rd_kw) {
        if (fortio_assert_stream_open(rd_file_view->fortio)) {

            rd_kw = rd_file_kw_get_kw(file_kw, rd_file_view->fortio,
                                      rd_file_view->inv_map);

            if (rd_file_view_flags_set(rd_file_view, RD_FILE_CLOSE_STREAM))
                fortio_fclose_stream(rd_file_view->fortio);
        }
    }
    return rd_kw;
}

rd_kw_type *rd_file_view_iget_kw(const rd_file_view_type *rd_file_view,
                                 int index) {
    rd_file_kw_type *file_kw = rd_file_view_iget_file_kw(rd_file_view, index);
    return rd_file_view_get_kw(rd_file_view, file_kw);
}

void rd_file_view_index_fload_kw(const rd_file_view_type *rd_file_view,
                                 const char *kw, int index,
                                 const int_vector_type *index_map,
                                 char *io_buffer) {
    rd_file_kw_type *file_kw =
        rd_file_view_iget_named_file_kw(rd_file_view, kw, index);

    if (fortio_assert_stream_open(rd_file_view->fortio)) {
        offset_type offset = rd_file_kw_get_offset(file_kw);
        rd_data_type data_type = rd_file_kw_get_data_type(file_kw);
        int element_count = rd_file_kw_get_size(file_kw);

        rd_kw_fread_indexed_data(rd_file_view->fortio,
                                 offset + RD_KW_HEADER_FORTIO_SIZE, data_type,
                                 element_count, index_map, io_buffer);
    }
}

int rd_file_view_find_kw_value(const rd_file_view_type *rd_file_view,
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

const char *rd_file_view_iget_distinct_kw(const rd_file_view_type *rd_file_view,
                                          int index) {
    const std::string &string = rd_file_view->distinct_kw[index];
    return string.c_str();
}

int rd_file_view_get_num_distinct_kw(const rd_file_view_type *rd_file_view) {
    return rd_file_view->distinct_kw.size();
}

int rd_file_view_get_size(const rd_file_view_type *rd_file_view) {
    return rd_file_view->kw_list.size();
}

rd_data_type rd_file_view_iget_data_type(const rd_file_view_type *rd_file_view,
                                         int index) {
    rd_file_kw_type *file_kw = rd_file_view_iget_file_kw(rd_file_view, index);
    return rd_file_kw_get_data_type(file_kw);
}

int rd_file_view_iget_size(const rd_file_view_type *rd_file_view, int index) {
    rd_file_kw_type *file_kw = rd_file_view_iget_file_kw(rd_file_view, index);
    return rd_file_kw_get_size(file_kw);
}

const char *rd_file_view_iget_header(const rd_file_view_type *rd_file_view,
                                     int index) {
    rd_file_kw_type *file_kw = rd_file_view_iget_file_kw(rd_file_view, index);
    return rd_file_kw_get_header(file_kw);
}

rd_kw_type *rd_file_view_iget_named_kw(const rd_file_view_type *rd_file_view,
                                       const char *kw, int ith) {
    rd_file_kw_type *file_kw =
        rd_file_view_iget_named_file_kw(rd_file_view, kw, ith);
    return rd_file_view_get_kw(rd_file_view, file_kw);
}

rd_data_type
rd_file_view_iget_named_data_type(const rd_file_view_type *rd_file_view,
                                  const char *kw, int ith) {
    rd_file_kw_type *file_kw =
        rd_file_view_iget_named_file_kw(rd_file_view, kw, ith);
    return rd_file_kw_get_data_type(file_kw);
}

int rd_file_view_iget_named_size(const rd_file_view_type *rd_file_view,
                                 const char *kw, int ith) {
    rd_file_kw_type *file_kw =
        rd_file_view_iget_named_file_kw(rd_file_view, kw, ith);
    return rd_file_kw_get_size(file_kw);
}

void rd_file_view_replace_kw(rd_file_view_type *rd_file_view,
                             rd_kw_type *old_kw, rd_kw_type *new_kw,
                             bool insert_copy) {
    size_t index = 0;
    while (index < rd_file_view->kw_list.size()) {
        auto *ikw = rd_file_view->kw_list[index];
        if (rd_file_kw_ptr_eq(ikw, old_kw)) {
            /*
         Found it; observe that the vector_iset() function will
         automatically invoke the destructor on the old_kw.
      */
            rd_kw_type *insert_kw = new_kw;

            if (insert_copy)
                insert_kw = rd_kw_alloc_copy(new_kw);
            rd_file_kw_replace_kw(ikw, rd_file_view->fortio, insert_kw);

            rd_file_view_make_index(rd_file_view);
            return;
        }
        index++;
    }
    util_abort("%s: could not find rd_kw ptr: %p \n", __func__, old_kw);
}

bool rd_file_view_load_all(rd_file_view_type *rd_file_view) {
    bool loadOK = false;

    if (fortio_assert_stream_open(rd_file_view->fortio)) {
        for (rd_file_kw_type *file_kw : rd_file_view->kw_list)
            rd_file_kw_get_kw(file_kw, rd_file_view->fortio,
                              rd_file_view->inv_map);
        loadOK = true;
    }

    if (rd_file_view_flags_set(rd_file_view, RD_FILE_CLOSE_STREAM))
        fortio_fclose_stream(rd_file_view->fortio);

    return loadOK;
}

void rd_file_view_add_kw(rd_file_view_type *rd_file_view,
                         rd_file_kw_type *file_kw) {
    rd_file_view->kw_list.push_back(file_kw);
}

void rd_file_view_free(rd_file_view_type *rd_file_view) {

    for (auto &child_ptr : rd_file_view->child_list)
        rd_file_view_free(child_ptr);

    if (rd_file_view->owner) {
        for (auto &kw_ptr : rd_file_view->kw_list)
            rd_file_kw_free(kw_ptr);
    }

    delete rd_file_view;
}

void rd_file_view_free__(void *arg) {
    rd_file_view_type *rd_file_view = (rd_file_view_type *)arg;
    rd_file_view_free(rd_file_view);
}

int rd_file_view_get_num_named_kw(const rd_file_view_type *rd_file_view,
                                  const char *kw) {
    if (rd_file_view_has_kw(rd_file_view, kw)) {
        const auto &index_vector = rd_file_view->kw_index.at(kw);
        return index_vector.size();
    } else
        return 0;
}

void rd_file_view_fwrite(const rd_file_view_type *rd_file_view,
                         fortio_type *target, int offset) {
    for (size_t index = offset; index < rd_file_view->kw_list.size(); index++) {
        rd_kw_type *rd_kw = rd_file_view_iget_kw(rd_file_view, index);
        rd_kw_fwrite(rd_kw, target);
    }
}

int rd_file_view_iget_occurence(const rd_file_view_type *rd_file_view,
                                int global_index) {
    const rd_file_kw_type *file_kw = rd_file_view->kw_list[global_index];
    const char *header = rd_file_kw_get_header(file_kw);
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

void rd_file_view_fprintf_kw_list(const rd_file_view_type *rd_file_view,
                                  FILE *stream) {
    for (auto &file_kw : rd_file_view->kw_list) {
        char *type_name = rd_type_alloc_name(rd_file_kw_get_data_type(file_kw));
        fprintf(stream, "%-8s %7d:%s\n", rd_file_kw_get_header(file_kw),
                rd_file_kw_get_size(file_kw), type_name);
        free(type_name);
    }
}

rd_file_view_type *
rd_file_view_alloc_blockview2(const rd_file_view_type *rd_file_view,
                              const char *start_kw, const char *end_kw,
                              int occurence) {
    if ((start_kw != NULL) &&
        rd_file_view_get_num_named_kw(rd_file_view, start_kw) <= occurence)
        return NULL;

    rd_file_view_type *block_map =
        rd_file_view_alloc(rd_file_view->fortio, rd_file_view->flags,
                           rd_file_view->inv_map, false);
    size_t kw_index = 0;
    if (start_kw)
        kw_index =
            rd_file_view_get_global_index(rd_file_view, start_kw, occurence);

    {
        rd_file_kw_type *file_kw = rd_file_view->kw_list[kw_index];
        while (true) {
            rd_file_view_add_kw(block_map, file_kw);

            kw_index++;
            if (kw_index == rd_file_view->kw_list.size())
                break;
            else {
                if (end_kw) {
                    file_kw = rd_file_view->kw_list[kw_index];
                    if (strcmp(end_kw, rd_file_kw_get_header(file_kw)) == 0)
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
rd_file_view_type *
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
        file_view->child_list.push_back(child);

    return child;
}

rd_file_view_type *rd_file_view_add_blockview2(rd_file_view_type *rd_file_view,
                                               const char *start_kw,
                                               const char *end_kw,
                                               int occurence) {
    rd_file_view_type *child = rd_file_view_alloc_blockview2(
        rd_file_view, start_kw, end_kw, occurence);

    if (child)
        rd_file_view->child_list.push_back(child);

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
   typcially coming from one DATES keyword in the ECLIPSE
   datafile. Observe that the file does not have the block structure
   visualized on this figure, the only thing separating the blocks in
   the file is the occurence of a SEQNUM keyword.

 o Only a few of the report steps are present, namely 0, 5, 10, 20 and
   40.

 o The different blocks are not equally long, the fourth block has an
   extra keyword OIL_DEN.

To adress these keywords and blocks using different time coordinates
we have introduced the following concepts:

 report_step: This corresponds to the value of the SEQNUM keword,
    i.e. to do queries based on the report_step we must load the
    seqnum kewyord and read the value.

        rd_file_get_unrstmap_report_step( rd_file , 0 ) => A
        rd_file_get_unrstmap_report_step( rd_file , 1 ) => NULL

        rd_file_has_report_step( rd_file , 5 ) => True
        rd_file_has_report_step( rd_file , 2 ) => False

 sim_time: This correpsonds to the true simulation time of the report
    step, the simulation time is stored as integers DAY, MONTH, YEAR
    in the INTEHEAD keyword; the function INTEHEAD_date() will extract
    the DAY, MONTH and YEAR values from an INTEHEAD keyword instance
    and convert to a time_t instance. The functions:

     rd_file_get_unrstmap_sim_time() and rd_file_has_has_sim_time()

    can be used to query for simulation times and get the
    corresponding block maps.

 index/global_index : This is typically the global running index of
    the keyword in the file; this is the unique address of the keyword
    which is used for the final lookup.

 occurence: The nth' time a particular keyword has occured in the
    file, i.e. the SEQNUM keyword in block C is the third occurence of
    SEQNUM. Instead of occurence xxxx_index is also used to indicate
    the occurence of keyword xxxx. The occurence number is the integer
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

time_t rd_file_view_iget_restart_sim_date(const rd_file_view_type *rd_file_view,
                                          int seqnum_index) {
    time_t sim_time = -1;
    rd_file_view_type *seqnum_map =
        rd_file_view_alloc_blockview(rd_file_view, SEQNUM_KW, seqnum_index);

    if (seqnum_map != NULL) {
        rd_kw_type *intehead_kw =
            rd_file_view_iget_named_kw(seqnum_map, INTEHEAD_KW, 0);
        sim_time = rd_rsthead_date(intehead_kw);
        rd_file_view_free(seqnum_map);
    }

    return sim_time;
}

double rd_file_view_iget_restart_sim_days(const rd_file_view_type *rd_file_view,
                                          int seqnum_index) {
    double sim_days = 0;
    rd_file_view_type *seqnum_map =
        rd_file_view_alloc_blockview(rd_file_view, SEQNUM_KW, seqnum_index);

    if (seqnum_map != NULL) {
        rd_kw_type *doubhead_kw =
            rd_file_view_iget_named_kw(seqnum_map, DOUBHEAD_KW, 0);
        sim_days = rd_kw_iget_double(doubhead_kw, DOUBHEAD_DAYS_INDEX);
        rd_file_view_free(seqnum_map);
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

bool rd_file_view_has_sim_days(const rd_file_view_type *rd_file_view,
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

int rd_file_view_seqnum_index_from_sim_time(rd_file_view_type *parent_map,
                                            time_t sim_time) {
    int num_seqnum = rd_file_view_get_num_named_kw(parent_map, SEQNUM_KW);
    rd_file_view_type *seqnum_map;

    for (int s_idx = 0; s_idx < num_seqnum; s_idx++) {
        seqnum_map = rd_file_view_alloc_blockview(parent_map, SEQNUM_KW, s_idx);

        if (!seqnum_map)
            continue;

        bool sim = rd_file_view_has_sim_time(seqnum_map, sim_time);
        rd_file_view_free(seqnum_map);
        if (sim)
            return s_idx;
    }
    return -1;
}

int rd_file_view_seqnum_index_from_sim_days(rd_file_view_type *file_view,
                                            double sim_days) {
    int num_seqnum = rd_file_view_get_num_named_kw(file_view, SEQNUM_KW);
    int seqnum_index = 0;
    rd_file_view_type *seqnum_map;

    while (true) {
        seqnum_map =
            rd_file_view_alloc_blockview(file_view, SEQNUM_KW, seqnum_index);

        if (seqnum_map != NULL) {
            if (rd_file_view_has_sim_days(seqnum_map, sim_days)) {
                rd_file_view_free(seqnum_map);
                return seqnum_index;
            } else {
                rd_file_view_free(seqnum_map);
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
    fortio_fclose_stream(file_view->fortio);
}

void rd_file_view_write_index(const rd_file_view_type *file_view,
                              FILE *ostream) {
    int size = rd_file_view_get_size(file_view);
    util_fwrite_int(size, ostream);

    rd_file_kw_type *file_kw;
    for (int i = 0; i < size; i++) {
        file_kw = rd_file_view_iget_file_kw(file_view, i);
        rd_file_kw_fwrite(file_kw, ostream);
    }
}

rd_file_view_type *rd_file_view_fread_alloc(fortio_type *fortio, int *flags,
                                            inv_map_type *inv_map,
                                            FILE *istream) {

    int index_size = util_fread_int(istream);
    rd_file_kw_type **file_kw_list =
        rd_file_kw_fread_alloc_multiple(istream, index_size);
    if (file_kw_list) {
        rd_file_view_type *file_view =
            rd_file_view_alloc(fortio, flags, inv_map, true);
        for (int i = 0; i < index_size; i++)
            rd_file_view_add_kw(file_view, file_kw_list[i]);

        free(file_kw_list);
        rd_file_view_make_index(file_view);
        return file_view;
    } else {
        fprintf(stderr, "%s: error reading rd_file_type index file.\n",
                __func__);
        return NULL;
    }
}

rd_file_transaction_type *
rd_file_view_start_transaction(rd_file_view_type *file_view) {
    rd_file_transaction_type *t =
        (rd_file_transaction_type *)util_malloc(sizeof *t);
    int size = rd_file_view_get_size(file_view);
    t->file_view = file_view;
    t->ref_count = (int *)util_malloc(size * sizeof *t->ref_count);
    for (int i = 0; i < size; i++) {
        rd_file_kw_type *file_kw = rd_file_view_iget_file_kw(file_view, i);
        rd_file_kw_start_transaction(file_kw, &t->ref_count[i]);
    }

    return t;
}

void rd_file_view_end_transaction(rd_file_view_type *file_view,
                                  rd_file_transaction_type *transaction) {
    if (transaction->file_view != file_view)
        util_abort("%s: internal error - file_view / transaction mismatch\n",
                   __func__);

    const int *ref_count = transaction->ref_count;
    for (int i = 0; i < rd_file_view_get_size(file_view); i++) {
        rd_file_kw_type *file_kw = rd_file_view_iget_file_kw(file_view, i);
        rd_file_kw_end_transaction(file_kw, ref_count[i]);
    }
    free(transaction->ref_count);
    free(transaction);
}
