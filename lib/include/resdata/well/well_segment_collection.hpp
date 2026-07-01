#pragma once
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_kw.hpp>

#include <vector>
#include <memory>

#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_branch_collection.hpp>
#include <resdata/well/well_rseg_loader.hpp>
#include <resdata/well/well_conn.hpp>

typedef struct well_segment_collection_struct well_segment_collection_type;

well_segment_collection_type *well_segment_collection_alloc();
void well_segment_collection_free(
    well_segment_collection_type *segment_collection);
int well_segment_collection_get_size(
    const well_segment_collection_type *segment_collection);
void well_segment_collection_add(
    well_segment_collection_type *segment_collection,
    std::shared_ptr<WellSegment> segment);
bool well_segment_collection_has_segment(
    const well_segment_collection_type *segment_collection, int segment_id);
std::shared_ptr<WellSegment> well_segment_collection_get(
    const well_segment_collection_type *segment_collection, int segment_id);
std::shared_ptr<WellSegment> well_segment_collection_iget(
    const well_segment_collection_type *segment_collection, int index);
void well_segment_collection_link(
    const well_segment_collection_type *segment_collection);
void well_segment_collection_add_branches(
    const well_segment_collection_type *segment_collection,
    well_branch_collection_type *branches);

int well_segment_collection_load_from_kw(
    well_segment_collection_type *segment_collection, int well_nr,
    const rd_kw_type *iwel_kw, const rd_kw_type *iseg_kw,
    const well_rseg_loader_type *rseg_loader, const RSTHead &rst_head,
    bool load_segment_information, bool *is_MSW_well);
void well_segment_collection_add_connections(
    well_segment_collection_type *segment_collection, const char *grid_name,
    const std::vector<std::shared_ptr<WellConnection>> &connections);

using well_segment_collection_ptr =
    std::unique_ptr<well_segment_collection_type,
                    decltype(&well_segment_collection_free)>;
