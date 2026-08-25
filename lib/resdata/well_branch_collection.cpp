
#include <memory>
#include <unordered_map>

#include <ert/util/type_macros.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_branch_collection.hpp>

#define WELL_BRANCH_COLLECTION_TYPE_ID 67177087

struct well_branch_collection_struct {
    UTIL_TYPE_ID_DECLARATION;

    std::unordered_map<int, std::shared_ptr<WellSegment>> start_segments;
};

UTIL_IS_INSTANCE_FUNCTION(well_branch_collection,
                          WELL_BRANCH_COLLECTION_TYPE_ID)

well_branch_collection_type *well_branch_collection_alloc() {
    well_branch_collection_type *branch_collection =
        new well_branch_collection_type();
    UTIL_TYPE_ID_INIT(branch_collection, WELL_BRANCH_COLLECTION_TYPE_ID);
    return branch_collection;
}

void well_branch_collection_free(well_branch_collection_type *branches) {
    delete branches;
}

int well_branch_collection_get_size(
    const well_branch_collection_type *branches) {
    return branches->start_segments.size();
}

bool well_branch_collection_has_branch(
    const well_branch_collection_type *branches, int branch_id) {
    return branches->start_segments.count(branch_id) > 0;
}

const std::shared_ptr<WellSegment> well_branch_collection_get_start_segment(
    const well_branch_collection_type *branches, int branch_id) {
    auto iter = branches->start_segments.find(branch_id);
    if (iter == branches->start_segments.end())
        return {nullptr};
    return iter->second;
}

bool well_branch_collection_add_start_segment(
    well_branch_collection_type *branches,
    std::shared_ptr<WellSegment> start_segment) {
    if ((start_segment->get_link_count() == 0) &&
        (start_segment->get_outlet())) {
        branches->start_segments[start_segment->get_branch_id()] =
            start_segment;
        return true;
    } else
        return false;
}
