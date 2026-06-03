#ifndef ERT_RD_SUM_TSTEP_H
#define ERT_RD_SUM_TSTEP_H

#include <ert/util/int_vector.hpp>

#include <resdata/rd_smspec.hpp>
#include <resdata/rd_kw.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_sum_tstep_struct rd_sum_tstep_type;

void rd_sum_tstep_free(rd_sum_tstep_type *ministep);
rd_sum_tstep_type *rd_sum_tstep_alloc_new(int report_step, int ministep,
                                          float sim_seconds,
                                          const rd_smspec_type *smspec);

void rd_sum_tstep_set_from_node(rd_sum_tstep_type *tstep,
                                const rd::smspec_node &smspec_node,
                                float value);
double rd_sum_tstep_get_from_node(const rd_sum_tstep_type *tstep,
                                  const rd::smspec_node &smspec_node);

time_t rd_sum_tstep_get_sim_time(const rd_sum_tstep_type *ministep);
double rd_sum_tstep_get_sim_days(const rd_sum_tstep_type *ministep);

int rd_sum_tstep_get_report(const rd_sum_tstep_type *ministep);
int rd_sum_tstep_get_ministep(const rd_sum_tstep_type *ministep);

void rd_sum_tstep_set_from_key(rd_sum_tstep_type *tstep, const char *gen_key,
                               float value);
double rd_sum_tstep_get_from_key(const rd_sum_tstep_type *tstep,
                                 const char *gen_key);
bool rd_sum_tstep_has_key(const rd_sum_tstep_type *tstep, const char *gen_key);

UTIL_SAFE_CAST_HEADER(rd_sum_tstep);
UTIL_SAFE_CAST_HEADER_CONST(rd_sum_tstep);

#ifdef __cplusplus
}

#endif
#endif
