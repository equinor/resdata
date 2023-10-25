#ifndef ERT_RD_RSTHEAD_H
#define ERT_RD_RSTHEAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>

typedef struct {
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
} rd_rsthead_type;

void rd_rsthead_free(rd_rsthead_type *rsthead);
rd_rsthead_type *rd_rsthead_alloc_from_kw(int report_step,
                                          const rd_kw_type *intehead_kw,
                                          const rd_kw_type *doubhead_kw,
                                          const rd_kw_type *logihead_kw);
rd_rsthead_type *rd_rsthead_alloc(const rd_file_view_type *rst_file,
                                  int report_step);
rd_rsthead_type *rd_rsthead_alloc_empty(void);
time_t rd_rsthead_date(const rd_kw_type *intehead_kw);
void rd_rsthead_fprintf(const rd_rsthead_type *header, FILE *stream);
void rd_rsthead_fprintf_struct(const rd_rsthead_type *header, FILE *stream);
bool rd_rsthead_equal(const rd_rsthead_type *header1,
                      const rd_rsthead_type *header2);
double rd_rsthead_get_sim_days(const rd_rsthead_type *header);
int rd_rsthead_get_report_step(const rd_rsthead_type *header);
time_t rd_rsthead_get_sim_time(const rd_rsthead_type *header);
int rd_rsthead_get_nxconz(const rd_rsthead_type *rsthead);
int rd_rsthead_get_ncwmax(const rd_rsthead_type *rsthead);

#ifdef __cplusplus
}
#endif
#endif
