#ifndef ERT_WELL_SEGMENT_H
#define ERT_WELL_SEGMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_rsthead.hpp>

#include <resdata/well/well_conn_collection.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_rseg_loader.hpp>

typedef struct well_segment_struct well_segment_type;

well_segment_type *
well_segment_alloc_from_kw(const rd_kw_type *iseg_kw,
                           const well_rseg_loader_type *rseg_loader,
                           const rd_rsthead_type *header, int well_nr,
                           int segment_index, int segment_id);
well_segment_type *well_segment_alloc(int segment_id, int outlet_segment_id,
                                      int branch_id, const double *rseg_data);
void well_segment_free(well_segment_type *segment);
void well_segment_free__(void *arg);

bool well_segment_active(const well_segment_type *segment);
bool well_segment_main_stem(const well_segment_type *segment);
bool well_segment_nearest_wellhead(const well_segment_type *segment);

int well_segment_get_link_count(const well_segment_type *segment);
int well_segment_get_branch_id(const well_segment_type *segment);
int well_segment_get_outlet_id(const well_segment_type *segment);
int well_segment_get_id(const well_segment_type *segment);
well_segment_type *well_segment_get_outlet(const well_segment_type *segment);
bool well_segment_link(well_segment_type *segment,
                       well_segment_type *outlet_segment);
void well_segment_link_strict(well_segment_type *segment,
                              well_segment_type *outlet_segment);
bool well_segment_has_grid_connections(const well_segment_type *segment,
                                       const char *grid_name);
bool well_segment_has_global_grid_connections(const well_segment_type *segment);
bool well_segment_add_connection(well_segment_type *segment,
                                 const char *grid_name, well_conn_type *conn);
const well_conn_collection_type *
well_segment_get_connections(const well_segment_type *segment,
                             const char *grid_name);
const well_conn_collection_type *
well_segment_get_global_connections(const well_segment_type *segment);
bool well_segment_well_is_MSW(int well_nr, const rd_kw_type *iwel_kw,
                              const rd_rsthead_type *rst_head);

double well_segment_get_depth(const well_segment_type *segment);
double well_segment_get_length(const well_segment_type *segment);
double well_segment_get_total_length(const well_segment_type *segment);
double well_segment_get_diameter(const well_segment_type *segment);

UTIL_IS_INSTANCE_HEADER(well_segment);

#ifdef __cplusplus
}
#endif
#endif
