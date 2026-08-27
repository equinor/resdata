#include <ctime>
#include <ios>
#include <new>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>

#include <resdata/rd_sum_tstep.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_endian_flip.hpp>

#include <detail/resdata/rd_sum_file_data.hpp>
#include <detail/resdata/rd_unsmry_loader.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_smspec.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_util.hpp>

namespace fs = std::filesystem;

namespace {

/** Report steps are stored as signed 32 bit values in the summary files and
 *  are derived from file names; validate before using them as indices. */
size_t validate_report_step(int report_step) {
    if (report_step < 0)
        throw std::invalid_argument("report step cannot be negative");
    return static_cast<size_t>(report_step);
}

} // namespace

/*
  This file implements the type rd_sum_data_type. The data structure
  is involved with holding all the actual summary data (the
  PARAMS vectors), in addition the time-information
  with MINISTEPS / REPORT_STEPS and so on is implemented here.

  This file has no information about how to index into the PARAMS
  vector, i.e. at which location can the WWCT for well P6 be found,
  that is the responsibility of the rd_smspec_type.

  The time direction in this system is implemented in terms of
  ministeps. There are some query / convert functions based on report
  steps.
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

namespace rd {

rd_sum_file_data::rd_sum_file_data(const rd_smspec_type *smspec)
    : rd_smspec(smspec) {}

rd_sum_file_data::~rd_sum_file_data() = default;

size_t rd_sum_file_data::length() const {
    if (this->loader)
        return this->loader->length();
    else
        return this->index.size();
}

size_t rd_sum_file_data::length_before(time_t end_time) const {
    size_t offset = 0;
    while (true) {
        time_t itime = this->iget_sim_time(offset);
        if (itime >= end_time)
            return offset;

        offset += 1;
        if (offset == this->length())
            return offset;
    }
}

size_t rd_sum_file_data::report_before(time_t end_time) const {
    size_t len = this->length_before(end_time);
    if (len == 0)
        throw std::invalid_argument("time argument before first report step");

    return this->index[len - 1].report_step;
}

size_t rd_sum_file_data::first_report() const {
    if (this->index.empty())
        throw std::out_of_range("rd_sum_file_data::first_report(): no data");
    const auto &node = this->index[0];
    return node.report_step;
}

size_t rd_sum_file_data::last_report() const {
    if (this->index.empty())
        throw std::out_of_range("rd_sum_file_data::last_report(): no data");
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

time_t rd_sum_file_data::iget_sim_time(size_t time_index) const {
    const auto &node = this->index[time_index];
    return node.sim_time;
}

/*
  Will return the length of the simulation in whatever units were used in input.
*/
double rd_sum_file_data::get_sim_length() const {
    const auto &node = this->index.back();
    return node.sim_seconds / rd_smspec_get_time_seconds(this->rd_smspec);
}

double rd_sum_file_data::iget(size_t time_index, size_t params_index) const {
    if (this->loader)
        return this->loader->iget(time_index, params_index);
    else {
        const rd_sum_tstep_type *ministep_data = iget_ministep(time_index);
        return rd_sum_tstep_iget(ministep_data, params_index);
    }
}

void rd_sum_file_data::append_tstep(rd_sum_tstep_ptr tstep) {
    /*
     Here the tstep is just appended naively, the vector will be
     sorted by ministep_nr before the data instance is returned.
  */

    data.push_back(std::move(tstep));
}

/*
  This function is meant to be called in write mode; and will create a
  new and empty tstep which is appended to the current data. The tstep
  will also be returned, so the calling scope can call
  rd_sum_tstep_iset() to set elements in the tstep.
*/

rd_sum_tstep_type *rd_sum_file_data::add_new_tstep(size_t report_step,
                                                   double sim_seconds) {
    int ministep_nr = static_cast<int>(data.size());
    rd_sum_tstep_ptr tstep(rd_sum_tstep_alloc_new(static_cast<int>(report_step),
                                                  ministep_nr, sim_seconds,
                                                  rd_smspec),
                           &rd_sum_tstep_free);
    rd_sum_tstep_type *prev_tstep = NULL;

    if (!data.empty())
        prev_tstep = data.back().get();

    rd_sum_tstep_type *new_tstep = tstep.get();
    append_tstep(std::move(tstep));

    bool rebuild_index = true;
    /*
    In the simple case that we just add another timestep to the
    currently active report_step, we do a limited update of the
    index, otherwise we call rd_sum_data_build_index() to get a
    full recalculation of the index.
  */
    if (!prev_tstep)
        goto exit;

    if (rd_sum_tstep_get_report(prev_tstep) !=
        rd_sum_tstep_get_report(new_tstep))
        goto exit;

    if (rd_sum_tstep_get_sim_days(prev_tstep) >=
        rd_sum_tstep_get_sim_days(new_tstep))
        goto exit;

    this->index.add(rd_sum_tstep_get_sim_time(new_tstep), sim_seconds,
                    report_step);
    rebuild_index = false;

exit:
    if (rebuild_index)
        this->build_index();

    return new_tstep;
}

rd_sum_tstep_type *
rd_sum_file_data::iget_ministep(size_t internal_index) const {
    return data.at(internal_index).get();
}

double rd_sum_file_data::iget_sim_days(size_t time_index) const {
    const auto &node = this->index[time_index];
    return node.sim_seconds / 86400;
}

double rd_sum_file_data::iget_sim_seconds(size_t time_index) const {
    const auto &node = this->index[time_index];
    return node.sim_seconds;
}

static bool cmp_ministep(const rd_sum_tstep_ptr &ministep1,
                         const rd_sum_tstep_ptr &ministep2) {
    return rd_sum_tstep_get_sim_time(ministep1.get()) <
           rd_sum_tstep_get_sim_time(ministep2.get());
}

void rd_sum_file_data::build_index() {
    this->index.clear();

    if (this->loader) {
        size_t offset = rd_smspec_get_first_step(this->rd_smspec) - 1;
        std::vector<size_t> report_steps = this->loader->report_steps(offset);
        std::vector<time_t> sim_time = this->loader->sim_time();
        std::vector<double> sim_seconds = this->loader->sim_seconds();

        for (size_t i = 0; i < this->loader->length(); i++) {
            this->index.add(sim_time[i], sim_seconds[i], report_steps[i]);
        }
    } else {
        std::stable_sort(data.begin(), data.end(), cmp_ministep);
        for (const auto &ministep : data)
            this->index.add(
                rd_sum_tstep_get_sim_time(ministep.get()),
                rd_sum_tstep_get_sim_seconds(ministep.get()),
                validate_report_step(rd_sum_tstep_get_report(ministep.get())));
    }
}

void rd_sum_file_data::get_time(size_t length, time_t *data) {
    for (size_t time_index = 0; time_index < length; time_index++)
        data[time_index] = this->iget_sim_time(time_index);
}

size_t rd_sum_file_data::get_time_report(size_t end_index, time_t *data) {
    size_t offset = 0;

    for (size_t report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        const auto range = this->report_range(report_step);
        if (!range)
            continue;

        if (range->last >= end_index)
            break;

        data[offset] = this->iget_sim_time(range->last);

        offset += 1;
    }
    return offset;
}

void rd_sum_file_data::get_data(size_t params_index, size_t length,
                                double *data) {
    if (this->loader) {
        const auto tmp_data = loader->get_vector(params_index);
        memcpy(data, tmp_data.data(), length * sizeof data);
    } else {
        for (size_t time_index = 0; time_index < length; time_index++)
            data[time_index] = this->iget(time_index, params_index);
    }
}

size_t rd_sum_file_data::get_data_report(std::optional<size_t> params_index,
                                         size_t end_index, double *data,
                                         double default_value) {
    size_t offset = 0;

    for (size_t report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        const auto range = this->report_range(report_step);
        if (!range)
            continue;

        if (range->last >= end_index)
            break;

        if (params_index)
            data[offset] = this->iget(range->last, *params_index);
        else
            data[offset] = default_value;

        offset += 1;
    }
    return offset;
}

bool rd_sum_file_data::has_report(size_t report_step) const {
    return this->index.has_report(report_step);
}

std::optional<ReportRange>
rd_sum_file_data::report_range(size_t report_step) const {
    return this->index.report_range(report_step);
}

void rd_sum_file_data::fwrite_report(size_t report_step,
                                     ERT::FortIO &fortio) const {
    {
        auto seqhdr_kw = make_rd_kw(SEQHDR_KW, SEQHDR_SIZE, RD_INT);
        rd_kw_iset_int(seqhdr_kw.get(), 0, 0);
        rd_kw_fwrite(seqhdr_kw.get(), fortio);
    }

    {
        const auto range = this->report_range(report_step);
        if (!range)
            return;

        for (size_t index = range->first; index <= range->last; index++) {
            const rd_sum_tstep_type *tstep = iget_ministep(index);
            rd_sum_tstep_fwrite(tstep, rd_smspec_get_index_map(rd_smspec),
                                fortio);
        }
    }
}

void rd_sum_file_data::fwrite_unified(ERT::FortIO &fortio) const {
    if (this->length() == 0)
        return;

    for (size_t report_step = first_report(); report_step <= last_report();
         report_step++) {
        if (has_report(report_step))
            fwrite_report(report_step, fortio);
    }
}

void rd_sum_file_data::fwrite_multiple(const std::string &rd_case,
                                       bool fmt_case) const {
    if (this->length() == 0)
        return;

    for (size_t report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        if (this->has_report(report_step)) {
            fs::path filename =
                rd::filename(rd_case, FileType::SUMMARY, fmt_case,
                             static_cast<int>(report_step));
            ERT::FortIO fortio(filename.string(), std::ios_base::out, fmt_case);

            fwrite_report(report_step, fortio);
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

bool rd_sum_file_data::check_file(rd::File *rd_file) {
    return rd_file->has_kw(PARAMS_KW) && (rd_file->num_named_kw(PARAMS_KW) ==
                                          rd_file->num_named_kw(MINISTEP_KW));
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

void rd_sum_file_data::add_rd_file(size_t report_step,
                                   rd::FileView &summary_view) {
    size_t num_ministep = summary_view.num_named_kw(PARAMS_KW);
    if (num_ministep > 0) {

        for (size_t ikw = 0; ikw < num_ministep; ikw++) {
            rd_kw_type *ministep_kw = summary_view.get_kw(MINISTEP_KW, ikw);
            rd_kw_type *params_kw = summary_view.get_kw(PARAMS_KW, ikw);

            {
                int ministep_nr = rd_kw_iget_int(ministep_kw, 0);
                std::string filename = summary_view.filename();
                rd_sum_tstep_ptr tstep(rd_sum_tstep_alloc_from_file(
                                           static_cast<int>(report_step),
                                           ministep_nr, params_kw,
                                           filename.c_str(), this->rd_smspec),
                                       &rd_sum_tstep_free);

                if (tstep)
                    append_tstep(std::move(tstep));
            }
        }
    }
}

bool rd_sum_file_data::fread(const std::vector<std::string> &filelist,
                             bool lazy_load, FileMode file_options) {
    if (filelist.empty())
        return false;

    std::string first_file = filelist[0];
    FileType file_type = rd_get_file_type(first_file.c_str(), NULL, NULL);
    if ((filelist.size() > 1) && (file_type != FileType::SUMMARY))
        throw std::invalid_argument("when calling with more than one file you "
                                    "can not supply a unified file");

    if (file_type == FileType::SUMMARY) {

        /* Not unified. */
        for (auto &data_file : filelist) {
            FileType file_type;
            int report_step;
            file_type = rd_get_file_type(data_file.c_str(), NULL, &report_step);
            if (file_type != FileType::SUMMARY)
                throw std::invalid_argument("file " + data_file +
                                            " has wrong type");
            {
                std::unique_ptr<rd::File> rd_file = rd::File::open(data_file);
                if (rd_file && check_file(rd_file.get())) {
                    auto global_view = rd_file->get_global_view();
                    this->add_rd_file(validate_report_step(report_step),
                                      *global_view);
                }
            }
        }
    } else if (file_type == FileType::UNIFIED_SUMMARY) {
        if (lazy_load) {
            try {
                this->loader.reset(new unsmry_loader(
                    this->rd_smspec, filelist[0], file_options));
            } catch (const std::bad_alloc &e) {
                return false;
            }
        } else {

            // Is this correct for a restarted chain of UNSMRY files? Looks like the
            // report step sequence will be restarted?
            std::unique_ptr<rd::File> rd_file = rd::File::open(first_file);
            if (rd_file && check_file(rd_file.get())) {
                size_t first_report_step =
                    rd_smspec_get_first_step(this->rd_smspec);
                size_t block_index = 0;
                while (true) {
                    /*
            Observe that there is a number discrepancy between ECLIPSE
            and the rd_file_select_smryblock() function. ECLIPSE
            starts counting report steps at 1; whereas the first
            SEQHDR block in the unified summary file is block zero (in
            ert counting).
        */
                    auto summary_view = rd_file->summary_view(block_index);
                    if (summary_view) {
                        this->add_rd_file(block_index + first_report_step,
                                          *summary_view);
                        block_index++;
                    } else
                        break;
                }
            }
        }
    }

    build_index();
    return (length() > 0);
}

const rd_smspec_type *rd_sum_file_data::smspec() const {
    return this->rd_smspec;
}

std::optional<size_t>
rd_sum_file_data::report_step_from_days(double sim_days) const {
    double sim_seconds = sim_days * 86400;
    for (size_t report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        const auto range = this->index.report_range(report_step);
        if (!range)
            continue;

        const auto &node = this->index[range->last];

        // Warning - this is a double == comparison!
        if (sim_seconds == node.sim_seconds)
            return report_step;
    }
    return std::nullopt;
}

std::optional<size_t>
rd_sum_file_data::report_step_from_time(time_t sim_time) const {
    for (size_t report_step = this->first_report();
         report_step <= this->last_report(); report_step++) {
        const auto range = this->index.report_range(report_step);
        if (!range)
            continue;

        const auto &node = this->index[range->last];
        if (sim_time == node.sim_time)
            return report_step;
    }
    return std::nullopt;
}

size_t rd_sum_file_data::iget_report(size_t time_index) const {
    const auto &index_node = this->index[time_index];
    return index_node.report_step;
}

} // namespace rd
