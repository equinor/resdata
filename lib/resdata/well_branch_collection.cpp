#include <stdbool.h>

#include <vector>

#include <ert/util/util.hpp>
#include <ert/util/type_macros.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_branch_collection.hpp>

#define WELL_BRANCH_COLLECTION_TYPE_ID 67177087

struct well_branch_collection_struct {
    UTIL_TYPE_ID_DECLARATION;

    std::vector<well_segment_type *> __start_segments;
    std::vector<int> index_map;
};

UTIL_IS_INSTANCE_FUNCTION(well_branch_collection,
                          WELL_BRANCH_COLLECTION_TYPE_ID)
static UTIL_SAFE_CAST_FUNCTION(well_branch_collection,
                               WELL_BRANCH_COLLECTION_TYPE_ID)

    well_branch_collection_type *well_branch_collection_alloc() {
    well_branch_collection_type *branch_collection =
        new well_branch_collection_type();
    UTIL_TYPE_ID_INIT(branch_collection, WELL_BRANCH_COLLECTION_TYPE_ID);
    return branch_collection;
}

namespace {

int well_branch_collection_safe_iget_index(
    const well_branch_collection_type *branches, int index) {
    if (index >= (int)branches->index_map.size())
        return -1;
    else
        return branches->index_map[index];
}

} // namespace

void well_branch_collection_free(well_branch_collection_type *branches) {
    delete branches;
}

int well_branch_collection_get_size(
    const well_branch_collection_type *branches) {
    return branches->__start_segments.size();
}

bool well_branch_collection_has_branch(
    const well_branch_collection_type *branches, int branch_id) {
    if (well_branch_collection_safe_iget_index(branches, branch_id) >= 0)
        return true;
    else
        return false;
}

const well_segment_type *well_branch_collection_iget_start_segment(
    const well_branch_collection_type *branches, int index) {
    if (index < static_cast<int>(branches->__start_segments.size()))
        return branches->__start_segments[index];
    else
        return NULL;
}

const well_segment_type *well_branch_collection_get_start_segment(
    const well_branch_collection_type *branches, int branch_id) {
    int internal_index =
        well_branch_collection_safe_iget_index(branches, branch_id);
    if (internal_index >= 0)
        return well_branch_collection_iget_start_segment(branches,
                                                         internal_index);
    else
        return NULL;
}

bool well_branch_collection_add_start_segment(
    well_branch_collection_type *branches, well_segment_type *start_segment) {
    if ((well_segment_get_link_count(start_segment) == 0) &&
        (well_segment_get_outlet(start_segment))) {
        int branch_id = well_segment_get_branch_id(start_segment);
        int current_index =
            well_branch_collection_safe_iget_index(branches, branch_id);
        if (current_index >= 0)
            branches->__start_segments[current_index] = start_segment;
        else {
            int new_index = branches->__start_segments.size();
            branches->__start_segments.push_back(start_segment);
            if (branch_id >= (int)branches->index_map.size())
                branches->index_map.resize(branch_id + 1, -1);
            branches->index_map[branch_id] = new_index;
        }

        return true;
    } else
        return false;
}
