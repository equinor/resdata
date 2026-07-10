#pragma once
#include <ctime>

#include <ert/util/util.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_util.hpp>

struct RSTHead {
    // The report step is from the SEQNUM keyword for unified files,
    // and inferred from the filename for non unified files.
    int report_step;
    int day;
    int year;
    int month;
    time_t sim_time;
    int version;   // 100, 300, 500 (Eclipse300-Thermal)
    int phase_sum; // Oil = 1   Gas = 2    Water = 4

    ert_rd_unit_enum unit_system;

    int nx;
    int ny;
    int nz;
    int nactive;
    /* All fields below the line are taken literally (apart from
       lowercasing) from the section about restart files in the
       ECLIPSE File Formats Reference Manual. The elements typically
       serve as dimensions in the ?WEL, ?SEG and ?CON arrays.
    */

    // Pure well properties
    int nwells; // Number of wells
    int niwelz; // Number of elements pr well in IWEL array
    int nzwelz; // Number of 8 character words pr well in ZWEL array
    int nxwelz; // Number of elements pr well in XWEL array.

    // Connection properties
    int niconz; // Number of elements per completion in ICON array
    int ncwmax; // Maximum number of completions per well
    int nsconz; // Number of elements per completion in SCON array
    int nxconz; // Number of elements per completion in XCON array

    // Segment properties
    int nisegz; // Number of entries pr segment in the ISEG array
    int nsegmx; // The maximum number of segments pr well
    int nswlmx; // The maximum number of segmented wells
    int nlbrmx; // The maximum number of lateral branches pr well
    int nilbrz; // The number of entries pr segment in ILBR array
    int nrsegz; // The number of entries pr segment in RSEG array

    // Properteies from the LOGIHEAD keyword:
    bool dualp;

    // Properties from the DOUBHEAD keyword:
    double sim_days;

    inline RSTHead(int report_step, int day, int year, int month,
                   time_t sim_time, int version, int phase_sum,
                   ert_rd_unit_enum unit_system, int nx, int ny, int nz,
                   int nactive, int nwells, int niwelz, int nzwelz, int nxwelz,
                   int niconz, int ncwmax, int nsconz, int nxconz, int nisegz,
                   int nsegmx, int nswlmx, int nlbrmx, int nilbrz, int nrsegz,
                   bool dualp, double sim_days)
        : report_step(report_step), day(day), year(year), month(month),
          sim_time(sim_time), version(version), phase_sum(phase_sum),
          unit_system(unit_system), nx(nx), ny(ny), nz(nz), nactive(nactive),
          nwells(nwells), niwelz(niwelz), nzwelz(nzwelz), nxwelz(nxwelz),
          niconz(niconz), ncwmax(ncwmax), nsconz(nsconz), nxconz(nxconz),
          nisegz(nisegz), nsegmx(nsegmx), nswlmx(nswlmx), nlbrmx(nlbrmx),
          nilbrz(nilbrz), nrsegz(nrsegz), dualp(dualp), sim_days(sim_days) {};

    inline RSTHead(int report_step, const rd_kw_type *intehead_kw,
                   const rd_kw_type *doubhead_kw, const rd_kw_type *logihead_kw)
        : report_step(report_step),
          sim_days(rd_kw_iget_double(doubhead_kw, DOUBHEAD_DAYS_INDEX)) {

        int nihead = rd_kw_get_size(intehead_kw);
        const int *data = (const int *)rd_kw_get_void_ptr(intehead_kw);

        auto get = [data, nihead](int index) {
            return index < nihead ? data[index] : 0;
        };

        this->day = get(INTEHEAD_DAY_INDEX);
        this->month = get(INTEHEAD_MONTH_INDEX);
        this->year = get(INTEHEAD_YEAR_INDEX);
        this->version = get(INTEHEAD_IPROG_INDEX);
        this->phase_sum = get(INTEHEAD_PHASE_INDEX);

        this->unit_system =
            static_cast<ert_rd_unit_enum>(get(INTEHEAD_UNIT_INDEX));

        this->nx = get(INTEHEAD_NX_INDEX);
        this->ny = get(INTEHEAD_NY_INDEX);
        this->nz = get(INTEHEAD_NZ_INDEX);
        this->nactive = get(INTEHEAD_NACTIVE_INDEX);

        this->nwells = get(INTEHEAD_NWELLS_INDEX);
        this->niwelz = get(INTEHEAD_NIWELZ_INDEX);
        this->nxwelz = get(INTEHEAD_NXWELZ_INDEX);
        this->nzwelz = get(INTEHEAD_NZWELZ_INDEX);
        this->nsconz = get(INTEHEAD_NSCONZ_INDEX);
        this->nxconz = get(INTEHEAD_NXCONZ_INDEX);
        this->niconz = get(INTEHEAD_NICONZ_INDEX);
        this->ncwmax = get(INTEHEAD_NCWMAX_INDEX);

        this->nisegz = get(INTEHEAD_NISEGZ_INDEX);
        this->nsegmx = get(INTEHEAD_NSEGMX_INDEX);
        this->nswlmx = get(INTEHEAD_NSWLMX_INDEX);
        this->nlbrmx = get(INTEHEAD_NLBRMX_INDEX);
        this->nilbrz = get(INTEHEAD_NILBRZ_INDEX);
        this->nrsegz = get(INTEHEAD_NRSEGZ_INDEX);

        this->sim_time = rd_make_date(this->day, this->month, this->year);

        if (logihead_kw)
            this->dualp = rd_kw_iget_bool(logihead_kw, LOGIHEAD_DUALP_INDEX);
        else
            this->dualp = false;
    }

    inline static RSTHead read(rd::FileView *rst_view, int report_step) {
        const rd_kw_type *intehead_kw = rst_view->get_kw(INTEHEAD_KW, 0);
        const rd_kw_type *doubhead_kw = rst_view->get_kw(DOUBHEAD_KW, 0);
        const rd_kw_type *logihead_kw = NULL;

        if (rst_view->has_kw(LOGIHEAD_KW))
            logihead_kw = rst_view->get_kw(LOGIHEAD_KW, 0);

        if (rst_view->has_kw(SEQNUM_KW)) {
            const rd_kw_type *seqnum_kw = rst_view->get_kw(SEQNUM_KW, 0);
            report_step = rd_kw_iget_int(seqnum_kw, 0);
        }

        return {report_step, intehead_kw, doubhead_kw, logihead_kw};
    }

    inline bool operator==(const RSTHead &other) const {
        bool equal = true;
        equal &= (this->day == other.day);
        equal &= (this->year == other.year);
        equal &= (this->month == other.month);
        equal &= (this->sim_time == other.sim_time);
        equal &= (this->version == other.version);
        equal &= (this->phase_sum == other.phase_sum);
        equal &= (this->nx == other.nx);
        equal &= (this->ny == other.ny);
        equal &= (this->nz == other.nz);
        equal &= (this->nactive == other.nactive);
        equal &= (this->nwells == other.nwells);
        equal &= (this->niwelz == other.niwelz);
        equal &= (this->nzwelz == other.nzwelz);
        equal &= (this->niconz == other.niconz);
        equal &= (this->ncwmax == other.ncwmax);
        equal &= (this->nisegz == other.nisegz);
        equal &= (this->nsegmx == other.nsegmx);
        equal &= (this->nswlmx == other.nswlmx);
        equal &= (this->nlbrmx == other.nlbrmx);
        equal &= (this->nilbrz == other.nilbrz);
        equal &= (this->dualp == other.dualp);
        equal &= util_double_approx_equal(this->sim_days, other.sim_days);

        return equal;
    }
};
