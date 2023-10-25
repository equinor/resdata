#ifndef ERT_WELL_TS_H
#define ERT_WELL_TS_H

#include <resdata/well/well_state.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct well_ts_struct well_ts_type;

void well_ts_free(well_ts_type *well_ts);
void well_ts_add_well(well_ts_type *well_ts, well_state_type *well_state);
well_ts_type *well_ts_alloc(const char *well_name);
void well_ts_free__(void *arg);
well_state_type *well_ts_get_state_from_sim_time(const well_ts_type *well_ts,
                                                 time_t sim_time);
well_state_type *well_ts_get_state_from_report(const well_ts_type *well_ts,
                                               int report_nr);
well_state_type *well_ts_iget_state(const well_ts_type *well_ts, int index);
int well_ts_get_size(const well_ts_type *well_ts);
const char *well_ts_get_name(const well_ts_type *well_ts);
well_state_type *well_ts_get_first_state(const well_ts_type *well_ts);
well_state_type *well_ts_get_last_state(const well_ts_type *well_ts);

#ifdef __cplusplus
}
#endif

#endif
