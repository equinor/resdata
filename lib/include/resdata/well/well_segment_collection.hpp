#ifndef ERT_WELL_SEGMENT_COLLECTION_H
#define ERT_WELL_SEGMENT_COLLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <resdata/rd_kw.hpp>

#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_conn_collection.hpp>
#include <resdata/well/well_branch_collection.hpp>
#include <resdata/well/well_rseg_loader.hpp>

typedef struct well_segment_collection_struct well_segment_collection_type;

well_segment_collection_type *well_segment_collection_alloc(void);
void well_segment_collection_free(
    well_segment_collection_type *segment_collection);
int well_segment_collection_get_size(
    const well_segment_collection_type *segment_collection);
void well_segment_collection_add(
    well_segment_collection_type *segment_collection,
    well_segment_type *segment);
bool well_segment_collection_has_segment(
    const well_segment_collection_type *segment_collection, int segment_id);
well_segment_type *well_segment_collection_get(
    const well_segment_collection_type *segment_collection, int segment_id);
well_segment_type *well_segment_collection_iget(
    const well_segment_collection_type *segment_collection, int index);
int well_segment_collection_load_from_kw(
    well_segment_collection_type *segment_collection, int well_nr,
    const rd_kw_type *iwel_kw, const rd_kw_type *iseg_kw,
    const well_rseg_loader_type *rseg_loader, const rd_rsthead_type *rst_head,
    bool load_segment_information, bool *is_MSW_well);

void well_segment_collection_link(
    const well_segment_collection_type *segment_collection);
void well_segment_collection_add_connections(
    well_segment_collection_type *segment_collection, const char *grid_name,
    const well_conn_collection_type *connections);
void well_segment_collection_add_branches(
    const well_segment_collection_type *segment_collection,
    well_branch_collection_type *branches);

#ifdef __cplusplus
}
#endif
#endif
