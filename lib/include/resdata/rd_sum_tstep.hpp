#ifndef ERT_RD_SUM_TSTEP_H
#define ERT_RD_SUM_TSTEP_H

#include <ert/util/int_vector.hpp>

#include <resdata/rd_smspec.hpp>
#include <resdata/rd_kw.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_sum_tstep_struct rd_sum_tstep_type;

rd_sum_tstep_type *
rd_sum_tstep_alloc_remap_copy(const rd_sum_tstep_type *src,
                              const rd_smspec_type *new_smspec,
                              float default_value, const int *params_map);
rd_sum_tstep_type *rd_sum_tstep_alloc_copy(const rd_sum_tstep_type *src);
void rd_sum_tstep_free(rd_sum_tstep_type *ministep);
void rd_sum_tstep_free__(void *__ministep);
rd_sum_tstep_type *rd_sum_tstep_alloc_from_file(int report_step,
                                                int ministep_nr,
                                                const rd_kw_type *params_kw,
                                                const char *src_file,
                                                const rd_smspec_type *smspec);

rd_sum_tstep_type *rd_sum_tstep_alloc_new(int report_step, int ministep,
                                          float sim_seconds,
                                          const rd_smspec_type *smspec);

void rd_sum_tstep_set_from_node(rd_sum_tstep_type *tstep,
                                const rd::smspec_node &smspec_node,
                                float value);
double rd_sum_tstep_get_from_node(const rd_sum_tstep_type *tstep,
                                  const rd::smspec_node &smspec_node);

double rd_sum_tstep_iget(const rd_sum_tstep_type *ministep, int index);
time_t rd_sum_tstep_get_sim_time(const rd_sum_tstep_type *ministep);
double rd_sum_tstep_get_sim_days(const rd_sum_tstep_type *ministep);
double rd_sum_tstep_get_sim_seconds(const rd_sum_tstep_type *ministep);

int rd_sum_tstep_get_report(const rd_sum_tstep_type *ministep);
int rd_sum_tstep_get_ministep(const rd_sum_tstep_type *ministep);

void rd_sum_tstep_fwrite(const rd_sum_tstep_type *ministep,
                         const int *index_map, int index_map_size,
                         fortio_type *fortio);
void rd_sum_tstep_iset(rd_sum_tstep_type *tstep, int index, float value);

/// scales with value; equivalent to iset( iget() * scalar)
void rd_sum_tstep_iscale(rd_sum_tstep_type *tstep, int index, float scalar);

/// adds addend to tstep[index]; equivalent to iset( iget() + addend)
void rd_sum_tstep_ishift(rd_sum_tstep_type *tstep, int index, float addend);

void rd_sum_tstep_set_from_key(rd_sum_tstep_type *tstep, const char *gen_key,
                               float value);
double rd_sum_tstep_get_from_key(const rd_sum_tstep_type *tstep,
                                 const char *gen_key);
bool rd_sum_tstep_has_key(const rd_sum_tstep_type *tstep, const char *gen_key);

bool rd_sum_tstep_sim_time_equal(const rd_sum_tstep_type *tstep1,
                                 const rd_sum_tstep_type *tstep2);

UTIL_SAFE_CAST_HEADER(rd_sum_tstep);
UTIL_SAFE_CAST_HEADER_CONST(rd_sum_tstep);

#ifdef __cplusplus
}

#endif
#endif
