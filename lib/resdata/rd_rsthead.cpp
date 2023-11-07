#include <stdlib.h>

#include <ert/util/util.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw_magic.hpp>

static time_t rsthead_date(int day, int month, int year) {
    return rd_make_date(day, month, year);
}

time_t rd_rsthead_date(const rd_kw_type *intehead_kw) {
    return rsthead_date(rd_kw_iget_int(intehead_kw, INTEHEAD_DAY_INDEX),
                        rd_kw_iget_int(intehead_kw, INTEHEAD_MONTH_INDEX),
                        rd_kw_iget_int(intehead_kw, INTEHEAD_YEAR_INDEX));
}

time_t rd_rsthead_get_sim_time(const rd_rsthead_type *header) {
    return header->sim_time;
}

double rd_rsthead_get_sim_days(const rd_rsthead_type *header) {
    return header->sim_days;
}

int rd_rsthead_get_report_step(const rd_rsthead_type *header) {
    return header->report_step;
}

rd_rsthead_type *rd_rsthead_alloc_from_kw(int report_step,
                                          const rd_kw_type *intehead_kw,
                                          const rd_kw_type *doubhead_kw,
                                          const rd_kw_type *logihead_kw) {
    rd_rsthead_type *rsthead = (rd_rsthead_type *)util_malloc(sizeof *rsthead);
    rsthead->report_step = report_step;
    {
        const int *data = (const int *)rd_kw_get_void_ptr(intehead_kw);

        rsthead->day = data[INTEHEAD_DAY_INDEX];
        rsthead->month = data[INTEHEAD_MONTH_INDEX];
        rsthead->year = data[INTEHEAD_YEAR_INDEX];
        rsthead->version = data[INTEHEAD_IPROG_INDEX];
        rsthead->phase_sum = data[INTEHEAD_PHASE_INDEX];

        rsthead->nx = data[INTEHEAD_NX_INDEX];
        rsthead->ny = data[INTEHEAD_NY_INDEX];
        rsthead->nz = data[INTEHEAD_NZ_INDEX];
        rsthead->nactive = data[INTEHEAD_NACTIVE_INDEX];

        rsthead->nwells = data[INTEHEAD_NWELLS_INDEX];
        rsthead->niwelz = data[INTEHEAD_NIWELZ_INDEX];
        rsthead->nxwelz = data[INTEHEAD_NXWELZ_INDEX];
        rsthead->nzwelz = data[INTEHEAD_NZWELZ_INDEX];
        rsthead->nsconz = data[INTEHEAD_NSCONZ_INDEX];
        rsthead->nxconz = data[INTEHEAD_NXCONZ_INDEX];
        rsthead->niconz = data[INTEHEAD_NICONZ_INDEX];
        rsthead->ncwmax = data[INTEHEAD_NCWMAX_INDEX];

        rsthead->nisegz = data[INTEHEAD_NISEGZ_INDEX];
        rsthead->nsegmx = data[INTEHEAD_NSEGMX_INDEX];
        rsthead->nswlmx = data[INTEHEAD_NSWLMX_INDEX];
        rsthead->nrsegz = data[INTEHEAD_NRSEGZ_INDEX];

        // The only derived quantity
        rsthead->sim_time =
            rsthead_date(rsthead->day, rsthead->month, rsthead->year);
    }
    rsthead->sim_days = rd_kw_iget_double(doubhead_kw, DOUBHEAD_DAYS_INDEX);
    if (logihead_kw)
        rsthead->dualp = rd_kw_iget_bool(logihead_kw, LOGIHEAD_DUALP_INDEX);

    return rsthead;
}

/*
  - If the rst_view corresponds to a block of an underlying unified
    restart file the report_step value will be inferred from the
    SEQNUM keyword.

  - If the rst_view corresponds to an underlying non-unified restart
    file the report step must have been inferred from the filename
    *prior* to calling this function.
*/

rd_rsthead_type *rd_rsthead_alloc(const rd_file_view_type *rst_view,
                                  int report_step) {
    const rd_kw_type *intehead_kw =
        rd_file_view_iget_named_kw(rst_view, INTEHEAD_KW, 0);
    const rd_kw_type *doubhead_kw =
        rd_file_view_iget_named_kw(rst_view, DOUBHEAD_KW, 0);
    const rd_kw_type *logihead_kw = NULL;

    if (rd_file_view_has_kw(rst_view, LOGIHEAD_KW))
        logihead_kw = rd_file_view_iget_named_kw(rst_view, LOGIHEAD_KW, 0);

    if (rd_file_view_has_kw(rst_view, SEQNUM_KW)) {
        const rd_kw_type *seqnum_kw =
            rd_file_view_iget_named_kw(rst_view, SEQNUM_KW, 0);
        report_step = rd_kw_iget_int(seqnum_kw, 0);
    }

    return rd_rsthead_alloc_from_kw(report_step, intehead_kw, doubhead_kw,
                                    logihead_kw);
}

rd_rsthead_type *rd_rsthead_alloc_empty() {
    rd_rsthead_type *rsthead = (rd_rsthead_type *)util_malloc(sizeof *rsthead);

    rsthead->day = 0;
    rsthead->month = 0;
    rsthead->year = 0;
    rsthead->version = 0;
    rsthead->phase_sum = 0;

    rsthead->nx = 0;
    rsthead->ny = 0;
    rsthead->nz = 0;
    rsthead->nactive = 0;

    rsthead->nwells = 0;
    rsthead->niwelz = 0;
    rsthead->nzwelz = 0;

    rsthead->nsconz = 0;
    rsthead->niconz = 0;
    rsthead->ncwmax = 0;

    rsthead->nisegz = 0;
    rsthead->nsegmx = 0;
    rsthead->nswlmx = 0;
    rsthead->nrsegz = 0;

    rsthead->sim_time = 0;

    rsthead->dualp = false;
    rsthead->sim_days = 0.0;

    return rsthead;
}

void rd_rsthead_fprintf(const rd_rsthead_type *header, FILE *stream) {
    fprintf(stream, "nx      %d \n", header->nx);
    fprintf(stream, "nwells  %d \n", header->nwells);
    fprintf(stream, "niwelz  %d \n\n", header->niwelz);
}

bool rd_rsthead_equal(const rd_rsthead_type *header1,
                      const rd_rsthead_type *header2) {
    bool equal = true;

    equal = equal && (header1->day == header2->day);
    equal = equal && (header1->year == header2->year);
    equal = equal && (header1->month == header2->month);
    equal = equal && (header1->sim_time == header2->sim_time);
    equal = equal && (header1->version == header2->version);
    equal = equal && (header1->phase_sum == header2->phase_sum);
    equal = equal && (header1->nx == header2->nx);
    equal = equal && (header1->ny == header2->ny);
    equal = equal && (header1->nz == header2->nz);
    equal = equal && (header1->nactive == header2->nactive);
    equal = equal && (header1->nwells == header2->nwells);
    equal = equal && (header1->niwelz == header2->niwelz);
    equal = equal && (header1->nzwelz == header2->nzwelz);
    equal = equal && (header1->niconz == header2->niconz);
    equal = equal && (header1->ncwmax == header2->ncwmax);
    equal = equal && (header1->nisegz == header2->nisegz);
    equal = equal && (header1->nsegmx == header2->nsegmx);
    equal = equal && (header1->nswlmx == header2->nswlmx);
    equal = equal && (header1->nlbrmx == header2->nlbrmx);
    equal = equal && (header1->nilbrz == header2->nilbrz);
    equal = equal && (header1->dualp == header2->dualp);
    equal =
        equal && util_double_approx_equal(header1->sim_days, header2->sim_days);

    return equal;
}

void rd_rsthead_fprintf_struct(const rd_rsthead_type *header, FILE *stream) {
    fprintf(stream, "{.day = %d,\n", header->day);
    fprintf(stream, ".year = %d,\n", header->year);
    fprintf(stream, ".month = %d,\n", header->month);
    fprintf(stream, ".sim_time = %ld,\n", header->sim_time);
    fprintf(stream, ".version = %d,\n", header->version);
    fprintf(stream, ".phase_sum = %d,\n", header->phase_sum);
    fprintf(stream, ".nx = %d,\n", header->nx);
    fprintf(stream, ".ny = %d,\n", header->ny);
    fprintf(stream, ".nz = %d,\n", header->nz);
    fprintf(stream, ".nactive = %d,\n", header->nactive);
    fprintf(stream, ".nwells = %d,\n", header->nwells);
    fprintf(stream, ".niwelz = %d,\n", header->niwelz);
    fprintf(stream, ".nzwelz = %d,\n", header->nzwelz);
    fprintf(stream, ".niconz = %d,\n", header->niconz);
    fprintf(stream, ".ncwmax = %d,\n", header->ncwmax);
    fprintf(stream, ".nisegz = %d,\n", header->nisegz);
    fprintf(stream, ".nsegmx = %d,\n", header->nsegmx);
    fprintf(stream, ".nswlmx = %d,\n", header->nswlmx);
    fprintf(stream, ".nlbrmx = %d,\n", header->nlbrmx);
    fprintf(stream, ".nilbrz = %d,\n", header->nilbrz);
    fprintf(stream, ".dualp  = %d,\n", header->dualp);
    fprintf(stream, ".sim_days  = %g};\n", header->sim_days);
}

void rd_rsthead_free(rd_rsthead_type *rsthead) { free(rsthead); }

int rd_rsthead_get_nxconz(const rd_rsthead_type *rsthead) {
    return rsthead->nxconz;
}

int rd_rsthead_get_ncwmax(const rd_rsthead_type *rsthead) {
    return rsthead->ncwmax;
}
