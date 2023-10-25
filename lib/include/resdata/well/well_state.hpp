#ifndef ERT_WELL_STATE_H
#define ERT_WELL_STATE_H

#include <time.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn_collection.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_branch_collection.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#define GLOBAL_GRID_NAME                                                       \
    "GLOBAL" // The name assigned to the global grid for name based lookup.

typedef struct well_state_struct well_state_type;

well_state_type *well_state_alloc(const char *well_name, int global_well_nr,
                                  bool open, well_type_enum type, int report_nr,
                                  time_t valid_from);
well_state_type *well_state_alloc_from_file(rd_file_type *rd_file,
                                            const rd_grid_type *grid,
                                            int report_step, int well_nr,
                                            bool load_segment_information);
well_state_type *well_state_alloc_from_file2(rd_file_view_type *file_view,
                                             const rd_grid_type *grid,
                                             int report_nr, int global_well_nr,
                                             bool load_segment_information);

void well_state_add_connections2(well_state_type *well_state,
                                 const rd_grid_type *grid,
                                 rd_file_view_type *rst_view, int well_nr);

void well_state_add_connections(well_state_type *well_state,
                                const rd_grid_type *grid,
                                rd_file_type *rst_file, int well_nr);

bool well_state_add_MSW(well_state_type *well_state, rd_file_type *rst_file,
                        int well_nr, bool load_segment_information);

bool well_state_add_MSW2(well_state_type *well_state,
                         rd_file_view_type *rst_view, int well_nr,
                         bool load_segment_information);

bool well_state_is_MSW(const well_state_type *well_state);

bool well_state_has_segment_data(const well_state_type *well_state);

well_segment_collection_type *
well_state_get_segments(const well_state_type *well_state);
well_branch_collection_type *
well_state_get_branches(const well_state_type *well_state);

void well_state_free(well_state_type *well);
const char *well_state_get_name(const well_state_type *well);
int well_state_get_report_nr(const well_state_type *well_state);
time_t well_state_get_sim_time(const well_state_type *well_state);
well_type_enum well_state_get_type(const well_state_type *well_state);
bool well_state_is_open(const well_state_type *well_state);
int well_state_get_well_nr(const well_state_type *well_state);

const well_conn_type *
well_state_get_global_wellhead(const well_state_type *well_state);
const well_conn_type *
well_state_iget_wellhead(const well_state_type *well_state, int grid_nr);
const well_conn_type *well_state_get_wellhead(const well_state_type *well_state,
                                              const char *grid_name);

well_type_enum well_state_translate_rd_type_int(int int_type);

const well_conn_collection_type *
well_state_get_grid_connections(const well_state_type *well_state,
                                const char *grid_name);
const well_conn_collection_type *
well_state_get_global_connections(const well_state_type *well_state);
bool well_state_has_grid_connections(const well_state_type *well_state,
                                     const char *grid_name);
bool well_state_has_global_connections(const well_state_type *well_state);

double well_state_get_oil_rate(const well_state_type *well_state);
double well_state_get_gas_rate(const well_state_type *well_state);
double well_state_get_water_rate(const well_state_type *well_state);
double well_state_get_volume_rate(const well_state_type *well_state);
double well_state_get_water_rate_si(const well_state_type *well_state);
double well_state_get_oil_rate_si(const well_state_type *well_state);
double well_state_get_volume_rate_si(const well_state_type *well_state);
double well_state_get_gas_rate_si(const well_state_type *well_state);

UTIL_IS_INSTANCE_HEADER(well_state);

#ifdef __cplusplus
}
#endif

#endif
