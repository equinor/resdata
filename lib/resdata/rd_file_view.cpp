#include <cstdio>
#include <ctime>
#include <algorithm>

#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <memory>
#include <optional>
#include <fmt/format.h>

#include <ert/util/int_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_file_flag.hpp>

namespace rd {

void FileView::make_index() {
    distinct_kw.clear();
    kw_index.clear();

    int global_index = 0;
    for (const auto &file_kw : kw_list) {
        const std::string &header = file_kw->get_header();
        if (kw_index.find(header) == kw_index.end())
            distinct_kw.push_back(header);

        auto &index_vector = kw_index[header];
        index_vector.push_back(global_index);
        global_index++;
    }
}

bool FileView::has_flags(FileMode flags) const {
    return (context->flags & flags) == flags;
}

void FileView::add_flag(FileMode flag) { context->flags |= flag; }

const std::string &FileView::filename() const {
    return context->fortio.filename();
}

void FileView::close() { context->fortio.fclose_stream(); }

bool FileView::drop_flags(FileMode flag) {
    bool flag_set = has_flags(flag);
    if (flag_set)
        context->flags &= ~flag;

    return flag_set;
}

rd_kw_type *FileView::get_kw(const std::shared_ptr<FileKW> &file_kw) {
    rd_kw_type *rd_kw = file_kw->get_kw_ptr();
    if (!rd_kw) {
        if (context->fortio.assert_stream_open()) {
            rd_kw = file_kw->get_kw(context->fortio);
            context->inv_map[rd_kw] = file_kw.get();

            if (has_flags(FileMode::CLOSE_STREAM))
                context->fortio.fclose_stream();
        }
    }
    return rd_kw;
}

void FileView::index_fload_kw(const std::string &kw, int index,
                              const int_vector_type *index_map,
                              char *io_buffer) {
    if (index < 0)
        throw std::invalid_argument(
            fmt::format("Got negative index in index_fload_kw: {}", index));

    auto file_kw = get_file_kw(kw, static_cast<size_t>(index));

    if (!file_kw)
        throw std::invalid_argument(std::string("Keyword '") + kw + "' index " +
                                    std::to_string(index) +
                                    " not found in file view");
    if (context->fortio.assert_stream_open()) {
        offset_type offset = file_kw->get_offset();
        rd_data_type data_type = file_kw->get_data_type();
        int element_count = file_kw->get_size();

        rd_kw_fread_indexed_data(context->fortio, offset, data_type,
                                 element_count, index_map, io_buffer);
    }
}

int FileView::find_kw_value(const std::string &kw, const void *value) {
    int global_index = -1;
    if (has_kw(kw)) {
        const auto &index_list = kw_index.at(kw);
        size_t index = 0;
        while (index < index_list.size()) {
            rd_kw_type *rd_kw = get_kw(index_list[index]);
            if (rd_kw_data_equal(rd_kw, value)) {
                global_index = index_list[index];
                break;
            }
            index++;
        }
    }
    return global_index;
}

bool FileView::load_all() {
    bool loadOK = false;

    if (context->fortio.assert_stream_open()) {
        for (auto &file_kw : kw_list) {
            auto rd_kw = file_kw->get_kw(context->fortio);
            context->inv_map[rd_kw] = file_kw.get();
        }
        loadOK = true;
    }

    if (has_flags(FileMode::CLOSE_STREAM))
        context->fortio.fclose_stream();

    return loadOK;
}

void FileView::write(ERT::FortIO &target, size_t offset) {
    for (size_t index = offset; index < kw_list.size(); index++) {
        rd_kw_type *rd_kw = get_kw(index);
        rd_kw_fwrite(rd_kw, target);
    }
}

int FileView::get_occurence(size_t global_index) {
    const auto &file_kw = kw_list[global_index];
    const std::string &header = file_kw->get_header();
    const auto &index_vector = kw_index.at(header);

    int occurence = -1;
    for (size_t i = 0; i < index_vector.size(); i++) {
        if (index_vector[i] == global_index)
            occurence = static_cast<int>(i);
    }
    if (occurence < 0)
        throw std::out_of_range(
            fmt::format("Could not find index {}", global_index));

    return occurence;
}

std::shared_ptr<FileView>
FileView::blockview(const std::optional<std::string> &start_kw,
                    const std::optional<std::string> &end_kw,
                    size_t occurence) {
    if (start_kw && num_named_kw(*start_kw) <= occurence)
        return {nullptr};

    auto block_map = std::make_shared<FileView>(context);
    auto begin =
        kw_list.begin() + (start_kw ? kw_index.at(*start_kw).at(occurence) : 0);
    auto stop = end_kw ? std::find_if(begin + 1, kw_list.end(),
                                      [&](const auto &kw) {
                                          return *end_kw == kw->get_header();
                                      })
                       : kw_list.end();
    block_map->kw_list.assign(begin, stop);
    block_map->make_index();
    return block_map;
}

/* There is no special datastructure for working with restart files,
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

        rd_file_has_report_step( rd_file , 5 ) => True
        rd_file_has_report_step( rd_file , 2 ) => False

 sim_time: This corresponds to the true simulation time of the report
    step. The simulation time is stored as integers DAY, MONTH, YEAR
    in the INTEHEAD keyword; the function INTEHEAD_date() will extract
    the DAY, MONTH and YEAR values from an INTEHEAD keyword instance
    and convert to a time_t instance. The functions rd_file_has_has_sim_time()
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
    call to create blockmaps. */

static time_t rsthead_date(int day, int month, int year) {
    return rd_make_date(day, month, year);
}

static time_t rd_rsthead_date(const rd_kw_type *intehead_kw) {
    return rsthead_date(rd_kw_iget_int(intehead_kw, INTEHEAD_DAY_INDEX),
                        rd_kw_iget_int(intehead_kw, INTEHEAD_MONTH_INDEX),
                        rd_kw_iget_int(intehead_kw, INTEHEAD_YEAR_INDEX));
}

time_t FileView::restart_sim_date(int seqnum_index) {
    time_t sim_time = -1;
    std::shared_ptr<FileView> seqnum_map =
        blockview(SEQNUM_KW, SEQNUM_KW, seqnum_index);

    if (seqnum_map) {
        rd_kw_type *intehead_kw = seqnum_map->get_kw(INTEHEAD_KW, 0);
        sim_time = rd_rsthead_date(intehead_kw);
    }

    return sim_time;
}

double FileView::restart_sim_days(int seqnum_index) {
    double sim_days = 0;
    std::shared_ptr<FileView> seqnum_map =
        blockview(SEQNUM_KW, SEQNUM_KW, seqnum_index);

    if (seqnum_map) {
        rd_kw_type *doubhead_kw = seqnum_map->get_kw(DOUBHEAD_KW, 0);
        sim_days = rd_kw_iget_double(doubhead_kw, DOUBHEAD_DAYS_INDEX);
    }

    return sim_days;
}

/* Will scan through the rd_file looking for INTEHEAD
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

find_sim_time with time_t of "01.03.2000" will return 1. This will
in general NOT agree with the DATES step number.

If the sim_time can not be found the function will return -1.

Observe that the function requires on-the-second-equality; which is
of course quite strict.

Each report step only has one occurence of SEQNUM, but one INTEHEAD
for each LGR; i.e. one should call iselect_rstblock() prior to
calling this function. */
int FileView::find_sim_time(time_t sim_time) {
    int seqnum_index = -1;
    if (has_kw(INTEHEAD_KW)) {
        const auto &intehead_index_list = kw_index.at(INTEHEAD_KW);
        for (size_t index = 0; index < intehead_index_list.size(); index++) {
            const rd_kw_type *intehead_kw = get_kw(intehead_index_list[index]);
            if (rd_rsthead_date(intehead_kw) == sim_time) {
                seqnum_index = static_cast<int>(index);
                break;
            }
        }
    }
    return seqnum_index;
}

bool FileView::has_sim_time(time_t sim_time) {
    size_t num_INTEHEAD = num_named_kw(INTEHEAD_KW);
    if (num_INTEHEAD == 0)
        return false; /* We have no INTEHEAD headers - probably not a restart file at all. */
    else {
        int intehead_index = 0;
        while (true) {
            time_t itime = restart_sim_date(intehead_index);

            if (itime == sim_time) /* Perfect hit. */
                return true;

            if (itime > sim_time)
                return false; /* We have gone past the target_time - i.e. we do not have it. */

            intehead_index++;
            if (intehead_index == num_INTEHEAD)
                return false; /* We have iterated through the whole thing without finding sim_time. */
        }
    }
}

bool FileView::has_sim_days(double sim_days) {
    size_t num_DOUBHEAD = num_named_kw(DOUBHEAD_KW);
    if (num_DOUBHEAD == 0)
        return false; /* We have no DOUBHEAD headers - probably not a restart file at all. */
    else {
        int doubhead_index = 0;
        while (true) {
            double file_sim_days = restart_sim_days(doubhead_index);

            if (util_double_approx_equal(sim_days, file_sim_days))
                return true;

            if (file_sim_days > sim_days)
                return false;

            doubhead_index++;
            if (doubhead_index == num_DOUBHEAD)
                return false;
        }
    }
}

std::shared_ptr<FileView>
FileView::restart_view_from_seqnum_index(size_t index) {
    return blockview(SEQNUM_KW, SEQNUM_KW, index);
}
std::shared_ptr<FileView>
FileView::restart_view_from_report_step(int report_step) {
    int global_index = find_kw_value(SEQNUM_KW, &report_step);
    if (global_index < 0)
        throw std::invalid_argument(
            fmt::format("No such restart block could be identified"));
    return restart_view_from_seqnum_index(get_occurence(global_index));
}

std::shared_ptr<FileView>
FileView::restart_view_from_sim_time(time_t sim_time) {
    size_t num_seqnum = num_named_kw(SEQNUM_KW);

    for (size_t s_idx = 0; s_idx < num_seqnum; s_idx++) {
        std::shared_ptr<FileView> seqnum_map =
            blockview(SEQNUM_KW, SEQNUM_KW, s_idx);

        if (!seqnum_map)
            continue;

        if (seqnum_map->has_sim_time(sim_time))
            return seqnum_map;
    }
    throw std::invalid_argument(
        fmt::format("No such restart block could be identified"));
}

std::shared_ptr<FileView>
FileView::restart_view_from_sim_days(double sim_days) {
    size_t num_seqnum = num_named_kw(SEQNUM_KW);
    size_t seqnum_index = 0;

    while (true) {
        std::shared_ptr<FileView> seqnum_map =
            blockview(SEQNUM_KW, SEQNUM_KW, seqnum_index);

        if (seqnum_map) {
            if (seqnum_map->has_sim_days(sim_days)) {
                return seqnum_map;
            } else {
                seqnum_index++;
            }
        }

        if (num_seqnum == seqnum_index)
            throw std::invalid_argument(
                fmt::format("No such restart block could be identified"));
    }
}

std::shared_ptr<FileView> FileView::summary_view(int report_step) {
    return blockview(SEQHDR_KW, SEQHDR_KW, report_step);
}

void FileView::write_index(FILE *ostream) const {
    util_fwrite_int(static_cast<int>(size()), ostream);

    for (const auto &file_kw : kw_list) {
        file_kw->write_header(ostream);
    }
}

std::shared_ptr<FileView> FileView::read(std::shared_ptr<FileContext> context,
                                         FILE *istream) {

    int index_size = util_fread_int(istream);
    if (index_size < 0)
        return {nullptr};
    auto file_view = std::make_shared<FileView>(std::move(context));

    try {
        file_view->kw_list =
            FileKW::read(istream, static_cast<size_t>(index_size));
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return nullptr;
    }

    file_view->make_index();
    return file_view;
}

void FileView::clear() {
    for (const auto &file_kw : kw_list) {
        if (auto *rd_kw = file_kw->get_kw_ptr())
            context->inv_map.erase(rd_kw);
        file_kw->clear();
    }
}
} // namespace rd
