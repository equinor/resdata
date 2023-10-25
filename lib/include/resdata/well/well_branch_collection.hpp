#ifndef ERT_WELL_BRANCH_COLLECTION_H
#define ERT_WELL_BRANCH_COLLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <ert/util/type_macros.hpp>

#include <resdata/well/well_segment.hpp>

typedef struct well_branch_collection_struct well_branch_collection_type;

well_branch_collection_type *well_branch_collection_alloc(void);
void well_branch_collection_free(well_branch_collection_type *branches);
void well_branch_collection_free__(void *arg);
bool well_branch_collection_has_branch(
    const well_branch_collection_type *branches, int branch_id);
int well_branch_collection_get_size(
    const well_branch_collection_type *branches);
const well_segment_type *well_branch_collection_iget_start_segment(
    const well_branch_collection_type *branches, int index);
const well_segment_type *well_branch_collection_get_start_segment(
    const well_branch_collection_type *branches, int branch_id);
bool well_branch_collection_add_start_segment(
    well_branch_collection_type *branches, well_segment_type *start_segment);

UTIL_IS_INSTANCE_HEADER(well_branch_collection);

#ifdef __cplusplus
}
#endif
#endif
