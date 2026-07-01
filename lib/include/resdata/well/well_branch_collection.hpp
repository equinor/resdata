#pragma once
#include <memory>

#include <ert/util/type_macros.hpp>

#include <resdata/well/well_segment.hpp>

typedef struct well_branch_collection_struct well_branch_collection_type;

well_branch_collection_type *well_branch_collection_alloc();
void well_branch_collection_free(well_branch_collection_type *branches);
bool well_branch_collection_has_branch(
    const well_branch_collection_type *branches, int branch_id);
int well_branch_collection_get_size(
    const well_branch_collection_type *branches);
const std::shared_ptr<WellSegment> well_branch_collection_iget_start_segment(
    const well_branch_collection_type *branches, int index);
const std::shared_ptr<WellSegment> well_branch_collection_get_start_segment(
    const well_branch_collection_type *branches, int branch_id);
bool well_branch_collection_add_start_segment(
    well_branch_collection_type *branches,
    std::shared_ptr<WellSegment> start_segment);

UTIL_IS_INSTANCE_HEADER(well_branch_collection);

using well_branch_collection_ptr =
    std::unique_ptr<well_branch_collection_type,
                    decltype(&well_branch_collection_free)>;
