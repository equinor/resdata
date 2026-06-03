#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <fmt/core.h>
#include <limits>
#include <locale.h>

#include <algorithm>
#include <ios>
#include <memory>
#include <new>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <optional>
#include <system_error>
#include <vector>
#include <map>
#include <utility>
#include <array>
#include <set>

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
#include <resdata/rd_smspec.hpp>
#include <resdata/smspec_node.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_file_view.hpp>

#include <detail/util/path.hpp>

namespace fs = std::filesystem;

#define RD_SMSPEC_ID 806647
#define PARAMS_GLOBAL_DEFAULT -99

typedef std::map<std::string, const rd::smspec_node *> node_map;

struct rd_smspec_struct {
    UTIL_TYPE_ID_DECLARATION;
    /*
    All the hash tables listed below here are different ways to access
    smspec_node instances. The actual smspec_node instances are
    owned by the smspec_nodes vector;
  */
    node_map field_var_index;
    node_map misc_var_index; /* Variables like 'TCPU' and 'NEWTON'. */
    node_map
        gen_var_index /* This is "everything" - things can either be found as gen_var("WWCT:OP_X") or as well_var("WWCT" , "OP_X") */
        ;

    std::map<std::string, node_map>
        well_var_index; /* Indexes for all well variables:
                                                     {well1: {var1: index1 , var2: index2} , well2: {var1: index1 , var2: index2}} */
    std::map<std::string, node_map>
        group_var_index; /* Indexes for group variables.*/
    std::map<int, node_map>
        region_var_index; /* The stored index is an offset. */
    std::map<int, node_map> block_var_index; /* Block variables like BPR */
    std::map<std::string, std::map<int, node_map>>
        well_completion_var_index; /* Indexes for completion indexes .*/

    std::vector<std::unique_ptr<rd::smspec_node>> smspec_nodes;
    bool write_mode;
    bool need_nums;
    std::vector<int> index_map;
    std::map<int, int> inv_index_map;
    int params_size;

    int time_seconds;
    int grid_dims[3]; /* Grid dimensions - in DIMENS[1,2,3] */
    int num_regions;
    int Nwells, param_offset;
    std::string
        key_join_string; /* The string used to join keys when building gen_key keys - typically ":" -
                                                      but arbitrary - NOT necessary to be able to invert the joining. */
    std::string
        header_file; /* FULL path to the currenbtly loaded header_file. */

    bool
        formatted; /* Has this summary instance been loaded from a formatted (i.e. FSMSPEC file) or unformatted (i.e. SMSPEC) file. */
    time_t sim_start_time; /* When did the simulation start - worldtime. */

    int time_index; /* The fields time_index, day_index, month_index and year_index */
    int day_index; /* are used by the rd_sum_data object to locate per. timestep */
    int month_index; /* time information. */
    int year_index;
    bool has_lgr;
    std::vector<float> params_default;

    std::string restart_case;
    ert_rd_unit_enum unit_system;
    int restart_step;
};

struct rd_sum_tstep_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::vector<float>
        data; /* A memcpy copy of the PARAMS vector in rd_kw instance - the raw data. */
    time_t
        sim_time; /* The true time (i.e. 20.th of october 2010) of corresponding to this timestep. */
    int ministep; /* The simulator time-step number; one ministep per numerical timestep. */
    int report_step; /* The report step this time-step is part of - in general there can be many timestep for each report step. */
    double sim_seconds; /* Accumulated simulation time up to this ministep. */
    int internal_index; /* Used for lookups of the next / previous ministep based on an existing ministep. */
    const rd_smspec_type *
        smspec; /* The smespec header information for this tstep - must be compatible. */
};

#define RD_SUM_TSTEP_ID 88631

namespace rd {

struct IndexNode {

    IndexNode(time_t sim_time, double sim_seconds, int report_step)
        : sim_time(sim_time), sim_seconds(sim_seconds),
          report_step(report_step) {}

    time_t sim_time;
    double sim_seconds;
    int report_step;
};

class TimeIndex {
public:
    void add(time_t sim_time, double sim_seconds, int report_step) {
        int internal_index = static_cast<int>(this->nodes.size());
        this->nodes.emplace_back(sim_time, sim_seconds, report_step);

        /* Indexing internal_index - report_step */
        if (static_cast<int>(this->report_map.size()) <= report_step)
            this->report_map.resize(
                report_step + 1,
                std::pair<int, int>(std::numeric_limits<int>::max(), -1));

        auto &range = this->report_map[report_step];
        range.first = std::min(range.first, internal_index);
        range.second = std::max(range.second, internal_index);
    }

    bool has_report(int report_step) const {
        if (report_step >= static_cast<int>(this->report_map.size()))
            return false;

        const auto &range_pair = this->report_map[report_step];
        if (range_pair.second < 0)
            return false;

        return true;
    }

    void clear() {
        this->nodes.clear();
        this->report_map.clear();
    }

    const IndexNode &operator[](size_t index) const {
        return this->nodes[index];
    }

    const IndexNode &back() const { return this->nodes.back(); }

    size_t size() const { return this->nodes.size(); }

    std::pair<int, int> &report_range(int report_step) {
        return this->report_map[report_step];
    }

    const std::pair<int, int> &report_range(int report_step) const {
        return this->report_map[report_step];
    }

private:
    std::vector<IndexNode> nodes;
    std::vector<std::pair<int, int>> report_map;
};
class unsmry_loader {
public:
    unsmry_loader(const rd_smspec_type *smspec, const std::string &filename,
                  int file_options);
    ~unsmry_loader();

    std::vector<double> get_vector(int pos) const;
    std::vector<double> sim_seconds() const;
    std::vector<time_t> sim_time() const;
    int length() const;

    time_t iget_sim_time(int time_index) const;
    double iget_sim_seconds(int time_index) const;
    std::vector<int> report_steps(int offset) const;
    double iget(int time_index, int params_index) const;

private:
    int size; //Number of entries in the smspec index
    int time_index;
    int time_seconds;
    time_t sim_start;
    int m_length; //Number of PARAMS in the UNSMRY file

    std::array<int, 3> date_index;
    rd_file_type *file;
    rd_file_view_type *file_view;
};

class rd_sum_file_data {

public:
    rd_sum_file_data(const rd_smspec_type *smspec);
    ~rd_sum_file_data();
    const rd_smspec_type *smspec() const;

    int length_before(time_t end_time) const;
    void get_time(int length, time_t *data);
    void get_data(int params_index, int length, double *data);
    int length() const;
    time_t get_data_start() const;
    time_t get_sim_end() const;
    double iget(int time_index, int params_index) const;
    time_t iget_sim_time(int time_index) const;
    double iget_sim_days(int time_index) const;
    double iget_sim_seconds(int time_index) const;
    rd_sum_tstep_type *iget_ministep(int internal_index) const;
    double get_days_start() const;
    double get_sim_length() const;

    std::pair<int, int> report_range(int report_step) const;
    bool report_step_equal(const rd_sum_file_data &other, bool strict) const;
    int report_before(time_t end_time) const;
    int get_time_report(int max_internal_index, time_t *data);
    int get_data_report(int params_index, int max_internal_index, double *data,
                        double default_value);
    int first_report() const;
    int last_report() const;
    int iget_report(int time_index) const;
    bool has_report(int report_step) const;
    int report_step_from_days(double sim_days) const;
    int report_step_from_time(time_t sim_time) const;

    rd_sum_tstep_type *add_new_tstep(int report_step, double sim_seconds);
    bool can_write() const;
    void fwrite_unified(fortio_type *fortio) const;
    void fwrite_multiple(const std::string &rd_case, bool fmt_case) const;
    bool fread(const stringlist_type *filelist, bool lazy_load,
               int file_options);

private:
    const rd_smspec_type *rd_smspec;

    TimeIndex index;
    vector_type *data;

    std::unique_ptr<rd::unsmry_loader> loader;

    void append_tstep(rd_sum_tstep_type *tstep);
    void build_index();
    void fwrite_report(int report_step, fortio_type *fortio) const;
    bool check_file(rd_file_type *rd_file);
    void add_rd_file(int report_step, const rd_file_view_type *summary_view);
};

} // namespace rd
namespace {

/*
  The class CaseIndex and the struct IndexNode are used to maintain a list of
  the rd_sum_file_data instances, and lookup the correct one based one various
  time related arguments.
*/

struct IndexNode {
    IndexNode(int d, int o, int l) {
        this->data_index = d;
        this->offset = o;
        this->length = l;
    }

    int end() const { return this->offset + this->length; }

    int data_index;
    int offset;
    int length;
    int report1;
    int report2;
    time_t time1;
    time_t time2;
    double days1;
    double days2;
    std::vector<int> params_map;
};

class CaseIndex {
public:
    IndexNode &add(int length) {
        int offset = 0;
        int data_index = this->index.size();

        if (!this->index.empty())
            offset = this->index.back().end();

        this->index.emplace_back(data_index, offset, length);
        return this->index.back();
    }

    /*
  The lookup_time() and lookup_report() methods will lookup which file_data
  instance corresponds to the time/report argument. The methods will return two
  pointers to file_data instances, if the argument is inside one file_data
  instance the pointers will be equal - otherwise they will point to the
  file_data instance before and after the argument:

  File 1                     File 2
  |------|-----|------|      |----|----------|---|
      /|\                /|\
       |                  |
       |                  |
       A                  B

  For time A the lookup_time function will return <file1,file1> whereas for time
  B the function will return <file1,file2>.
 */

    std::pair<const IndexNode *, const IndexNode *>
    lookup_time(time_t sim_time) const {
        auto iter = this->index.begin();
        auto next = this->index.begin();
        if (sim_time < iter->time1)
            throw std::invalid_argument("Simulation time out of range");

        ++next;
        while (true) {
            double t1 = iter->time1;
            double t2 = iter->time2;

            if (sim_time >= t1) {
                if (sim_time <= t2)
                    return std::make_pair<const IndexNode *, const IndexNode *>(
                        &(*iter), &(*iter));

                if (next == this->index.end())
                    throw std::invalid_argument("Simulation days out of range");

                if (sim_time < next->time1)
                    return std::make_pair<const IndexNode *, const IndexNode *>(
                        &(*iter), &(*next));
            }
            ++next;
            ++iter;
        }
    }

    std::pair<const IndexNode *, const IndexNode *>
    lookup_days(double days) const {
        auto iter = this->index.begin();
        auto next = this->index.begin();
        if (days < iter->days1)
            throw std::invalid_argument("Simulation days out of range");

        ++next;
        while (true) {
            double d1 = iter->days1;
            double d2 = iter->days2;

            if (days >= d1) {
                if (days <= d2)
                    return std::make_pair<const IndexNode *, const IndexNode *>(
                        &(*iter), &(*iter));

                if (next == this->index.end())
                    throw std::invalid_argument("Simulation days out of range");

                if (days < next->days1)
                    return std::make_pair<const IndexNode *, const IndexNode *>(
                        &(*iter), &(*next));
            }
            ++next;
            ++iter;
        }
    }

    const IndexNode &lookup(int internal_index) const {
        for (const auto &node : this->index)
            if (internal_index >= node.offset && internal_index < node.end())
                return node;

        throw std::invalid_argument("Internal error when looking up index: " +
                                    std::to_string(internal_index));
    }

    const IndexNode &lookup_report(int report) const {
        for (const auto &node : this->index)
            if (node.report1 <= report && node.report2 >= report)
                return node;

        throw std::invalid_argument("Internal error when looking up report: " +
                                    std::to_string(report));
    }

    /*
      This will check that we have a datafile which report range covers the
      report argument, in adition there can be 'holes' in the series - that must
      be checked by actually querying the data_file object.
    */

    bool has_report(int report) const {
        for (const auto &node : this->index)
            if (node.report1 <= report && node.report2 >= report)
                return true;

        return false;
    }

    IndexNode &back() { return this->index.back(); }

    void clear() { this->index.clear(); }

    int length() const { return this->index.back().end(); }

    std::vector<IndexNode>::const_iterator begin() const {
        return this->index.begin();
    }

    std::vector<IndexNode>::const_iterator end() const {
        return this->index.end();
    }

private:
    std::vector<IndexNode> index;
};

} // namespace

typedef struct rd_sum_data_struct rd_sum_data_type;

struct rd_sum_data_struct {
    const rd_smspec_type *smspec;
    std::vector<std::shared_ptr<rd::rd_sum_file_data>>
        data_files; /** These data files are shared_ptr
                       as rd_sum_data_add_case makes file_data
                       ownership shared among rd_sum_data. */
    CaseIndex index;
};

using rd_smspec_ptr =
    std::unique_ptr<rd_smspec_type, decltype(&rd_smspec_free)>;

#define RD_SUM_ID 89067

using rd_sum_data_ptr = std::unique_ptr<rd_sum_data_type>;

struct rd_sum_struct {
    UTIL_TYPE_ID_DECLARATION;
    rd_smspec_ptr smspec{
        nullptr,
        &rd_smspec_free}; /* Internalized version of the SMSPEC file. */
    rd_sum_data_ptr data{nullptr};
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

static rd_sum_tstep_type *rd_sum_tstep_alloc(int report_step, int ministep_nr,
                                             const rd_smspec_type *smspec) {
    rd_sum_tstep_type *tstep = new rd_sum_tstep_type();
    UTIL_TYPE_ID_INIT(tstep, RD_SUM_TSTEP_ID);
    tstep->smspec = smspec;
    tstep->report_step = report_step;
    tstep->ministep = ministep_nr;
    tstep->data.resize(smspec->params_size);
    return tstep;
}

static void rd_sum_tstep_set_time_info_from_seconds(rd_sum_tstep_type *tstep,
                                                    time_t sim_start,
                                                    double sim_seconds) {
    tstep->sim_seconds = sim_seconds;
    tstep->sim_time = sim_start;
    util_inplace_forward_seconds_utc(&tstep->sim_time, tstep->sim_seconds);
}

static void rd_sum_tstep_set_time_info_from_date(rd_sum_tstep_type *tstep,
                                                 time_t sim_start,
                                                 time_t sim_time) {
    tstep->sim_time = sim_time;
    tstep->sim_seconds = util_difftime_seconds(sim_start, tstep->sim_time);
}

static void rd_sum_tstep_set_time_info(rd_sum_tstep_type *tstep,
                                       const rd_smspec_type *smspec) {
    int date_day_index = smspec->day_index;
    int date_month_index = smspec->month_index;
    int date_year_index = smspec->year_index;
    int sim_time_index = smspec->time_index;
    time_t sim_start = smspec->sim_start_time;

    if (sim_time_index >= 0) {
        double sim_time = tstep->data[sim_time_index];
        double sim_seconds = sim_time * smspec->time_seconds;
        rd_sum_tstep_set_time_info_from_seconds(tstep, sim_start, sim_seconds);
    } else if (date_day_index >= 0) {
        int day = util_roundf(tstep->data[date_day_index]);
        int month = util_roundf(tstep->data[date_month_index]);
        int year = util_roundf(tstep->data[date_year_index]);

        time_t sim_time = rd_make_date(day, month, year);
        rd_sum_tstep_set_time_info_from_date(tstep, sim_start, sim_time);
    } else
        util_abort("%s: Hmmm - could not extract date/time information from "
                   "SMSPEC header file? \n",
                   __func__);
}

static double rd_sum_tstep_iget(const rd_sum_tstep_type *ministep, int index) {
    if ((index >= 0) && (index < (int)ministep->data.size()))
        return ministep->data[index];
    else {
        util_abort("%s: param index:%d invalid: Valid range: [0,%d) \n",
                   __func__, index, ministep->data.size());
        return -1;
    }
}

namespace rd {

unsmry_loader::unsmry_loader(const rd_smspec_type *smspec,
                             const std::string &filename, int file_options)
    : size(smspec->params_size), time_index(smspec->time_index),
      time_seconds(smspec->time_seconds), sim_start(smspec->sim_start_time) {
    rd_file_type *file = rd_file_open(filename.c_str(), file_options);
    if (!file)
        throw std::bad_alloc();

    if (!rd_file_has_kw(file, PARAMS_KW)) {
        rd_file_close(file);
        throw std::bad_alloc();
    }

    if (rd_file_get_num_named_kw(file, PARAMS_KW) !=
        rd_file_get_num_named_kw(file, MINISTEP_KW)) {
        rd_file_close(file);
        throw std::bad_alloc();
    }

    this->date_index = {
        {smspec->day_index, smspec->month_index, smspec->year_index}};
    this->file = file;
    this->file_view = rd_file_get_global_view(this->file);
    this->m_length = rd_file_view_get_num_named_kw(this->file_view, PARAMS_KW);
}

unsmry_loader::~unsmry_loader() { rd_file_close(file); }

int unsmry_loader::length() const { return this->m_length; }

std::vector<double> unsmry_loader::get_vector(int pos) const {
    if (pos >= size)
        throw std::out_of_range(
            "unsmry_loader::get_vector pos: " + std::to_string(pos) +
            " PARAMS_SIZE: " + std::to_string(size));

    std::vector<double> data(this->length());
    int_vector_type *index_map = int_vector_alloc(1, pos);
    char buffer[4];

    for (int index = 0; index < this->length(); index++) {
        rd_file_view_index_fload_kw(file_view, PARAMS_KW, index, index_map,
                                    buffer);
        float *data_value = (float *)buffer;
        data[index] = *data_value;
    }
    int_vector_free(index_map);

    if (rd_file_view_flags_set(file_view, RD_FILE_CLOSE_STREAM))
        rd_file_view_fclose_stream(file_view);

    return data;
}

// This is horribly inefficient
double unsmry_loader::iget(int time_index, int params_index) const {
    int_vector_type *index_map = int_vector_alloc(1, params_index);
    float value;
    rd_file_view_index_fload_kw(this->file_view, PARAMS_KW, time_index,
                                index_map, (char *)&value);
    int_vector_free(index_map);
    return value;
}

time_t unsmry_loader::iget_sim_time(int time_index) const {
    if (this->time_index >= 0) {
        double sim_seconds = this->iget_sim_seconds(time_index);
        time_t sim_time = this->sim_start;
        util_inplace_forward_seconds_utc(&sim_time, sim_seconds);
        return sim_time;
    } else {
        int_vector_type *index_map = int_vector_alloc(3, 0);
        int_vector_iset(index_map, 0, this->date_index[0]);
        int_vector_iset(index_map, 1, this->date_index[1]);
        int_vector_iset(index_map, 2, this->date_index[2]);

        float values[3];
        rd_file_view_index_fload_kw(this->file_view, PARAMS_KW, time_index,
                                    index_map, (char *)&values);
        int_vector_free(index_map);

        return rd_make_date(util_roundf(values[0]), util_roundf(values[1]),
                            util_roundf(values[2]));
    }
}

double unsmry_loader::iget_sim_seconds(int time_index) const {
    if (this->time_index >= 0) {
        double raw_time = this->iget(time_index, this->time_index);
        return raw_time * this->time_seconds;
    } else {
        time_t sim_time = this->iget_sim_time(time_index);
        return util_difftime_seconds(this->sim_start, sim_time);
    }
}

std::vector<int> unsmry_loader::report_steps(int offset) const {
    std::vector<int> report_steps;
    int current_step = offset;
    for (int i = 0; i < rd_file_view_get_size(this->file_view); i++) {
        const rd_file_kw_type *file_kw =
            rd_file_view_iget_file_kw(this->file_view, i);
        if (util_string_equal(SEQHDR_KW, rd_file_kw_get_header(file_kw)))
            current_step++;

        if (util_string_equal(PARAMS_KW, rd_file_kw_get_header(file_kw)))
            report_steps.push_back(current_step);
    }
    return report_steps;
}

std::vector<time_t> unsmry_loader::sim_time() const {
    if (this->time_index >= 0) {
        const std::vector<double> sim_seconds = this->sim_seconds();
        std::vector<time_t> st(this->length(), this->sim_start);

        for (size_t i = 0; i < st.size(); i++)
            util_inplace_forward_seconds_utc(&st[i], sim_seconds[i]);

        return st;

    } else {
        const auto day = this->get_vector(this->date_index[0]);
        const auto month = this->get_vector(this->date_index[1]);
        const auto year = this->get_vector(this->date_index[2]);
        std::vector<time_t> st(this->length());

        for (size_t i = 0; i < st.size(); i++)
            st[i] = rd_make_date(util_round(day[i]), util_round(month[i]),
                                 util_round(year[i]));

        return st;
    }
}

std::vector<double> unsmry_loader::sim_seconds() const {
    if (this->time_index >= 0) {
        std::vector<double> seconds = this->get_vector(this->time_index);
        for (size_t i = 0; i < seconds.size(); i++)
            seconds[i] *= this->time_seconds;

        return seconds;
    } else {
        std::vector<time_t> st = this->sim_time();
        std::vector<double> seconds(st.size());

        for (size_t i = 0; i < st.size(); i++)
            seconds[i] = util_difftime_seconds(this->sim_start, st[i]);

        return seconds;
    }
}

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

/*
   About ministeps and report steps.
   ---------------------------------

   A sequence of summary data will typically look like this:

   ------------------
   SEQHDR            \
   MINISTEP  0        |
   PARAMS    .....    |
   MINISTEP  1        |==> This is REPORT STEP 1, in file BASE.S00001
   PARAMS    .....    |
   MINISTEP  2        |
   PARAMS    .....   /
   ------------------
   SEQHDR            \
   MINISTEP  3        |
   PARAMS    .....    |
   MINISTEP  4        |
   PARAMS    .....    |
   MINISTEP  5        |==> This is REPORT STEP 2, in file BASE.S0002
   PARAMS    .....    |
   MINISTEP  6        |
   PARAMS    .....    |
   SEQHDR             |
   MINISTEP  7        |
   PARAMS    .....   /
   ------------------


   Observe the following:

     * The MINISTEP counter runs continously, and does not
       differentiate between unified files and not unified files.

     * When using multiple files we can read off the report number
       from the filename, for unified files this is IMPOSSIBLE, and we
       just have to assume that the first block corresponds to
       report_step 1 and then count afterwards.

     * When asking for a summary variable at a particular REPORT STEP
       (as we do in enkf) it is ambigous as to which ministep within
       the block one should use. The convention we have employed
       (which corresponds to the old RPTONLY based behaviour) is to
       use the last ministep in the block.

     * There is no BASE.SOOOO file

     * The report steps are halfopen intervals in the "wrong way":
       (....]




   About MINISTEP, REPORTSTEP, rates and continous sim_time/sim_days:
   ------------------------------------------------------------------

   For summary files the smallest unit of time resolution is
   called the ministep - a ministep corresponds to a time step in the
   underlying partial differential equation, i.e. the length of the
   timesteps is controlled by the simulator itself - there is no finer
   temporal resolution.

   The user has told the simulator to store (i.e. save to file
   results) the results at reportsteps. A reportstep will typically
   consist of several ministeps. The timeline below shows a simulation
   consisting of two reportsteps:


                                                 S0001                                          S0002
   ||------|------|------------|------------------||----------------------|----------------------||
          M1     M2           M3                 M4                      M5                     M6

   The first reportstep consist of four ministeps, the second
   reportstep consists of only two ministeps. As a user you have no
   control over the length/number of ministeps apart from:

      1. Indirectly through the TUNING keywords.
      2. A ministep will always end at a report step.


   RPTONLY: In conjunction with enkf it has been customary to use the
   keyword RPTONLY. This is purely a storage directive, the effect is
   that only the ministep ending at the REPORT step is reported,
   i.e. in the case above we would get the ministeps [M4 , M6], where
   the ministeps M4 and M6 will be unchanged, and there will be many
   'holes' in the timeline.

   About truetime: The ministeps have a finite length; this implies
   that

     [rates]: The ministep value is NOT actually an instantaneous
        value, it is the total production during the ministep period
        - divided by the length of the ministep. I.e. it is an average
        value. (I.e. the differential time element dt is actually quite
        looong).

     [state]: For state variables (this will include total production
        of various phases), the ministep value corresponds to the
        reservoir state at THE END OF THE MINISTEP.

   This difference between state variables and rates implies a
   difference in how continous time-variables (in the middle of a
   ministep) are reported, i.e.


   S0000                                                      S0001
   ||--------------|---------------|------------X-------------||
                  M1              M2           /|\            M3
                                                |
                                                |

   We have enteeed the sim_days/sim_time cooresponding to the location
   of 'X' on the timeline, i.e. in the middle of ministep M3. If we
   are interested in the rate at this time the function:

        rd_sum_data_get_from_sim_time()

   will just return the M3 value, whereas if you are interested in
   e.g. pressure at this time the function will return a weighted
   average of the M2 and M3 values. Whether a variable in question is
   interpreted as a 'rate' is effectively determined by the
   rd_smspec_set_rate_var() function in rd_smspec.c.



   Indexing and _get() versus _iget()
   ----------------------------------
   As already mentionded the set of ministeps is not necessarrily a
   continous series, we can easily have a series of ministeps with
   "holes" in it, and the series can also start on a non-zero
   value. Internally all the ministeps are stored in a dense, zero
   offset vector instance; and we must be able to translate back and
   forth between ministep_nr and internal index.

   Partly due to EnKF heritage the MINISTEP nr has been the main
   method to access the time dimension of the data, i.e. all the
   functions like rd_sum_get_general_var() expect the time direction
   to be given as a ministep; however it is also possible to get the
   data by giving an internal (not that internal ...) index. In
   rd_sum_data.c the latter functions have _iget():


      rd_sum_data_get_xxx : Expects the time direction given as a ministep_nr.
      rd_sum_data_iget_xxx: Expects the time direction given as an internal index.

*/

rd_sum_file_data::rd_sum_file_data(const rd_smspec_type *smspec)
    : rd_smspec(smspec), data(vector_alloc_new()) {}

rd_sum_file_data::~rd_sum_file_data() { vector_free(data); }

int rd_sum_file_data::length() const {
    if (this->loader)
        return this->loader->length();
    else
        return this->index.size();
}

int rd_sum_file_data::length_before(time_t end_time) const {
    int offset = 0;
    while (true) {
        time_t itime = this->iget_sim_time(offset);
        if (itime >= end_time)
            return offset;

        offset += 1;
        if (offset == this->length())
            return offset;
    }
}

int rd_sum_file_data::report_before(time_t end_time) const {
    if (end_time < this->first_report())
        throw std::invalid_argument("time argument before first report step");

    int r = this->first_report();
    int last_report = this->last_report();
    while (true) {
        if (r == last_report)
            return last_report;

        auto next_range = this->index.report_range(r + 1);
        if (this->iget_sim_time(next_range.first) > end_time)
            return r;

        r += 1;
    }
}

int rd_sum_file_data::first_report() const {
    const auto &node = this->index[0];
    return node.report_step;
}

int rd_sum_file_data::last_report() const {
    const auto &node = this->index.back();
    return node.report_step;
}

time_t rd_sum_file_data::get_data_start() const {
    if (this->index.size() == 0)
        throw std::out_of_range("rd_sum_data_get_sim_end: index empty");
    const auto &node = this->index[0];
    return node.sim_time;
}

time_t rd_sum_file_data::get_sim_end() const {
    if (this->index.size() == 0)
        throw std::out_of_range(
            "rd_sum_file_data::get_sim_end(): index size is 0");
    const auto &node = this->index.back();
    return node.sim_time;
}

time_t rd_sum_file_data::iget_sim_time(int time_index) const {
    const auto &node = this->index[time_index];
    return node.sim_time;
}

/*
  Will return the length of the simulation in whatever units were used in input.
*/
double rd_sum_file_data::get_sim_length() const {
    const auto &node = this->index.back();
    return node.sim_seconds / this->rd_smspec->time_seconds;
}

double rd_sum_file_data::iget(int time_index, int params_index) const {
    if (this->loader)
        return this->loader->iget(time_index, params_index);
    else {
        const rd_sum_tstep_type *ministep_data = iget_ministep(time_index);
        return rd_sum_tstep_iget(ministep_data, params_index);
    }
}

static void rd_sum_tstep_free__(void *__ministep) {
    rd_sum_tstep_type *ministep = rd_sum_tstep_safe_cast(__ministep);
    rd_sum_tstep_free(ministep);
}

void rd_sum_file_data::append_tstep(rd_sum_tstep_type *tstep) {
    /*
     Here the tstep is just appended naively, the vector will be
     sorted by ministep_nr before the data instance is returned.
  */

    vector_append_owned_ref(data, tstep, rd_sum_tstep_free__);
}

/*
  This function is meant to be called in write mode; and will create a
  new and empty tstep which is appended to the current data. The tstep
  will also be returned, so the calling scope can call
  rd_sum_tstep_iset() to set elements in the tstep.
*/

rd_sum_tstep_type *rd_sum_file_data::add_new_tstep(int report_step,
                                                   double sim_seconds) {
    int ministep_nr = vector_get_size(data);
    rd_sum_tstep_type *tstep = rd_sum_tstep_alloc_new(report_step, ministep_nr,
                                                      sim_seconds, rd_smspec);
    rd_sum_tstep_type *prev_tstep = NULL;

    if (vector_get_size(data) > 0)
        prev_tstep = (rd_sum_tstep_type *)vector_get_last(data);

    append_tstep(tstep);

    bool rebuild_index = true;
    /*
    In the simple case that we just add another timestep to the
    currently active report_step, we do a limited update of the
    index, otherwise we call rd_sum_data_build_index() to get a
    full recalculation of the index.
  */
    if (!prev_tstep)
        goto exit;

    if (rd_sum_tstep_get_report(prev_tstep) != rd_sum_tstep_get_report(tstep))
        goto exit;

    if (rd_sum_tstep_get_sim_days(prev_tstep) >=
        rd_sum_tstep_get_sim_days(tstep))
        goto exit;

    this->index.add(rd_sum_tstep_get_sim_time(tstep), sim_seconds, report_step);
    rebuild_index = false;

exit:
    if (rebuild_index)
        this->build_index();

    return tstep;
}

rd_sum_tstep_type *rd_sum_file_data::iget_ministep(int internal_index) const {
    return (rd_sum_tstep_type *)vector_iget(data, internal_index);
}

double rd_sum_file_data::iget_sim_days(int time_index) const {
    const auto &node = this->index[time_index];
    return node.sim_seconds / 86400;
}

double rd_sum_file_data::iget_sim_seconds(int time_index) const {
    const auto &node = this->index[time_index];
    return node.sim_seconds;
}

static int cmp_ministep(const void *arg1, const void *arg2) {
    const rd_sum_tstep_type *ministep1 = rd_sum_tstep_safe_cast_const(arg1);
    const rd_sum_tstep_type *ministep2 = rd_sum_tstep_safe_cast_const(arg2);

    time_t time1 = rd_sum_tstep_get_sim_time(ministep1);
    time_t time2 = rd_sum_tstep_get_sim_time(ministep2);

    if (time1 < time2)
        return -1;
    else if (time1 == time2)
        return 0;
    else
        return 1;
}

static int first_step(const rd_smspec_type *rd_smspec) {
    if (rd_smspec->restart_step > 0)
        return rd_smspec->restart_step + 1;
    else
        return 1;
}

void rd_sum_file_data::build_index() {
    this->index.clear();

    if (this->loader) {
        int offset = first_step(this->rd_smspec) - 1;
        std::vector<int> report_steps = this->loader->report_steps(offset);
        std::vector<time_t> sim_time = this->loader->sim_time();
        std::vector<double> sim_seconds = this->loader->sim_seconds();

        for (int i = 0; i < this->loader->length(); i++) {
            this->index.add(sim_time[i], sim_seconds[i], report_steps[i]);
        }
    } else {
        vector_sort(data, cmp_ministep);
        for (int internal_index = 0; internal_index < vector_get_size(data);
             internal_index++) {
            const rd_sum_tstep_type *ministep = iget_ministep(internal_index);
            this->index.add(rd_sum_tstep_get_sim_time(ministep),
                            ministep->sim_seconds,
                            rd_sum_tstep_get_report(ministep));
        }
    }
}

void rd_sum_file_data::get_time(int length, time_t *data) {
    for (int time_index = 0; time_index < length; time_index++)
        data[time_index] = this->iget_sim_time(time_index);
}

int rd_sum_file_data::get_time_report(int end_index, time_t *data) {
    int offset = 0;

    for (int report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        const auto &range = this->report_range(report_step);
        int time_index = range.second;
        if (time_index >= end_index)
            break;

        data[offset] = this->iget_sim_time(time_index);

        offset += 1;
    }
    return offset;
}

void rd_sum_file_data::get_data(int params_index, int length, double *data) {
    if (this->loader) {
        const auto tmp_data = loader->get_vector(params_index);
        memcpy(data, tmp_data.data(), length * sizeof data);
    } else {
        for (int time_index = 0; time_index < length; time_index++)
            data[time_index] = this->iget(time_index, params_index);
    }
}

int rd_sum_file_data::get_data_report(int params_index, int end_index,
                                      double *data, double default_value) {
    int offset = 0;

    for (int report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        int time_index = this->index.report_range(report_step).second;
        if (time_index >= end_index)
            break;

        if (params_index >= 0)
            data[offset] = this->iget(time_index, params_index);
        else
            data[offset] = default_value;

        offset += 1;
    }
    return offset;
}

bool rd_sum_file_data::has_report(int report_step) const {
    return this->index.has_report(report_step);
}

std::pair<int, int> rd_sum_file_data::report_range(int report_step) const {
    return this->index.report_range(report_step);
}

static void rd_sum_tstep_fwrite(const rd_sum_tstep_type *ministep,
                                const int *index_map, int index_map_size,
                                fortio_type *fortio) {
    {
        rd_kw_type *ministep_kw = rd_kw_alloc(MINISTEP_KW, 1, RD_INT);
        rd_kw_iset_int(ministep_kw, 0, ministep->ministep);
        rd_kw_fwrite(ministep_kw, fortio);
        rd_kw_free(ministep_kw);
    }

    {
        int compact_size = index_map_size;
        rd_kw_type *params_kw = rd_kw_alloc(PARAMS_KW, compact_size, RD_FLOAT);

        float *data = (float *)rd_kw_get_ptr(params_kw);

        {
            int i;
            for (i = 0; i < compact_size; i++)
                data[i] = ministep->data[index_map[i]];
        }
        rd_kw_fwrite(params_kw, fortio);
        rd_kw_free(params_kw);
    }
}

void rd_sum_file_data::fwrite_report(int report_step,
                                     fortio_type *fortio) const {
    {
        rd_kw_type *seqhdr_kw = rd_kw_alloc(SEQHDR_KW, SEQHDR_SIZE, RD_INT);
        rd_kw_iset_int(seqhdr_kw, 0, 0);
        rd_kw_fwrite(seqhdr_kw, fortio);
        rd_kw_free(seqhdr_kw);
    }

    {
        auto range = this->report_range(report_step);
        for (int index = range.first; index <= range.second; index++) {
            const rd_sum_tstep_type *tstep = iget_ministep(index);
            rd_sum_tstep_fwrite(tstep, rd_smspec->index_map.data(),
                                rd_smspec_num_nodes(rd_smspec), fortio);
        }
    }
}

void rd_sum_file_data::fwrite_unified(fortio_type *fortio) const {
    if (this->length() == 0)
        return;

    for (int report_step = first_report(); report_step <= last_report();
         report_step++) {
        if (has_report(report_step))
            fwrite_report(report_step, fortio);
    }
}

void rd_sum_file_data::fwrite_multiple(const std::string &rd_case,
                                       bool fmt_case) const {
    if (this->length() == 0)
        return;

    for (int report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        if (this->has_report(report_step)) {
            fs::path filename =
                rd::filename(rd_case, RD_SUMMARY_FILE, fmt_case, report_step);
            ERT::FortIO fortio(filename.string(), std::ios_base::out, fmt_case);

            fwrite_report(report_step, fortio.get());
        }
    }
}

bool rd_sum_file_data::can_write() const {
    if (this->loader)
        return false;

    return true;
}

double rd_sum_file_data::get_days_start() const {
    const auto &node = this->index[0];
    return node.sim_seconds * 86400;
}

bool rd_sum_file_data::check_file(rd_file_type *rd_file) {
    return rd_file_has_kw(rd_file, PARAMS_KW) &&
           (rd_file_get_num_named_kw(rd_file, PARAMS_KW) ==
            rd_file_get_num_named_kw(rd_file, MINISTEP_KW));
}

/**
   If the rd_kw instance is in some way invalid (i.e. wrong size);
   the function will return NULL:
*/
rd_sum_tstep_type *rd_sum_tstep_alloc_from_file(int report_step,
                                                int ministep_nr,
                                                const rd_kw_type *params_kw,
                                                const char *src_file,
                                                const rd_smspec_type *smspec) {

    int data_size = rd_kw_get_size(params_kw);

    if (data_size == smspec->params_size) {
        rd_sum_tstep_type *ministep =
            rd_sum_tstep_alloc(report_step, ministep_nr, smspec);
        rd_kw_get_memcpy_data(params_kw, ministep->data.data());
        rd_sum_tstep_set_time_info(ministep, smspec);
        return ministep;
    } else {
        /*
       This is actually a fatal error / bug; the difference in smspec
       header structure should have been detected already in the
       rd_smspec_load_restart() function and the restart case
       discarded.
    */
        fprintf(stderr,
                "** Warning size mismatch between timestep loaded from:%s(%d) "
                "and header:%s(%d) - timestep discarded.\n",
                src_file, data_size, smspec->header_file.c_str(),
                smspec->params_size);
        return NULL;
    }
}

/**
   Malformed/incomplete files:
   ----------------------------
   Observe that ECLIPSE works in the following way:

     1. At the start of a report step a summary data section
        containing only the 'SEQHDR' keyword is written - this is
        currently an 'invalid' summary section.

     2. ECLIPSE simulates.

     3. When the time step is complete data is written to the summary
        file.

   Now - if ECLIPSE goes down during step 2 a malformed
   summary file will be left around, to handle this situation
   reasonably gracefully we check that the rd_file instance has at
   least one "PARAMS" keyword.

   One rd_file corresponds to one report_step (limited by SEQHDR); in
   the case of non unfied summary files these objects correspond to
   one BASE.Annnn or BASE.Snnnn file, in the case of unified files the
   calling routine will read the unified summary file partly.
*/

void rd_sum_file_data::add_rd_file(int report_step,
                                   const rd_file_view_type *summary_view) {

    int num_ministep = rd_file_view_get_num_named_kw(summary_view, PARAMS_KW);
    if (num_ministep > 0) {
        int ikw;

        for (ikw = 0; ikw < num_ministep; ikw++) {
            rd_kw_type *ministep_kw =
                rd_file_view_iget_named_kw(summary_view, MINISTEP_KW, ikw);
            rd_kw_type *params_kw =
                rd_file_view_iget_named_kw(summary_view, PARAMS_KW, ikw);

            {
                int ministep_nr = rd_kw_iget_int(ministep_kw, 0);
                rd_sum_tstep_type *tstep = rd_sum_tstep_alloc_from_file(
                    report_step, ministep_nr, params_kw,
                    rd_file_view_get_src_file(summary_view), this->rd_smspec);

                if (tstep)
                    append_tstep(tstep);
            }
        }
    }
}

bool rd_sum_file_data::fread(const stringlist_type *filelist, bool lazy_load,
                             int file_options) {
    if (stringlist_get_size(filelist) == 0)
        return false;

    rd_file_enum file_type =
        rd_get_file_type(stringlist_iget(filelist, 0), NULL, NULL);
    if ((stringlist_get_size(filelist) > 1) && (file_type != RD_SUMMARY_FILE))
        util_abort("%s: internal error - when calling with more than one file "
                   "- you can not supply a unified file - come on?! \n",
                   __func__);

    if (file_type == RD_SUMMARY_FILE) {

        /* Not unified. */
        for (int filenr = 0; filenr < stringlist_get_size(filelist); filenr++) {
            const char *data_file = stringlist_iget(filelist, filenr);
            rd_file_enum file_type;
            int report_step;
            file_type = rd_get_file_type(data_file, NULL, &report_step);
            if (file_type != RD_SUMMARY_FILE)
                util_abort("%s: file:%s has wrong type \n", __func__,
                           data_file);
            {
                rd_file_type *rd_file = rd_file_open(data_file, 0);
                if (rd_file && check_file(rd_file)) {
                    this->add_rd_file(report_step,
                                      rd_file_get_global_view(rd_file));
                    rd_file_close(rd_file);
                }
            }
        }
    } else if (file_type == RD_UNIFIED_SUMMARY_FILE) {
        if (lazy_load) {
            try {
                this->loader.reset(new unsmry_loader(
                    this->rd_smspec, stringlist_iget(filelist, 0),
                    file_options));
            } catch (const std::bad_alloc &e) {
                return false;
            }
        } else {

            // Is this correct for a restarted chain of UNSMRY files? Looks like the
            // report step sequence will be restarted?
            rd_file_type *rd_file =
                rd_file_open(stringlist_iget(filelist, 0), 0);
            if (rd_file && check_file(rd_file)) {
                int first_report_step = first_step(this->rd_smspec);
                int block_index = 0;
                while (true) {
                    /*
            Observe that there is a number discrepancy between ECLIPSE
            and the rd_file_select_smryblock() function. ECLIPSE
            starts counting report steps at 1; whereas the first
            SEQHDR block in the unified summary file is block zero (in
            ert counting).
        */
                    rd_file_view_type *summary_view =
                        rd_file_get_summary_view(rd_file, block_index);
                    if (summary_view) {
                        this->add_rd_file(block_index + first_report_step,
                                          summary_view);
                        block_index++;
                    } else
                        break;
                }
                rd_file_close(rd_file);
            }
        }
    }

    build_index();
    return (length() > 0);
}

const rd_smspec_type *rd_sum_file_data::smspec() const {
    return this->rd_smspec;
}

#define INVALID_MINISTEP_NR -1
#define INVALID_TIME_T 0

bool rd_sum_file_data::report_step_equal(const rd_sum_file_data &other,
                                         bool strict) const {
    if (strict && this->first_report() != other.first_report())
        return false;

    if (strict && (this->last_report() != other.last_report()))
        return false;

    int report_step = std::max(this->first_report(), other.first_report());
    int last_report = std::min(this->last_report(), other.last_report());
    while (true) {
        int time_index1 = this->report_range(report_step).second;
        int time_index2 = other.report_range(report_step).second;

        if ((time_index1 != INVALID_MINISTEP_NR) &&
            (time_index2 != INVALID_MINISTEP_NR)) {
            time_t time1 = this->iget_sim_time(time_index1);
            time_t time2 = other.iget_sim_time(time_index2);

            if (time1 != time2)
                return false;

        } else if (time_index1 != time_index2) {
            if (strict)
                return false;
        }

        report_step++;
        if (report_step > last_report)
            break;
    }
    return true;
}

int rd_sum_file_data::report_step_from_days(double sim_days) const {
    int report_step = this->first_report();
    double sim_seconds = sim_days * 86400;
    while (true) {
        const auto &range = this->index.report_range(report_step);
        if (range.second >= 0) {
            const auto &node = this->index[range.second];

            // Warning - this is a double == comparison!
            if (sim_seconds == node.sim_seconds)
                return report_step;

            report_step++;
            if (report_step > this->last_report())
                return -1;
        }
    }
}

int rd_sum_file_data::report_step_from_time(time_t sim_time) const {
    int report_step = this->first_report();
    while (true) {
        const auto &range = this->index.report_range(report_step);
        if (range.second >= 0) {
            const auto &node = this->index[range.second];
            if (sim_time == node.sim_time)
                return report_step;

            report_step++;
            if (report_step > this->last_report())
                return -1;
        }
    }
}

int rd_sum_file_data::iget_report(int time_index) const {
    const auto &index_node = this->index[time_index];
    return index_node.report_step;
}

} // namespace rd

/**
About indexing:
---------------

The RDISPE summary files are organised (roughly) like this:

 1. A header-file called xxx.SMPSEC is written, which is common to
    every timestep.

 2. For each timestep the summary vector is written in the form of a
    vector 'PARAMS' of length N with floats. In the PARAMS vector all
    types of data are stacked togeheter, and one must use the header
    info in the SMSPEC file to disentangle the summary data.

Here I will try to describe how the header in SMSPEC is organised, and
how that support is imlemented here. The SMSPEC header is organized
around three character vectors, of length N. To find the position in
the PARAMS vector of a certain quantity, you must consult one, two or
all three of these vectors. The most important vecor - which must
always be consulted is the KEYWORDS vector, then it is the WGNAMES and
NUMS (integer) vectors whcih must be consulted for some variable
types.


Let us a consider a system consisting of:

  * Two wells: P1 and P2 - for each well we have variables WOPR, WWCT
    and WGOR.

  * Three regions: For each region we have variables RPR and RXX(??)

  * We have stored field properties FOPT and FWPT


KEYWORDS = ['TIME','FOPR','FPR','FWCT','WOPR','WOPR,'WWCT','WWCT]
       ....



general_var:
------------
VAR_TYPE:(WELL_NAME|GROUP_NAME|NUMBER):NUMBER

Field var:         VAR_TYPE
Misc var:          VAR_TYPE
Well var:          VAR_TYPE:WELL_NAME
Group var:         VAR_TYPE:GROUP_NAME
Block var:         VAR_TYPE:i,j,k  (where i,j,k is calculated form NUM)
Region var         VAR_TYPE:index  (where index is NOT from the nums vector, it it is just an offset).
Completion var:    VAR_TYPE:WELL_NAME:NUM
....

*/

/*
  The smspec_required_keywords variable contains a list of keywords
  which are *absolutely* required in the SMSPEC file, but observe that
  depending on the content of the "KEYWORDS" array other keywords
  might bre requred as well - this typically includes the NUMS
  keyword. Such 'second-order' dependencies are not accounted for with
  this simple list.
*/

static const size_t num_req_keywords = 5;
static const char *smspec_required_keywords[] = {
    WGNAMES_KW, KEYWORDS_KW, STARTDAT_KW, UNITS_KW, DIMENS_KW};

namespace {

const rd::smspec_node *rd_smspec_get_var_node(const node_map &mp,
                                              const char *var) {
    const auto it = mp.find(var);
    if (it == mp.end())
        return nullptr;

    return it->second;
}

} //end namespace

int rd_smspec_num_nodes(const rd_smspec_type *smspec) {
    return smspec->smspec_nodes.size();
}

rd_smspec_var_type rd_smspec_identify_var_type(const char *var) {
    return rd::smspec_node::identify_var_type(var);
}

/*
  When loading a summary case from file many of the nodes can be ignored, in
  that case the size of PARAMS vector in the data files is larger than the
  number of internalized nodes. Therefor we need to maintain the
  params_size member.
*/

static rd_smspec_ptr rd_smspec_alloc_empty(bool write_mode,
                                           const std::string &key_join_string) {
    rd_smspec_ptr rd_smspec{new rd_smspec_type(), &rd_smspec_free};
    UTIL_TYPE_ID_INIT(rd_smspec.get(), RD_SMSPEC_ID);

    rd_smspec->sim_start_time = -1;
    rd_smspec->key_join_string = key_join_string;
    rd_smspec->header_file = "";

    rd_smspec->time_index = -1;
    rd_smspec->day_index = -1;
    rd_smspec->year_index = -1;
    rd_smspec->month_index = -1;
    rd_smspec->time_seconds = -1;
    rd_smspec->params_size = -1;

    /*
    The unit system is given as an integer in the INTEHEAD keyword. The INTEHEAD
    keyword is optional, and we have for a long time been completely oblivious
    to the possibility of extracting unit system information from the SMSPEC file.
  */
    rd_smspec->unit_system = RD_METRIC_UNITS;

    rd_smspec->restart_step = -1;
    rd_smspec->write_mode = write_mode;
    rd_smspec->need_nums = false;

    return rd_smspec;
}

/* There is a quite wide range of error which are just returned as
   "Not found" (i.e. -1). */
/* Completions not supported yet. */

static const rd::smspec_node &
rd_smspec_get_general_var_node(const rd_smspec_type *smspec,
                               const char *lookup_kw) {
    const auto node_ptr =
        rd_smspec_get_var_node(smspec->gen_var_index, lookup_kw);
    if (!node_ptr)
        throw std::out_of_range("No such variable: " + std::string(lookup_kw));

    return *node_ptr;
}

/**
   Observe that the index here is into the __INTERNAL__ indexing in
   the smspec_nodes vector; and in general widely different from the
   params_index of the returned smspec_node instance.
*/

const rd::smspec_node &
rd_smspec_iget_node_w_node_index(const rd_smspec_type *smspec, int node_index) {
    const auto &node = smspec->smspec_nodes[node_index];
    return *node.get();
}

const rd::smspec_node &
rd_smspec_iget_node_w_params_index(const rd_smspec_type *smspec,
                                   int params_index) {
    int node_index = smspec->inv_index_map.at(params_index);
    return rd_smspec_iget_node_w_node_index(smspec, node_index);
}

/**
 * Returns an ecl data type for which all names will fit. If the maximum name
 * length is at most 8, an RD_CHAR is returned and otherwise a large enough
 * RD_STRING.
 */
static rd_data_type get_wgnames_type(const rd_smspec_type *smspec) {
    size_t max_len = 0;
    for (int i = 0; i < rd_smspec_num_nodes(smspec); ++i) {
        const rd::smspec_node &node =
            rd_smspec_iget_node_w_node_index(smspec, i);
        const char *name = smspec_node_get_wgname(&node);
        if (name)
            max_len = util_size_t_max(max_len, strlen(name));
    }

    return max_len <= RD_STRING8_LENGTH ? RD_CHAR : RD_STRING(max_len);
}

static void rd_smspec_fwrite_INTEHEAD(const rd_smspec_type *smspec,
                                      fortio_type *fortio) {
    rd_kw_ptr intehead = make_rd_kw(INTEHEAD_KW, INTEHEAD_SMSPEC_SIZE, RD_INT);
    rd_kw_iset_int(intehead.get(), INTEHEAD_SMSPEC_UNIT_INDEX,
                   smspec->unit_system);
    /* The simulator type is just hardcoded to ECLIPSE100. */
    rd_kw_iset_int(intehead.get(), INTEHEAD_SMSPEC_IPROG_INDEX,
                   INTEHEAD_ECLIPSE100_VALUE);
    rd_kw_fwrite(intehead.get(), fortio);
}

static void rd_smspec_fwrite_RESTART(const rd_smspec_type *smspec,
                                     fortio_type *fortio) {
    rd_kw_ptr restart_kw =
        make_rd_kw(RESTART_KW, SUMMARY_RESTART_SIZE, RD_CHAR);
    for (int i = 0; i < SUMMARY_RESTART_SIZE; i++)
        rd_kw_iset_string8(restart_kw.get(), i, "");

    if (smspec->restart_case.size() > 0) {
        size_t restart_case_len = smspec->restart_case.size();

        size_t offset = 0;
        for (size_t i = 0; i < SUMMARY_RESTART_SIZE; i++) {
            if (offset < restart_case_len)
                rd_kw_iset_string8(restart_kw.get(), i,
                                   &smspec->restart_case[offset]);
            offset += RD_STRING8_LENGTH;
        }
    }
    rd_kw_fwrite(restart_kw.get(), fortio);
}

static void rd_smspec_fwrite_DIMENS(const rd_smspec_type *smspec,
                                    fortio_type *fortio) {
    rd_kw_ptr dimens_kw = make_rd_kw(DIMENS_KW, DIMENS_SIZE, RD_INT);
    int num_nodes = rd_smspec_num_nodes(smspec);
    rd_kw_iset_int(dimens_kw.get(), DIMENS_SMSPEC_SIZE_INDEX, num_nodes);
    rd_kw_iset_int(dimens_kw.get(), DIMENS_SMSPEC_NX_INDEX,
                   smspec->grid_dims[0]);
    rd_kw_iset_int(dimens_kw.get(), DIMENS_SMSPEC_NY_INDEX,
                   smspec->grid_dims[1]);
    rd_kw_iset_int(dimens_kw.get(), DIMENS_SMSPEC_NZ_INDEX,
                   smspec->grid_dims[2]);
    rd_kw_iset_int(dimens_kw.get(), 4, 0); // Do not know what this is for.
    rd_kw_iset_int(dimens_kw.get(), DIMENS_SMSPEC_RESTART_STEP_INDEX,
                   smspec->restart_step);

    rd_kw_fwrite(dimens_kw.get(), fortio);
}

static void rd_smspec_fwrite_STARTDAT(const rd_smspec_type *smspec,
                                      fortio_type *fortio) {
    auto startdat_kw = make_rd_kw(STARTDAT_KW, STARTDAT_SIZE, RD_INT);
    int second, minute, hour, mday, month, year;
    rd_set_datetime_values(smspec->sim_start_time, &second, &minute, &hour,
                           &mday, &month, &year);

    rd_kw_iset_int(startdat_kw.get(), STARTDAT_DAY_INDEX, mday);
    rd_kw_iset_int(startdat_kw.get(), STARTDAT_MONTH_INDEX, month);
    rd_kw_iset_int(startdat_kw.get(), STARTDAT_YEAR_INDEX, year);
    rd_kw_iset_int(startdat_kw.get(), STARTDAT_HOUR_INDEX, hour);
    rd_kw_iset_int(startdat_kw.get(), STARTDAT_MINUTE_INDEX, minute);
    rd_kw_iset_int(startdat_kw.get(), STARTDAT_MICRO_SECOND_INDEX,
                   second * 1000000);

    rd_kw_fwrite(startdat_kw.get(), fortio);
}

static void rd_smspec_fortio_fwrite(const rd_smspec_type *smspec,
                                    fortio_type *fortio) {
    rd_smspec_fwrite_INTEHEAD(smspec, fortio);
    rd_smspec_fwrite_RESTART(smspec, fortio);
    rd_smspec_fwrite_DIMENS(smspec, fortio);

    int num_nodes = rd_smspec_num_nodes(smspec);
    auto keywords_kw = make_rd_kw(KEYWORDS_KW, num_nodes, RD_CHAR);
    auto units_kw = make_rd_kw(UNITS_KW, num_nodes, RD_CHAR);
    rd_kw_ptr nums_kw{nullptr, &rd_kw_free};

    // If the names_type is an RD_STRING we expect this to be an INTERSECT
    // summary, otherwise an ECLIPSE summary.
    rd_data_type names_type = get_wgnames_type(smspec);
    auto wgnames_kw =
        make_rd_kw(rd_type_is_char(names_type) ? WGNAMES_KW : NAMES_KW,
                   num_nodes, names_type);

    if (smspec->need_nums)
        nums_kw.reset(rd_kw_alloc(NUMS_KW, num_nodes, RD_INT));

    for (int i = 0; i < rd_smspec_num_nodes(smspec); i++) {
        const rd::smspec_node &smspec_node =
            rd_smspec_iget_node_w_node_index(smspec, i);
        /*
      It is possible to add variables with deferred initialisation
      with the rd_sum_add_blank_var() function. Before these
      variables can be actually used for anything interesting they
      must be initialized with the rd_sum_init_var() function.

      If a call to save the smspec file comes before all the
      variable have been initialized things will potentially go
      belly up. This is solved with the following uber-hack:

      o One of the well related keywords is chosen; in
        particular 'WWCT' in this case.

      o The wgname value is set to DUMMY_WELL

      The use of DUMMY_WELL ensures that this field will be
      ignored when/if this smspec file is read in at a later
      stage.
    */
        if (smspec_node.get_var_type() == RD_SMSPEC_INVALID_VAR) {
            rd_kw_iset_string8(keywords_kw.get(), i, "WWCT");
            rd_kw_iset_string8(units_kw.get(), i, "????????");
            rd_kw_iset_string_ptr(wgnames_kw.get(), i, DUMMY_WELL);
        } else {
            rd_kw_iset_string8(keywords_kw.get(), i,
                               smspec_node_get_keyword(&smspec_node));
            rd_kw_iset_string8(units_kw.get(), i,
                               smspec_node_get_unit(&smspec_node));
            {
                const char *wgname = DUMMY_WELL;
                if (smspec_node_get_wgname(&smspec_node))
                    wgname = smspec_node_get_wgname(&smspec_node);
                rd_kw_iset_string_ptr(wgnames_kw.get(), i, wgname);
            }
        }

        if (nums_kw)
            rd_kw_iset_int(nums_kw.get(), i, smspec_node.get_num());
    }
    rd_kw_fwrite(keywords_kw.get(), fortio);
    rd_kw_fwrite(wgnames_kw.get(), fortio);
    if (nums_kw)
        rd_kw_fwrite(nums_kw.get(), fortio);
    rd_kw_fwrite(units_kw.get(), fortio);

    rd_smspec_fwrite_STARTDAT(smspec, fortio);
}

static void rd_smspec_fwrite(const rd_smspec_type *smspec, const char *rd_case,
                             bool fmt_file) {
    std::string filename =
        rd::filename(rd_case, RD_SUMMARY_HEADER_FILE, fmt_file, 0).string();
    ERT::FortIO fortio(filename, std::ios_base::out, fmt_file);

    if (!fortio.get())
        throw std::runtime_error(
            fmt::format("Unable to open fortio file {}, error: {}", filename,
                        strerror(errno)));

    rd_smspec_fortio_fwrite(smspec, fortio.get());
}

static rd_smspec_type *
rd_smspec_alloc_writer__(const char *key_join_string, const char *restart_case,
                         int restart_step, time_t sim_start, bool time_in_days,
                         int nx, int ny, int nz) {
    rd_smspec_ptr rd_smspec = rd_smspec_alloc_empty(true, key_join_string);
    /*
    Only a total of 9 * 8 characters is set aside for the restart keyword, if
    the supplied restart case is longer than that we silently ignore it.
  */
    if (restart_case) {
        if (strlen(restart_case) <=
            (SUMMARY_RESTART_SIZE * RD_STRING8_LENGTH)) {
            rd_smspec->restart_case = restart_case;
            rd_smspec->restart_step = restart_step;
        }
    }
    rd_smspec->grid_dims[0] = nx;
    rd_smspec->grid_dims[1] = ny;
    rd_smspec->grid_dims[2] = nz;
    rd_smspec->sim_start_time = sim_start;

    {
        const rd::smspec_node *time_node;

        if (time_in_days) {
            rd_smspec->time_seconds = 3600 * 24;
            time_node = rd_smspec_add_node(rd_smspec.get(), "TIME", "DAYS", 0);
        } else {
            rd_smspec->time_seconds = 3600;
            time_node = rd_smspec_add_node(rd_smspec.get(), "TIME", "HOURS", 0);
        }
        rd_smspec->time_index = time_node->get_params_index();
    }
    return rd_smspec.release();
}

rd_smspec_type *rd_smspec_alloc_restart_writer(
    const char *key_join_string, const char *restart_case, int restart_step,
    time_t sim_start, bool time_in_days, int nx, int ny, int nz) {
    return rd_smspec_alloc_writer__(key_join_string, restart_case, restart_step,
                                    sim_start, time_in_days, nx, ny, nz);
}

rd_smspec_type *rd_smspec_alloc_writer(const char *key_join_string,
                                       time_t sim_start, bool time_in_days,
                                       int nx, int ny, int nz) {
    return rd_smspec_alloc_writer__(key_join_string, NULL, 0, sim_start,
                                    time_in_days, nx, ny, nz);
}

UTIL_SAFE_CAST_FUNCTION(rd_smspec, RD_SMSPEC_ID)

static bool rd_smspec_lgr_var_type(rd_smspec_var_type var_type) {
    if ((var_type == RD_SMSPEC_LOCAL_BLOCK_VAR) ||
        (var_type == RD_SMSPEC_LOCAL_WELL_VAR) ||
        (var_type == RD_SMSPEC_LOCAL_COMPLETION_VAR))

        return true;
    else
        return false;
}

/**
   This function takes a fully initialized smspec_node instance, generates the
   corresponding key and inserts smspec_node instance in the main hash table
   smspec->gen_var_index.

   The format strings used, i.e. VAR:WELL for well based variables is implicitly
   defined through the format strings used in this function.
*/

static void rd_smspec_install_gen_keys(rd_smspec_type *smspec,
                                       const rd::smspec_node &smspec_node) {
    /* Insert the default general mapping. */
    {
        const char *gen_key1 = smspec_node.get_gen_key1();
        if (gen_key1)
            smspec->gen_var_index[gen_key1] = &smspec_node;
    }

    /* Insert the (optional) extra mapping for block related variables and region_2_region variables: */
    {
        const char *gen_key2 = smspec_node.get_gen_key2();
        if (gen_key2)
            smspec->gen_var_index[gen_key2] = &smspec_node;
    }
}

static void rd_smspec_install_special_keys(rd_smspec_type *rd_smspec,
                                           const rd::smspec_node &smspec_node) {
    /**
      This large switch is for installing keys which have custom lookup
      paths, in addition to the lookup based on general keys. Examples
      of this is e.g. well variables which can be looked up through:

      rd_smspec_get_well_var_index( smspec , well_name , var );
  */

    const char *well = smspec_node_get_wgname(&smspec_node);
    const char *group = well;
    const int num = smspec_node_get_num(&smspec_node);
    const char *keyword = smspec_node_get_keyword(&smspec_node);
    rd_smspec_var_type var_type = smspec_node_get_var_type(&smspec_node);

    switch (var_type) {
    case (RD_SMSPEC_COMPLETION_VAR):
        rd_smspec->well_completion_var_index[well][num][keyword] = &smspec_node;
        break;
    case (RD_SMSPEC_FIELD_VAR):
        rd_smspec->field_var_index[keyword] = &smspec_node;
        break;
    case (RD_SMSPEC_GROUP_VAR):
        rd_smspec->group_var_index[group][keyword] = &smspec_node;
        break;
    case (RD_SMSPEC_REGION_VAR):
        rd_smspec->region_var_index[num][keyword] = &smspec_node;
        rd_smspec->num_regions = util_int_max(rd_smspec->num_regions, num);
        break;
    case (RD_SMSPEC_WELL_VAR):
        rd_smspec->well_var_index[well][keyword] = &smspec_node;
        break;
    case (RD_SMSPEC_MISC_VAR):
        /* Misc variable - i.e. date or CPU time ... */
        rd_smspec->misc_var_index[keyword] = &smspec_node;
        break;
    case (RD_SMSPEC_BLOCK_VAR):
        rd_smspec->block_var_index[num][keyword] = &smspec_node;
        break;
        /**
        The variables below are ONLY accesable through the gen_key
        setup; but the must be mentioned in this switch statement,
        otherwise they will induce a hard failure in the default: target
        below.
    */
    case (RD_SMSPEC_LOCAL_BLOCK_VAR):
        break;
    case (RD_SMSPEC_LOCAL_COMPLETION_VAR):
        break;
    case (RD_SMSPEC_LOCAL_WELL_VAR):
        break;
    case (RD_SMSPEC_SEGMENT_VAR):
        break;
    case (RD_SMSPEC_REGION_2_REGION_VAR):
        break;
    case (RD_SMSPEC_AQUIFER_VAR):
        break;
    case (RD_SMSPEC_NETWORK_VAR):
        break;
    default:
        throw std::invalid_argument("Internal error - should not be here \n");
    }
}

bool rd_smspec_equal(const rd_smspec_type *self, const rd_smspec_type *other) {
    if (self->smspec_nodes.size() != other->smspec_nodes.size())
        return false;

    for (size_t i = 0; i < self->smspec_nodes.size(); i++) {
        const rd::smspec_node *node1 = self->smspec_nodes[i].get();
        const rd::smspec_node *node2 = other->smspec_nodes[i].get();

        if (node1->cmp(*node2) != 0)
            return false;
    }

    return true;
}

static void rd_smspec_load_restart(rd_smspec_type *rd_smspec,
                                   const rd_file_type *header) {
    if (!rd_file_has_kw(header, RESTART_KW))
        return;
    const rd_kw_type *restart_kw = rd_file_iget_named_kw(header, RESTART_KW, 0);
    int num_blocks = rd_kw_get_size(restart_kw);
    num_blocks = (num_blocks < 0) ? 0 : num_blocks;
    auto tmp_base = rd::checked_calloc<char>(8 * num_blocks + 1);
    for (int i = 0; i < num_blocks; i++) {
        const char *part = (const char *)rd_kw_iget_ptr(restart_kw, i);
        strncat(tmp_base.get(), part, 8);
    }

    std::string restart_base = rd::strip_spaces(std::string(tmp_base.get()));

    /* We ignore the empty ones. */
    if (!restart_base.size())
        return;

    std::string smspec_header;
    fs::path dir = fs::path(rd_smspec->header_file).parent_path();

    /*
    The conditional block here is to support the following situation:

    1. A simulation with a restart has been performed on Posix with path
       separator '/'.

    2. The simulation is loaded on windows, where the native path
       separator is '\'.

    This code block will translate '/' -> '\' in the restart keyword which
    is read from the summary file.
    */

#ifdef ERT_WINDOWS
    for (int i = 0; i < restart_base.size(); i++) {
        if (restart_base[i] == UTIL_POSIX_PATH_SEP_CHAR)
            restart_base[i] = UTIL_PATH_SEP_CHAR;
    }
#endif

    fs::path restart_path(restart_base);
    if (restart_path.is_relative()) {
        restart_path = dir / restart_path;
    }
    smspec_header = rd::filename(restart_path, RD_SUMMARY_HEADER_FILE,
                                 rd_smspec->formatted, 0)
                        .string();

    std::error_code ec;
    if (!fs::exists(smspec_header, ec) || ec)
        return;

    // Restart from current case is ignored
    if (util_same_file(smspec_header.c_str(), rd_smspec->header_file.c_str()))
        return;

    // restart_path should not exist, but we are checking
    // anyways to keep existing behavior
    if (fs::exists(restart_path, ec) && !ec)
        restart_path = fs::canonical(restart_path);
    rd_smspec->restart_case = restart_path.string();
}

static const rd::smspec_node *
rd_smspec_insert_node(rd_smspec_type *rd_smspec,
                      std::unique_ptr<rd::smspec_node> smspec_node) {
    int params_index = smspec_node->get_params_index();

    /* This indexing must be used when writing. */
    rd_smspec->index_map.push_back(params_index);
    rd_smspec->params_default.resize(params_index + 1, PARAMS_GLOBAL_DEFAULT);
    rd_smspec->params_default[params_index] = smspec_node->get_default();
    rd_smspec->inv_index_map.insert(
        std::make_pair(params_index, rd_smspec->smspec_nodes.size()));

    rd_smspec_install_gen_keys(rd_smspec, *smspec_node.get());
    rd_smspec_install_special_keys(rd_smspec, *smspec_node.get());

    if (smspec_node->need_nums())
        rd_smspec->need_nums = true;

    rd_smspec->smspec_nodes.push_back(std::move(smspec_node));

    if (params_index > rd_smspec->params_size)
        rd_smspec->params_size = params_index + 1;

    if (static_cast<int>(rd_smspec->smspec_nodes.size()) >
        rd_smspec->params_size)
        rd_smspec->params_size = rd_smspec->smspec_nodes.size();

    const auto &node = rd_smspec->smspec_nodes.back();
    return node.get();
}

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword, int num,
                                          const char *unit,
                                          float default_value) {
    int params_index = rd_smspec->smspec_nodes.size();
    return rd_smspec_insert_node(
        rd_smspec, std::unique_ptr<rd::smspec_node>(new rd::smspec_node(
                       params_index, keyword, num, unit, rd_smspec->grid_dims,
                       default_value, rd_smspec->key_join_string.c_str())));
}

//copy given node with a new index
const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const rd::smspec_node &node) {
    int params_index = rd_smspec->smspec_nodes.size();
    return rd_smspec_insert_node(rd_smspec,
                                 std::unique_ptr<rd::smspec_node>(
                                     new rd::smspec_node(node, params_index)));
}

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword, const char *unit,
                                          float default_value) {
    int params_index = rd_smspec->smspec_nodes.size();
    return rd_smspec_insert_node(
        rd_smspec, std::unique_ptr<rd::smspec_node>(new rd::smspec_node(
                       params_index, keyword, unit, default_value)));
}

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword,
                                          const char *wgname, const char *unit,
                                          float default_value) {
    int params_index = rd_smspec->smspec_nodes.size();
    return rd_smspec_insert_node(
        rd_smspec, std::unique_ptr<rd::smspec_node>(new rd::smspec_node(
                       params_index, keyword, wgname, unit, default_value,
                       rd_smspec->key_join_string.c_str())));
}

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword,
                                          const char *wgname, int num,
                                          const char *unit,
                                          float default_value) {
    int params_index = rd_smspec->smspec_nodes.size();
    return rd_smspec_insert_node(
        rd_smspec,
        std::unique_ptr<rd::smspec_node>(new rd::smspec_node(
            params_index, keyword, wgname, num, unit, rd_smspec->grid_dims,
            default_value, rd_smspec->key_join_string.c_str())));
}

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          int params_index, const char *keyword,
                                          const char *wgname, int num,
                                          const char *unit,
                                          float default_value) {
    return rd_smspec_insert_node(
        rd_smspec,
        std::unique_ptr<rd::smspec_node>(new rd::smspec_node(
            params_index, keyword, wgname, num, unit, rd_smspec->grid_dims,
            default_value, rd_smspec->key_join_string.c_str())));
}

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          int params_index, const char *keyword,
                                          const char *wgname, int num,
                                          const char *unit, const char *lgr,
                                          int lgr_i, int lgr_j, int lgr_k,
                                          float default_value) {
    return rd_smspec_insert_node(
        rd_smspec,
        std::unique_ptr<rd::smspec_node>(new rd::smspec_node(
            params_index, keyword, wgname, unit, lgr, lgr_i, lgr_j, lgr_k,
            default_value, rd_smspec->key_join_string.c_str())));
}

const int *rd_smspec_get_index_map(const rd_smspec_type *smspec) {
    return smspec->index_map.data();
}

/**
 * This function is to support the NAMES alias for WGNAMES. If similar
 * situations occur in the future, this is a sane starting point for general
 * support.
 */
static const char *get_active_keyword_alias(rd_file_type *header,
                                            const char *keyword) {
    if (strcmp(keyword, WGNAMES_KW) == 0 || strcmp(keyword, NAMES_KW) == 0)
        return rd_file_has_kw(header, WGNAMES_KW) ? WGNAMES_KW : NAMES_KW;

    return keyword;
}

static bool rd_smspec_check_header(rd_file_type *header) {
    bool OK = true;
    for (size_t i = 0; i < num_req_keywords && OK; i++) {
        OK &= rd_file_has_kw(header, get_active_keyword_alias(
                                         header, smspec_required_keywords[i]));
    }

    return OK;
}

static bool rd_smspec_fread_header(rd_smspec_type *rd_smspec,
                                   const std::string &header_file,
                                   bool include_restart) {
    rd_file_type *header = rd_file_open(header_file.c_str(), 0);
    if (header && rd_smspec_check_header(header)) {
        const char *names_alias = get_active_keyword_alias(header, WGNAMES_KW);
        rd_kw_type *wells = rd_file_iget_named_kw(header, names_alias, 0);
        rd_kw_type *keywords = rd_file_iget_named_kw(header, KEYWORDS_KW, 0);
        rd_kw_type *startdat = rd_file_iget_named_kw(header, STARTDAT_KW, 0);
        rd_kw_type *units = rd_file_iget_named_kw(header, UNITS_KW, 0);
        rd_kw_type *dimens = rd_file_iget_named_kw(header, DIMENS_KW, 0);
        rd_kw_type *nums = NULL;
        rd_kw_type *lgrs = NULL;
        rd_kw_type *numlx = NULL;
        rd_kw_type *numly = NULL;
        rd_kw_type *numlz = NULL;

        int params_index;
        rd_smspec->num_regions = 0;
        rd_smspec->params_size = rd_kw_get_size(keywords);
        if (startdat == NULL)
            throw std::invalid_argument(
                "Could not locate STARTDAT keyword in header");

        if (rd_file_has_kw(header, NUMS_KW))
            nums = rd_file_iget_named_kw(header, NUMS_KW, 0);

        if (rd_file_has_kw(header, INTEHEAD_KW)) {
            const rd_kw_type *intehead =
                rd_file_iget_named_kw(header, INTEHEAD_KW, 0);
            rd_smspec->unit_system = (ert_rd_unit_enum)rd_kw_iget_int(
                intehead, INTEHEAD_SMSPEC_UNIT_INDEX);
            /*
        The second item in the INTEHEAD vector is an integer designating which
        simulator has been used for the current simulation, that is currently
        ignored.
      */
        }

        if (rd_file_has_kw(header,
                           LGRS_KW)) { /* The file has LGR information. */
            lgrs = rd_file_iget_named_kw(header, LGRS_KW, 0);
            numlx = rd_file_iget_named_kw(header, NUMLX_KW, 0);
            numly = rd_file_iget_named_kw(header, NUMLY_KW, 0);
            numlz = rd_file_iget_named_kw(header, NUMLZ_KW, 0);
            rd_smspec->has_lgr = true;
        } else
            rd_smspec->has_lgr = false;

        {
            int *date = rd_kw_get_int_ptr(startdat);
            int year = date[STARTDAT_YEAR_INDEX];
            int month = date[STARTDAT_MONTH_INDEX];
            int day = date[STARTDAT_DAY_INDEX];
            int hour = 0;
            int min = 0;
            int sec = 0;
            if (rd_kw_get_size(startdat) == 6) {
                hour = date[STARTDAT_HOUR_INDEX];
                min = date[STARTDAT_MINUTE_INDEX];
                sec = date[STARTDAT_MICRO_SECOND_INDEX] / 1000000;
            }

            rd_smspec->sim_start_time =
                rd_make_datetime(sec, min, hour, day, month, year);
        }

        rd_smspec->grid_dims[0] =
            rd_kw_iget_int(dimens, DIMENS_SMSPEC_NX_INDEX);
        rd_smspec->grid_dims[1] =
            rd_kw_iget_int(dimens, DIMENS_SMSPEC_NY_INDEX);
        rd_smspec->grid_dims[2] =
            rd_kw_iget_int(dimens, DIMENS_SMSPEC_NZ_INDEX);
        rd_smspec->restart_step =
            rd_kw_iget_int(dimens, DIMENS_SMSPEC_RESTART_STEP_INDEX);

        rd_get_file_type(header_file.c_str(), &rd_smspec->formatted, NULL);

        {
            for (params_index = 0; params_index < rd_kw_get_size(wells);
                 params_index++) {
                float default_value = PARAMS_GLOBAL_DEFAULT;
                int num = SMSPEC_NUMS_INVALID;
                std::string well =
                    rd_kw_iget_stripped_string(wells, params_index);
                std::string kw =
                    rd_kw_iget_stripped_string(keywords, params_index);
                std::string unit =
                    rd_kw_iget_stripped_string(units, params_index);

                rd_smspec_var_type var_type;
                if (nums != NULL)
                    num = rd_kw_iget_int(nums, params_index);
                var_type =
                    rd::smspec_node::valid_type(kw.c_str(), well.c_str(), num);
                if (var_type == RD_SMSPEC_INVALID_VAR) {
                    continue;
                }

                if (rd_smspec_lgr_var_type(var_type)) {
                    int lgr_i = rd_kw_iget_int(numlx, params_index);
                    int lgr_j = rd_kw_iget_int(numly, params_index);
                    int lgr_k = rd_kw_iget_int(numlz, params_index);
                    std::string lgr_name =
                        rd_kw_iget_stripped_string(lgrs, params_index);

                    rd_smspec_insert_node(
                        rd_smspec,
                        std::make_unique<rd::smspec_node>(
                            params_index, kw.c_str(), well.c_str(),
                            unit.c_str(), lgr_name.c_str(), lgr_i, lgr_j, lgr_k,
                            default_value, rd_smspec->key_join_string.c_str()));
                } else
                    rd_smspec_insert_node(
                        rd_smspec,
                        std::make_unique<rd::smspec_node>(
                            params_index, kw.c_str(), well.c_str(), num,
                            unit.c_str(), rd_smspec->grid_dims, default_value,
                            rd_smspec->key_join_string.c_str()));
            }
        }

        rd_smspec->header_file = fs::canonical(header_file).string();
        if (include_restart)
            rd_smspec_load_restart(rd_smspec, header);

        rd_file_close(header);

        return true;
    } else
        return false;
}

rd_smspec_type *rd_smspec_fread_alloc(const std::string &header_file,
                                      const std::string &key_join_string,
                                      bool include_restart) {
    rd_smspec_ptr rd_smspec = rd_smspec_alloc_empty(false, key_join_string);

    if (rd_smspec_fread_header(rd_smspec.get(), header_file, include_restart)) {

        const rd::smspec_node *time_node =
            rd_smspec_get_var_node(rd_smspec->misc_var_index, "TIME");
        if (time_node) {
            const char *time_unit = time_node->get_unit();
            rd_smspec->time_index = time_node->get_params_index();

            if (util_string_equal(time_unit, "DAYS"))
                rd_smspec->time_seconds = 3600 * 24;
            else if (util_string_equal(time_unit, "HOURS"))
                rd_smspec->time_seconds = 3600;
            else
                throw std::invalid_argument(
                    fmt::format("time_unit:{} not recognized", time_unit));
        }

        const rd::smspec_node *day_node =
            rd_smspec_get_var_node(rd_smspec->misc_var_index, "DAY");
        const rd::smspec_node *month_node =
            rd_smspec_get_var_node(rd_smspec->misc_var_index, "MONTH");
        const rd::smspec_node *year_node =
            rd_smspec_get_var_node(rd_smspec->misc_var_index, "YEAR");

        if (day_node != NULL && month_node != NULL && year_node != NULL) {
            rd_smspec->day_index = day_node->get_params_index();
            rd_smspec->month_index = month_node->get_params_index();
            rd_smspec->year_index = year_node->get_params_index();
        }

        if ((rd_smspec->time_index == -1) && (rd_smspec->day_index == -1)) {
            // Unusable configuration.
            // Seems the restart file can also have time specified with
            // 'YEARS' as basic time unit; that mode is not supported.

            throw std::invalid_argument(
                "The SMSPEC file seems to lack all time information, need "
                "either TIME, or DAY/MONTH/YEAR information. Can not "
                "proceed.");
        }
        return rd_smspec.release();
    } else {
        /** Failed to load from disk. */
        return nullptr;
    }
}

/*
   For each type of summary data (according to the types in
   rd_smcspec_var_type there are a set accessor functions:

   xx_get_xx: This function will take the apropriate input, and
   return a double value. The function will fail
   if the rd_smspec object can not recognize the input. THis
   function is not here.

   xxx_has_xx: Ths will return true / false depending on whether the
   rd_smspec object the variable we ask for.

   xxx_get_xxx_index: This function will rerturn an (internal)
   integer index of where the variable in question is stored, this
   index can then be subsequently used for faster lookup. If the
   variable can not be found, the function will return -1.

   In general the index function is the real function, the others are
   only wrappers around this. In addition there are specialized
   functions, like get_well_names() and so on.
*/

namespace {

bool node_exists(const rd::smspec_node *node_ptr) {
    if (node_ptr)
        return true;

    return false;
}

int node_valid_index(const rd::smspec_node *node_ptr) {
    if (node_ptr)
        return node_ptr->get_params_index();

    throw std::out_of_range("Invalid lookup summary object");
}

} // namespace

static bool rd_smspec_has_general_var(const rd_smspec_type *rd_smspec,
                                      const char *lookup_kw) {
    const auto node_ptr =
        rd_smspec_get_var_node(rd_smspec->gen_var_index, lookup_kw);
    return node_exists(node_ptr);
}

static const char *rd_smspec_get_restart_case(const rd_smspec_type *rd_smspec) {
    if (rd_smspec->restart_case.size() > 0)
        return rd_smspec->restart_case.c_str();
    else
        return NULL;
}

const std::vector<float> &
rd_smspec_get_params_default(const rd_smspec_type *rd_smspec) {
    return rd_smspec->params_default;
}

void rd_smspec_free(rd_smspec_type *rd_smspec) { delete rd_smspec; }

/**
   Fills a stringlist instance with all the gen_key string matching
   the supplied pattern. I.e.

     rd_sum_alloc_matching_general_var_list( smspec , "WGOR:*");

   will give a list of WGOR for ALL the wells. The function is
   unfortunately not as useful as one might think because ECLIPSE
   will for instance happily give you the WOPR for a
   water injector or WWIR for an oil producer.

   The function can be called several times with different patterns,
   the stringlist is not cleared on startup; the keys in the list are
   unique - keys are not added multiple times. If pattern == NULL all
   keys will match.
*/
static void rd_smspec_select_matching_general_var_list(
    const rd_smspec_type *smspec, const char *pattern, stringlist_type *keys) {
    std::set<std::string> ex_keys;
    for (int i = 0; i < stringlist_get_size(keys); i++)
        ex_keys.insert(stringlist_iget(keys, i));

    {
        for (const auto &pair : smspec->gen_var_index) {
            const char *key = pair.first.c_str();

            /*
         The TIME is typically special cased by output and will not
         match the 'all keys' wildcard.
      */
            if (util_string_equal(key, "TIME")) {
                if ((pattern == NULL) || (util_string_equal(pattern, "*")))
                    continue;
            }

            if ((pattern == NULL) || (util_fnmatch(pattern, key) == 0)) {
                if (ex_keys.find(key) == ex_keys.end())
                    stringlist_append_copy(keys, key);
            }
        }
    }

    stringlist_sort(keys, (string_cmp_ftype *)util_strcmp_int);
}

/**
    Returns a stringlist instance with all the (valid) well names. It
    is the responsability of the calling scope to free the stringlist
    with stringlist_free();


    If @pattern is different from NULL only wells which 'match' the
    pattern is included; if @pattern == NULL all wells are
    included. The match is done with function fnmatch() -
    i.e. standard shell wildcards.
*/

static stringlist_type *
rd_smspec_alloc_map_list(const std::map<std::string, node_map> &mp,
                         const char *pattern) {
    stringlist_type *map_list = stringlist_alloc_new();

    for (const auto &pair : mp) {
        const char *map_name = pair.first.c_str();

        if (pattern == NULL)
            stringlist_append_copy(map_list, map_name);
        else if (util_fnmatch(pattern, map_name) == 0)
            stringlist_append_copy(map_list, map_name);
    }
    stringlist_sort(map_list, (string_cmp_ftype *)util_strcmp_int);
    return map_list;
}

UTIL_SAFE_CAST_FUNCTION(rd_sum_tstep, RD_SUM_TSTEP_ID)
UTIL_SAFE_CAST_FUNCTION_CONST(rd_sum_tstep, RD_SUM_TSTEP_ID)

void rd_sum_tstep_free(rd_sum_tstep_type *ministep) { delete ministep; }

/**
   This function sets the internal time representation in the
   rd_sum_tstep. The treatment of time is a bit weird; on the one
   hand the time elements in the summary data are just like any other
   element like e.g. the FOPT or GGPR:NAME - on the other hand the
   time information is strictly required and the summary file will
   fall to pieces if it is missing.

   The time can be provided in using (at least) two different
   keywords:

      DAYS/HOURS: The data vector will contain the number of
            days/hours since the simulation start (hours in the case
            of lab units).

      DAY,MONTH,YEAR: The data vector will contain the true date of
           the tstep.

   The rd_sum_tstep class can utilize both types of information, but
   will select the DAYS variety if both are present.
*/

static void rd_sum_tstep_iset(rd_sum_tstep_type *tstep, int index,
                              float value) {
    if ((index < static_cast<int>(tstep->data.size())) && (index >= 0))
        tstep->data[index] = value;
    else
        util_abort("%s: index:%d invalid. Valid range: [0,%d) \n", __func__,
                   index, tstep->data.size());
}

/*
  Should be called in write mode.
*/

rd_sum_tstep_type *rd_sum_tstep_alloc_new(int report_step, int ministep,
                                          float sim_seconds,
                                          const rd_smspec_type *smspec) {
    rd_sum_tstep_type *tstep =
        rd_sum_tstep_alloc(report_step, ministep, smspec);
    tstep->data = rd_smspec_get_params_default(smspec);

    rd_sum_tstep_set_time_info_from_seconds(tstep, smspec->sim_start_time,
                                            sim_seconds);
    rd_sum_tstep_iset(tstep, smspec->time_index,
                      sim_seconds / smspec->time_seconds);
    return tstep;
}

time_t rd_sum_tstep_get_sim_time(const rd_sum_tstep_type *ministep) {
    return ministep->sim_time;
}

double rd_sum_tstep_get_sim_days(const rd_sum_tstep_type *ministep) {
    return ministep->sim_seconds / (24 * 3600);
}

int rd_sum_tstep_get_report(const rd_sum_tstep_type *ministep) {
    return ministep->report_step;
}

int rd_sum_tstep_get_ministep(const rd_sum_tstep_type *ministep) {
    return ministep->ministep;
}

void rd_sum_tstep_set_from_node(rd_sum_tstep_type *tstep,
                                const rd::smspec_node &smspec_node,
                                float value) {
    int data_index = smspec_node_get_params_index(&smspec_node);
    rd_sum_tstep_iset(tstep, data_index, value);
}

double rd_sum_tstep_get_from_node(const rd_sum_tstep_type *tstep,
                                  const rd::smspec_node &smspec_node) {
    int data_index = smspec_node_get_params_index(&smspec_node);
    return rd_sum_tstep_iget(tstep, data_index);
}

void rd_sum_tstep_set_from_key(rd_sum_tstep_type *tstep, const char *gen_key,
                               float value) {
    const rd::smspec_node &smspec_node =
        rd_smspec_get_general_var_node(tstep->smspec, gen_key);
    rd_sum_tstep_set_from_node(tstep, smspec_node, value);
}

double rd_sum_tstep_get_from_key(const rd_sum_tstep_type *tstep,
                                 const char *gen_key) {
    const rd::smspec_node &smspec_node =
        rd_smspec_get_general_var_node(tstep->smspec, gen_key);
    return rd_sum_tstep_get_from_node(tstep, smspec_node);
}

bool rd_sum_tstep_has_key(const rd_sum_tstep_type *tstep, const char *gen_key) {
    return rd_smspec_has_general_var(tstep->smspec, gen_key);
}

static void rd_sum_data_build_index(rd_sum_data_type *self);
static rd_sum_data_type *rd_sum_data_alloc(rd_smspec_type *smspec) {
    rd_sum_data_type *data = new rd_sum_data_type();
    data->smspec = smspec;
    return data;
}

/**
   This function will take a report as input , and update the two
   pointers ministep1 and ministep2 with the range of the report step
   (in terms of ministeps).

   Calling this function with report_step == 2 for the example
   documented at the top of the file will yield: *ministep1 = 3 and
   *ministep2 = 7. If you are only interested in one of the limits you
   can pass in NULL for the other limit, i.e.

      xxx(data , report_step , NULL , &ministep2);

   to get the last step.

   If the supplied report_step is invalid the function will set both
   return values to -1 (the return value from safe_iget). In that case
   it is the responsability of the calling scope to check the return
   values.
*/

static double rd_sum_data_iget_sim_seconds(const rd_sum_data_type *data,
                                           int internal_index) {
    const auto index_node = data->index.lookup(internal_index);
    const auto data_file = data->data_files[index_node.data_index];
    return data_file->iget_sim_seconds(internal_index - index_node.offset);
}

static void rd_sum_data_fwrite_unified(const rd_sum_data_type *data,
                                       const fs::path &rd_case, bool fmt_case) {
    std::string filename =
        rd::filename(rd_case, RD_UNIFIED_SUMMARY_FILE, fmt_case, 0).string();
    ERT::FortIO fortio(filename, std::ios_base::out, fmt_case);

    for (auto &data_file : data->data_files)
        data_file->fwrite_unified(fortio.get());
}

static void rd_sum_data_fwrite_multiple(const rd_sum_data_type *data,
                                        const fs::path &rd_case,
                                        bool fmt_case) {

    for (auto &data_file : data->data_files)
        data_file->fwrite_multiple(rd_case.string(), fmt_case);
}

static void rd_sum_data_fwrite(const rd_sum_data_type *data,
                               const char *rd_case, bool fmt_case,
                               bool unified) {
    fs::path case_path(rd_case);
    if (unified)
        rd_sum_data_fwrite_unified(data, case_path, fmt_case);
    else
        rd_sum_data_fwrite_multiple(data, case_path, fmt_case);
}

static bool rd_sum_data_can_write(const rd_sum_data_type *data) {
    bool can_write = true;
    for (const auto &file_ptr : data->data_files)
        can_write &= file_ptr->can_write();

    return can_write;
}

static time_t rd_sum_data_get_sim_end(const rd_sum_data_type *data) {
    if (data->data_files.empty())
        throw std::out_of_range("rd_sum_data_get_sim_end: data_files empty");

    const auto &file_data = data->data_files.back();
    return file_data->get_sim_end();
}

static time_t rd_sum_data_get_data_start(const rd_sum_data_type *data) {
    if (data->data_files.empty())
        throw std::out_of_range("rd_sum_data_get_data_start: data_files empty");
    const auto &file_data = data->data_files[0];
    return file_data->get_data_start();
}

static double rd_sum_data_get_first_day(const rd_sum_data_type *data) {
    const auto &file_data = data->data_files[0];
    return file_data->get_days_start();
}

/**
   Returns the number of simulations days from the start of the
   simulation (irrespective of whether the that summary data has
   actually been loaded) to the last loaded simulation step.
*/
static double rd_sum_data_get_sim_length(const rd_sum_data_type *data) {
    const auto &file_data = data->data_files.back();
    return file_data->get_sim_length();
}

/**
   The check_sim_time() and check_sim_days() routines check if you
   have summary data for the requested date/days value. In the case of
   a restarted case, where the original case is missing - this will
   return false if the input values are in the region after simulation
   start with no data.
*/

static bool rd_sum_data_check_sim_time(const rd_sum_data_type *data,
                                       time_t sim_time) {
    if (sim_time < rd_sum_data_get_data_start(data))
        return false;

    if (sim_time > rd_sum_data_get_sim_end(data))
        return false;

    return true;
}

static time_t rd_sum_data_iget_sim_time(const rd_sum_data_type *data,
                                        int ministep_index) {
    const auto &index_node = data->index.lookup(ministep_index);
    const auto data_file = data->data_files[index_node.data_index];
    return data_file->iget_sim_time(ministep_index - index_node.offset);
}

/**
   This function will return the ministep corresponding to a time_t
   instance 'sim_time'. The function will fail hard if the time_t is
   before the simulation start, or after the end of the
   simulation. Check with

       smspec->sim_start_time and rd_sum_data_get_sim_end()

   first.

   See the documentation about report steps, ministeps and rates at
   the top of this file for how the sim_time relates to to the
   returned ministep_nr.

   The indices used in this function are the internal indices, and not
   ministep numbers. Observe that if there are holes in the
   time-domain, i.e. if RPTONLY has been used, the function can return
   a ministep index which does NOT cover the input time:

     The 'X' should represent report times - the dashed lines
     represent the temporal extent of two ministeps. Outside the '--'
     area we do not have any results. The two ministeps we actually
     have are M15 and M25, i.e. there is a hole.


      X      .      +-----X            +----X
            /|\        M15               M25
             |
             |

     When asking for the ministep number at the location of the arrow,
     the function will return '15', i.e. the valid ministep following
     the sim_time. Of course - the ideal situation is if the time
     sequence has no holes.
*/

static int rd_sum_data_get_index_from_sim_time(const rd_sum_data_type *data,
                                               time_t sim_time) {
    if (!rd_sum_data_check_sim_time(data, sim_time)) {
        time_t start_time = rd_sum_data_get_data_start(data);
        time_t end_time = rd_sum_data_get_sim_end(data);
        throw std::out_of_range(fmt::format(
            "Invalid time_t instance:{} interval:[{},{}] (simulation start:{})",
            sim_time, start_time, end_time, data->smspec->sim_start_time));
    }

    /*
     The moment we have passed the intial test we MUST find a valid
     ministep index, however care should be taken that there can
     perfectly well be 'holes' in the time domain, because of e.g. the
     RPTONLY keyword.
  */

    int low_index = 0;
    int high_index = data->index.length() - 1;

    // perform binary search
    while (low_index + 1 < high_index) {
        int center_index = (low_index + high_index) / 2;
        const time_t center_time =
            rd_sum_data_iget_sim_time(data, center_index);

        if (sim_time > center_time)
            low_index = center_index;
        else
            high_index = center_index;
    }

    return sim_time <= rd_sum_data_iget_sim_time(data, low_index) ? low_index
                                                                  : high_index;
}

/**
   This function will take a true time 'sim_time' as input. The
   ministep indices bracketing this sim_time are identified, and the
   corresponding weights are calculated.

   The actual value we are interested in can then be computed with the
   rd_sum_data_interp_get() function:


   int    param_index;
   time_t sim_time;
   {
      int    ministep1 , ministep2;
      double weight1   , weight2;

      rd_sum_data_init_interp_from_sim_time( data , sim_time , &ministep1 , &ministep2 , &weight1 , &weight2);
      return rd_sum_data_interp_get( data , ministep1 , ministep2 , weight1 , weight2 , param_index );
   }


   For further explanation (in particular for which keywords the
   function should be used), consult documentation at the top of this
   file.
*/

static void rd_sum_data_init_interp_from_sim_time(const rd_sum_data_type *data,
                                                  time_t sim_time, int *index1,
                                                  int *index2, double *weight1,
                                                  double *weight2) {
    int idx = rd_sum_data_get_index_from_sim_time(data, sim_time);

    // if sim_time is first date, idx=0 and then we cannot interpolate, so we give
    // weight 1 to index1=index2=0.
    if (idx == 0) {
        *index1 = 0;
        *index2 = 0;
        *weight1 = 1;
        *weight2 = 0;
        return;
    }

    time_t sim_time1 = rd_sum_data_iget_sim_time(data, idx - 1);
    time_t sim_time2 = rd_sum_data_iget_sim_time(data, idx);

    *index1 = idx - 1;
    *index2 = idx;

    // weights the interpolation each of the ministeps according to distance from sim_time
    double time_diff = sim_time2 - sim_time1;
    double time_dist1 = (sim_time - sim_time1);
    double time_dist2 = -(sim_time - sim_time2);

    *weight1 = time_dist2 / time_diff;
    *weight2 = time_dist1 / time_diff;
}

/**
    This will look up a value based on an internal index. The internal
    index will ALWAYS run in the interval [0,num_ministep), without
    any holes.
*/
static double rd_sum_data_iget(const rd_sum_data_type *data, int time_index,
                               int params_index) {
    const auto &index_node = data->index.lookup(time_index);
    const auto &file_data = data->data_files[index_node.data_index];
    const auto &params_map = index_node.params_map;
    if (params_map[params_index] >= 0)
        return file_data->iget(time_index - index_node.offset,
                               params_map[params_index]);
    else {
        const rd::smspec_node &smspec_node =
            rd_smspec_iget_node_w_params_index(data->smspec, params_index);
        return smspec_node.get_default();
    }
}

static double rd_sum_data_iget_last_value(const rd_sum_data_type *data,
                                          int param_index) {
    return rd_sum_data_iget(data, data->index.length() - 1, param_index);
}

static double rd_sum_data_iget_first_value(const rd_sum_data_type *data,
                                           int param_index) {
    return rd_sum_data_iget(data, 0, param_index);
}

static std::vector<int> rd_smspec_alloc_mapping(const rd_smspec_type *self,
                                                const rd_smspec_type *other) {
    int params_size = self->params_size;
    std::vector<int> mapping(params_size, -1);

    for (int i = 0; i < rd_smspec_num_nodes(self); i++) {
        const rd::smspec_node &self_node =
            rd_smspec_iget_node_w_node_index(self, i);
        int self_index = self_node.get_params_index();
        const char *key = self_node.get_gen_key1();
        if (rd_smspec_has_general_var(other, key)) {
            const rd::smspec_node &other_node =
                rd_smspec_get_general_var_node(other, key);
            int other_index = other_node.get_params_index();
            mapping[self_index] = other_index;
        }
    }

    return mapping;
}

static void rd_sum_data_build_index(rd_sum_data_type *self) {
    std::sort(self->data_files.begin(), self->data_files.end(),
              [](const std::shared_ptr<rd::rd_sum_file_data> &case1,
                 const std::shared_ptr<rd::rd_sum_file_data> &case2) {
                  return case1->get_data_start() < case2->get_data_start();
              });

    self->index.clear();
    for (size_t i = 0; i < self->data_files.size(); i++) {
        const auto &data = self->data_files[i];
        bool main_case = (i == (self->data_files.size() - 1));
        time_t next_start;

        if (main_case)
            self->index.add(data->length());
        else {
            const auto &next = self->data_files[i + 1];
            next_start = next->get_data_start();
            self->index.add(data->length_before(next_start));
        }

        auto &node = self->index.back();
        if (node.length > 0) {
            node.report1 = data->first_report();

            if (main_case)
                node.report2 = data->last_report();
            else
                node.report2 = data->report_before(next_start);

            node.time1 = data->get_data_start();
            node.time2 = data->get_sim_end();
            node.days1 = data->get_days_start();
            node.days2 = data->get_sim_length();
            node.params_map =
                rd_smspec_alloc_mapping(self->smspec, data->smspec());
        }
    }
}

static rd_sum_data_type *rd_sum_data_alloc_writer(rd_smspec_type *smspec) {
    rd_sum_data_type *data = rd_sum_data_alloc(smspec);
    data->data_files.push_back(std::make_shared<rd::rd_sum_file_data>(smspec));
    rd_sum_data_build_index(data);
    return data;
}

static void rd_sum_data_add_case(rd_sum_data_type *self,
                                 const rd_sum_data_type *other) {
    for (auto &other_file : other->data_files)
        self->data_files.push_back(other_file);

    rd_sum_data_build_index(self);
}

/*
  Observe that this can be called several times (but not with the same
  data - that will die).

  Warning: The index information of the rd_sum_data instance has
  __NOT__ been updated when leaving this function. That is done with a
  call to rd_sum_data_build_index().
*/

static bool rd_sum_data_fread(rd_sum_data_type *data,
                              const stringlist_type *filelist, bool lazy_load,
                              int file_options) {
    auto file_data = std::make_shared<rd::rd_sum_file_data>(data->smspec);
    if (file_data->fread(filelist, lazy_load, file_options)) {
        data->data_files.push_back(file_data);
        rd_sum_data_build_index(data);
        return true;
    }
    return false;
}

/**
   This function will form a weight average of the two ministeps
   @ministep1 and @ministep2. The weights and the ministep indices
   should (typically) be determined by the

      rd_sum_data_init_interp_from_sim_xxx()

   functions. The function will typically the last function called
   when we seek a reservoir state variable at an intermediate time
   between two ministeps.
*/

static double rd_sum_data_interp_get(const rd_sum_data_type *data,
                                     int time_index1, int time_index2,
                                     double weight1, double weight2,
                                     int params_index) {
    return rd_sum_data_iget(data, time_index1, params_index) * weight1 +
           rd_sum_data_iget(data, time_index2, params_index) * weight2;
}

static double rd_sum_data_vector_iget(const rd_sum_data_type *data,
                                      time_t sim_time, int params_index,
                                      bool is_rate, int time_index1,
                                      int time_index2, double weight1,
                                      double weight2) {

    double value = 0.0;
    if (is_rate) {
        int time_index = rd_sum_data_get_index_from_sim_time(data, sim_time);
        // uses step function since it is a rate
        value = rd_sum_data_iget(data, time_index, params_index);
    } else {
        // uses interpolation between timesteps
        value = rd_sum_data_interp_get(data, time_index1, time_index2, weight1,
                                       weight2, params_index);
    }
    return value;
}

static double
rd_sum_data_get_from_sim_time(const rd_sum_data_type *data, time_t sim_time,
                              const rd::smspec_node &smspec_node) {
    int params_index = smspec_node_get_params_index(&smspec_node);
    if (smspec_node_is_rate(&smspec_node)) {
        /*
      In general the mapping from sim_time to index is based on half
      open intervals, which are closed in the upper end:

          []<------------]<--------------]<-----------]
          t0             t1             t2           t3

       However - as indicated on the figure above there is a zero
       measure point right at the start which corresponds to
       time_index == 0; this is to ensure that there is correspondance
       with the simulator results if you ask for a value interpolated to
       the starting time.
    */
        int time_index = rd_sum_data_get_index_from_sim_time(data, sim_time);
        return rd_sum_data_iget(data, time_index, params_index);
    } else {
        /* Interpolated lookup based on two (hopefully) consecutive ministeps. */
        double weight1, weight2;
        int time_index1, time_index2;

        rd_sum_data_init_interp_from_sim_time(data, sim_time, &time_index1,
                                              &time_index2, &weight1, &weight2);
        return rd_sum_data_interp_get(data, time_index1, time_index2, weight1,
                                      weight2, params_index);
    }
}

/**
   Will go through the data and find the report step which EXACTLY
   matches the input sim_time. If no report step matches exactly the
   function will return -1.

   Observe that by default the report steps consist of half-open time
   intervals like this: (t1, t2]. However the first report step
   (i.e. report step 1, is a fully inclusive interval: [t0 , t1] where
   t0 is the simulation start time. That is not implemented here;
   meaning that if you supply the start time as @sim_time argument you
   will get -1 and not 0 as you might expect.

   It would certainly be possible to detect the start_time input
   argument and special case the return, but the opposite would be
   'impossible' - you would never get anything sensible out when using
   report_step == 0 as input to one of the functions expecting
   report_step input.
*/

static int rd_sum_data_get_report_step_from_time(const rd_sum_data_type *data,
                                                 time_t sim_time) {
    if (!rd_sum_data_check_sim_time(data, sim_time))
        return -1;
    else {
        auto files = data->index.lookup_time(sim_time);
        if (files.first != files.second)
            return -1;

        const auto &data_file = data->data_files[files.first->data_index];
        return data_file->report_step_from_time(sim_time);
    }
}

static double rd_sum_data_time2days(const rd_sum_data_type *data,
                                    time_t sim_time) {
    time_t start_time = data->smspec->sim_start_time;
    return util_difftime_days(start_time, sim_time);
}

static int rd_sum_data_get_first_report_step(const rd_sum_data_type *data) {
    const auto &data_file = data->data_files[0];
    return data_file->first_report();
}

static int rd_sum_data_get_last_report_step(const rd_sum_data_type *data) {
    const auto &data_file = data->data_files.back();
    return data_file->last_report();
}

static void rd_sum_data_init_time_vector__(const rd_sum_data_type *data,
                                           time_t *output_data,
                                           bool report_only) {
    int offset = 0;
    for (const auto &index_node : data->index) {
        const auto &data_file = data->data_files[index_node.data_index];

        if (report_only)
            offset += data_file->get_time_report(index_node.length,
                                                 &output_data[offset]);
        else {
            data_file->get_time(index_node.length, &output_data[offset]);
            offset += index_node.length;
        }
    }
}

static void rd_sum_data_init_double_vector(const rd_sum_data_type *data,
                                           int main_params_index,
                                           double *output_data,
                                           bool report_only = false) {
    int offset = 0;
    for (const auto &index_node : data->index) {
        const auto &data_file = data->data_files[index_node.data_index];
        const auto &params_map = index_node.params_map;
        int params_index = params_map[main_params_index];

        if (report_only) {
            const rd::smspec_node &smspec_node =
                rd_smspec_iget_node_w_params_index(data->smspec,
                                                   main_params_index);
            double default_value = smspec_node.get_default();
            offset +=
                data_file->get_data_report(params_index, index_node.length,
                                           &output_data[offset], default_value);
        } else {

            if (params_index >= 0)
                data_file->get_data(params_index, index_node.length,
                                    &output_data[offset]);
            else {
                const rd::smspec_node &smspec_node =
                    rd_smspec_iget_node_w_params_index(data->smspec,
                                                       main_params_index);
                for (int i = 0; i < index_node.length; i++)
                    output_data[offset + i] = smspec_node.get_default();
            }
            offset += index_node.length;
        }
    }
}

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
                     rd_sum->smspec->formatted, -1);
    rd_sum_ptr restart_case =
        read_summary(restart_path.string(), ":", lazy_load, true, file_options);
    if (restart_case) {
        rd_sum->restart_case = std::move(restart_case);
        rd_sum_data_add_case(rd_sum->data.get(),
                             rd_sum->restart_case->data.get());
    }
}

static rd_smspec_ptr read_smspec(const std::string &header_file,
                                 const std::string &key_join_string,
                                 bool include_restart) {
    return {
        rd_smspec_fread_alloc(header_file, key_join_string, include_restart),
        &rd_smspec_free};
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
    if (rd_sum->data.get()->index.length() > 0)
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
    if (rd_sum->data.get()->index.length() > 0)
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
    auto data = rd_sum->data.get();
    const auto &file_data = data->data_files.back();
    rd_sum_tstep_type *tstep =
        file_data->add_new_tstep(report_step, sim_seconds);
    rd_sum_data_build_index(data);
    return tstep;
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
    const auto node_ptr =
        rd_smspec_get_var_node(rd_sum->smspec->gen_var_index, lookup_kw);
    return node_valid_index(node_ptr);
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

/**
  If the keylist contains invalid indices the corresponding element in the
  results vector will *not* be updated; i.e. it is smart to initialize the
  results vector with an invalid-value marker before calling this function:

  double_vector_type * results = double_vector_alloc( rd_sum_vector_get_size(keys), NAN);
  rd_sum_get_interp_vector( data, sim_time, keys, results);
*/
void rd_sum_get_interp_vector(const rd_sum_type *rd_sum, time_t sim_time,
                              const rd_sum_vector_type *keylist,
                              double_vector_type *results) {
    auto data = rd_sum->data.get();
    int num_keywords = rd_sum_vector_get_size(keylist);
    double weight1, weight2;
    int time_index1, time_index2;

    rd_sum_data_init_interp_from_sim_time(data, sim_time, &time_index1,
                                          &time_index2, &weight1, &weight2);
    double_vector_reset(results);
    for (int i = 0; i < num_keywords; i++) {
        if (rd_sum_vector_iget_valid(keylist, i)) {
            int params_index = rd_sum_vector_iget_param_index(keylist, i);
            bool is_rate = rd_sum_vector_iget_is_rate(keylist, i);
            double value = rd_sum_data_vector_iget(
                data, sim_time, params_index, is_rate, time_index1, time_index2,
                weight1, weight2);
            double_vector_iset(results, i, value);
        }
    }
}

void rd_sum_fwrite_interp_csv_line(const rd_sum_type *rd_sum, time_t sim_time,
                                   const rd_sum_vector_type *key_words,
                                   FILE *fp) {
    auto data = rd_sum->data.get();
    int num_keywords = rd_sum_vector_get_size(key_words);
    double weight1, weight2;
    int time_index1, time_index2;

    rd_sum_data_init_interp_from_sim_time(data, sim_time, &time_index1,
                                          &time_index2, &weight1, &weight2);

    for (int i = 0; i < num_keywords; i++) {
        if (rd_sum_vector_iget_valid(key_words, i)) {
            int params_index = rd_sum_vector_iget_param_index(key_words, i);
            bool is_rate = rd_sum_vector_iget_is_rate(key_words, i);
            double value = rd_sum_data_vector_iget(
                data, sim_time, params_index, is_rate, time_index1, time_index2,
                weight1, weight2);

            if (i == 0)
                fprintf(fp, "%f", value);
            else
                fprintf(fp, ",%f", value);
        } else {
            if (i == 0)
                fputs("", fp);
            else
                fputs(",", fp);
        }
    }
}

double rd_sum_get_general_var_from_sim_time(const rd_sum_type *rd_sum,
                                            time_t sim_time, const char *var) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, var);
    return rd_sum_get_from_sim_time(rd_sum, sim_time, node);
}
double rd_sum_get_general_var_from_sim_days(const rd_sum_type *rd_sum,
                                            double sim_days, const char *var) {
    const rd::smspec_node *node = rd_sum_get_general_var_node(rd_sum, var);
    time_t sim_time = rd_sum->data->smspec->sim_start_time;
    util_inplace_forward_days_utc(&sim_time, sim_days);
    return rd_sum_data_get_from_sim_time(rd_sum->data.get(), sim_time, *node);
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

    const int *grid_dims = rd_sum->smspec->grid_dims;

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

/**
   Returns the last index included in report step @report_step.
   Observe that if the dataset does not include @report_step at all,
   the function will return INVALID_MINISTEP_NR; this must be checked for in the
   calling scope.
*/
static int rd_sum_data_iget_report_end(const rd_sum_data_type *data,
                                       int report_step) {
    const auto &index_node = data->index.lookup_report(report_step);
    const auto &file_data = data->data_files[index_node.data_index];
    auto range = file_data->report_range(report_step);
    return range.second;
}

int rd_sum_iget_report_end(const rd_sum_type *rd_sum, int report_step) {
    return rd_sum_data_iget_report_end(rd_sum->data.get(), report_step);
}

int rd_sum_iget_report_step(const rd_sum_type *rd_sum, int internal_index) {
    auto data = rd_sum->data.get();
    const auto &index_node = data->index.lookup(internal_index);
    const auto &file_data = data->data_files[index_node.data_index];
    return file_data->iget_report(internal_index - index_node.offset);
}

time_t_vector_type *rd_sum_alloc_time_vector(const rd_sum_type *rd_sum,
                                             bool report_only) {
    auto data = rd_sum->data.get();
    std::vector<time_t> output_data;
    if (report_only)
        output_data.resize(1 + rd_sum_data_get_last_report_step(data) -
                           rd_sum_data_get_first_report_step(data));
    else
        output_data.resize(data->index.length());

    rd_sum_data_init_time_vector__(data, output_data.data(), report_only);
    time_t_vector_type *time_vector =
        time_t_vector_alloc(output_data.size(), 0);
    {
        time_t *tmp_data = time_t_vector_get_ptr(time_vector);
        memcpy(tmp_data, output_data.data(),
               output_data.size() * sizeof(time_t));
    }
    return time_vector;
}

void rd_sum_init_double_vector(const rd_sum_type *rd_sum, const char *gen_key,
                               double *data) {
    int params_index = rd_sum_get_general_var_params_index(rd_sum, gen_key);
    rd_sum_data_init_double_vector(rd_sum->data.get(), params_index, data);
}

void rd_sum_init_double_vector_interp(const rd_sum_type *rd_sum,
                                      const char *gen_key,
                                      const time_t_vector_type *time_points,
                                      double *output_data) {
    const rd::smspec_node &smspec_node =
        rd_smspec_get_general_var_node(rd_sum->smspec.get(), gen_key);
    auto data = rd_sum->data.get();
    bool is_rate = smspec_node_is_rate(&smspec_node);
    int params_index = smspec_node_get_params_index(&smspec_node);
    time_t start_time = rd_sum_data_get_data_start(data);
    time_t end_time = rd_sum_data_get_sim_end(data);
    double start_value = 0;
    double end_value = 0;

    if (!is_rate) {
        start_value = rd_sum_data_iget_first_value(data, params_index);
        end_value = rd_sum_data_iget_last_value(data, params_index);
    }

    for (int time_index = 0; time_index < time_t_vector_size(time_points);
         time_index++) {
        time_t sim_time = time_t_vector_iget(time_points, time_index);
        double value;
        if (sim_time < start_time)
            value = start_value;

        else if (sim_time > end_time)
            value = end_value;

        else {
            int time_index1, time_index2;
            double weight1, weight2;
            rd_sum_data_init_interp_from_sim_time(
                data, sim_time, &time_index1, &time_index2, &weight1, &weight2);
            value = rd_sum_data_vector_iget(data, sim_time, params_index,
                                            is_rate, time_index1, time_index2,
                                            weight1, weight2);
        }

        output_data[time_index] = value;
    }
}

void rd_sum_init_double_frame_interp(const rd_sum_type *rd_sum,
                                     const rd_sum_vector_type *keywords,
                                     const time_t_vector_type *time_points,
                                     double *output_data) {
    auto data = rd_sum->data.get();
    int num_keywords = rd_sum_vector_get_size(keywords);
    int time_stride = num_keywords;
    int key_stride = 1;
    time_t start_time = rd_sum_data_get_data_start(data);
    time_t end_time = rd_sum_data_get_sim_end(data);

    for (int time_index = 0; time_index < time_t_vector_size(time_points);
         time_index++) {
        time_t sim_time = time_t_vector_iget(time_points, time_index);
        if (sim_time < start_time) {
            for (int key_index = 0; key_index < num_keywords; key_index++) {
                int param_index =
                    rd_sum_vector_iget_param_index(keywords, key_index);
                int data_index =
                    key_index * key_stride + time_index * time_stride;
                bool is_rate = rd_sum_vector_iget_is_rate(keywords, key_index);
                if (is_rate)
                    output_data[data_index] = 0;
                else
                    output_data[data_index] =
                        rd_sum_data_iget_first_value(data, param_index);
            }
        } else if (sim_time > end_time) {
            for (int key_index = 0; key_index < num_keywords; key_index++) {
                int param_index =
                    rd_sum_vector_iget_param_index(keywords, key_index);
                int data_index =
                    key_index * key_stride + time_index * time_stride;
                bool is_rate = rd_sum_vector_iget_is_rate(keywords, key_index);
                if (is_rate)
                    output_data[data_index] = 0;
                else
                    output_data[data_index] =
                        rd_sum_data_iget_last_value(data, param_index);
            }
        } else {
            double weight1, weight2;
            int time_index1, time_index2;

            rd_sum_data_init_interp_from_sim_time(
                data, sim_time, &time_index1, &time_index2, &weight1, &weight2);

            for (int key_index = 0; key_index < num_keywords; key_index++) {
                int param_index =
                    rd_sum_vector_iget_param_index(keywords, key_index);
                int data_index =
                    key_index * key_stride + time_index * time_stride;
                bool is_rate = rd_sum_vector_iget_is_rate(keywords, key_index);
                double value = rd_sum_data_vector_iget(
                    data, sim_time, param_index, is_rate, time_index1,
                    time_index2, weight1, weight2);
                output_data[data_index] = value;
            }
        }
    }
}

double_vector_type *rd_sum_alloc_data_vector(const rd_sum_type *rd_sum,
                                             int data_index, bool report_only) {
    auto data = rd_sum->data.get();
    std::vector<double> output_data;
    if (report_only)
        output_data.resize(1 + rd_sum_data_get_last_report_step(data) -
                           rd_sum_data_get_first_report_step(data));
    else
        output_data.resize(data->index.length());

    if (data_index >= data->smspec->params_size)
        throw std::out_of_range("Out of range");

    rd_sum_data_init_double_vector(data, data_index, output_data.data(),
                                   report_only);
    double_vector_type *data_vector =
        double_vector_alloc(output_data.size(), 0);
    {
        double *tmp_data = double_vector_get_ptr(data_vector);
        memcpy(tmp_data, output_data.data(),
               output_data.size() * sizeof(double));
    }
    return data_vector;
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
    const int length = rd_sum->data->index.length();
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
    auto data = rd_sum->data.get();
    if (report_step == 0)
        return data->smspec->sim_start_time;
    else {
        int internal_index = rd_sum_data_iget_report_end(data, report_step);
        if (internal_index == -1)
            throw std::out_of_range(
                "Tried to look up step nr: " + std::to_string(report_step) +
                " from the restart file in summary file, but it does not "
                "exist. \n" +
                "The step entries in summary file must cover all entries "
                "in the restart file.");
        return rd_sum_data_iget_sim_time(data, internal_index);
    }
}

time_t rd_sum_iget_sim_time(const rd_sum_type *rd_sum, int index) {
    return rd_sum_data_iget_sim_time(rd_sum->data.get(), index);
}

time_t rd_sum_get_data_start(const rd_sum_type *rd_sum) {
    return rd_sum_data_get_data_start(rd_sum->data.get());
}

time_t rd_sum_get_start_time(const rd_sum_type *rd_sum) {
    return rd_sum->smspec->sim_start_time;
}

time_t rd_sum_get_end_time(const rd_sum_type *rd_sum) {
    try {
        return rd_sum_data_get_sim_end(rd_sum->data.get());
    } catch (std::out_of_range const &) {
        return rd_sum->smspec->sim_start_time;
    }
}

double rd_sum_iget_sim_days(const rd_sum_type *rd_sum, int index) {
    auto data = rd_sum->data.get();
    const auto index_node = data->index.lookup(index);
    const auto data_file = data->data_files[index_node.data_index];
    return data_file->iget_sim_days(index - index_node.offset);
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
    return rd_sum->smspec->restart_step;
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
    stringlist_type *keys = stringlist_alloc_new();
    rd_smspec_select_matching_general_var_list(rd_sum->smspec.get(), pattern,
                                               keys);
    return keys;
}

void rd_sum_select_matching_general_var_list(const rd_sum_type *rd_sum,
                                             const char *pattern,
                                             stringlist_type *keys) {
    rd_smspec_select_matching_general_var_list(rd_sum->smspec.get(), pattern,
                                               keys);
}

stringlist_type *rd_sum_alloc_well_list(const rd_sum_type *rd_sum,
                                        const char *pattern) {
    return rd_smspec_alloc_map_list(rd_sum->smspec->well_var_index, pattern);
}

stringlist_type *rd_sum_alloc_group_list(const rd_sum_type *rd_sum,
                                         const char *pattern) {
    return rd_smspec_alloc_map_list(rd_sum->smspec->group_var_index, pattern);
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
    return rd_sum->data->index.length();
}

bool rd_sum_check_sim_time(const rd_sum_type *sum, time_t sim_time) {
    return rd_sum_data_check_sim_time(sum->data.get(), sim_time);
}

bool rd_sum_check_sim_days(const rd_sum_type *sum, double sim_days) {
    auto data = sum->data.get();
    return sim_days >= rd_sum_data_get_first_day(data) &&
           sim_days <= rd_sum_data_get_sim_length(data);
}

int rd_sum_get_report_step_from_time(const rd_sum_type *sum, time_t sim_time) {
    return rd_sum_data_get_report_step_from_time(sum->data.get(), sim_time);
}

int rd_sum_get_report_step_from_days(const rd_sum_type *sum, double sim_days) {
    auto data = sum->data.get();
    if ((sim_days < rd_sum_data_get_first_day(data)) ||
        (sim_days > rd_sum_data_get_sim_length(data)))
        return -1;
    else {
        auto files = data->index.lookup_days(sim_days);
        if (files.first != files.second)
            return -1;

        const auto &data_file = data->data_files[files.first->data_index];
        return data_file->report_step_from_days(sim_days);
    }
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
    auto data = rd_sum->data.get();
    double_vector_type *solution = double_vector_alloc(0, 0);
    const int param_index = smspec_node_get_params_index(node);
    const int size = data->index.length();

    if (size <= 1)
        return solution;

    for (int index = 0; index < size; ++index) {
        int prev_index = util_int_max(0, index - 1);
        double value = rd_sum_data_iget(data, index, param_index);
        double prev_value = rd_sum_data_iget(data, prev_index, param_index);

        // cmp_value in interval value (closed) and prev_value (open)
        bool contained = (value == cmp_value);
        contained |= (util_double_min(prev_value, value) < cmp_value) &&
                     (cmp_value < util_double_max(prev_value, value));

        if (!contained)
            continue;

        double prev_time = rd_sum_data_iget_sim_seconds(data, prev_index);
        double time = rd_sum_data_iget_sim_seconds(data, index);

        if (smspec_node_is_rate(node)) {
            double_vector_append(solution,
                                 rates_clamp_lower ? prev_time + 1 : time);
        } else {
            double slope = (value - prev_value) / (time - prev_time);
            double seconds = (cmp_value - prev_value) / slope + prev_time;
            double_vector_append(solution, seconds);
        }
    }
    return solution;
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
    return rd_sum->smspec->unit_system;
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
