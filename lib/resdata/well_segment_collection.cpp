
#include <memory>
#include <vector>

#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_rsthead.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_branch_collection.hpp>

struct well_segment_collection_struct {
    std::vector<int> segment_index_map;
    std::vector<std::shared_ptr<WellSegment>> __segment_storage;
};

well_segment_collection_type *well_segment_collection_alloc() {
    return new well_segment_collection_type();
}

void well_segment_collection_free(
    well_segment_collection_type *segment_collection) {
    delete segment_collection;
}

int well_segment_collection_get_size(
    const well_segment_collection_type *segment_collection) {
    return segment_collection->__segment_storage.size();
}

void well_segment_collection_add(
    well_segment_collection_type *segment_collection,
    std::shared_ptr<WellSegment> segment) {
    int segment_id = segment->get_id();
    int current_index = -1;
    if (segment_id <
        static_cast<int>(segment_collection->segment_index_map.size()))
        current_index = segment_collection->segment_index_map[segment_id];
    if (current_index >= 0) {
        segment_collection->__segment_storage[current_index] = segment;
    } else {
        int new_index = segment_collection->__segment_storage.size();
        segment_collection->__segment_storage.push_back(segment);
        if (segment_id >=
            static_cast<int>(segment_collection->segment_index_map.size()))
            segment_collection->segment_index_map.resize(segment_id + 1, -1);
        segment_collection->segment_index_map[segment_id] = new_index;
    }
}

std::shared_ptr<WellSegment> well_segment_collection_iget(
    const well_segment_collection_type *segment_collection, int index) {
    return segment_collection->__segment_storage[index];
}

std::shared_ptr<WellSegment> well_segment_collection_get(
    const well_segment_collection_type *segment_collection, int segment_id) {
    int internal_index = -1;
    if (segment_id <
        static_cast<int>(segment_collection->segment_index_map.size()))
        internal_index = segment_collection->segment_index_map[segment_id];
    if (internal_index >= 0)
        return well_segment_collection_iget(segment_collection, internal_index);
    else
        return {nullptr};
}

bool well_segment_collection_has_segment(
    const well_segment_collection_type *segment_collection, int segment_id) {
    int internal_index = -1;
    if (segment_id <
        static_cast<int>(segment_collection->segment_index_map.size()))
        internal_index = segment_collection->segment_index_map[segment_id];
    if (internal_index >= 0)
        return true;
    else
        return false;
}

int well_segment_collection_load_from_kw(
    well_segment_collection_type *segment_collection, int well_nr,
    const rd_kw_type *iwel_kw, const rd_kw_type *iseg_kw,
    const well_rseg_loader_type *rseg_loader, const RSTHead &rst_head,
    bool load_segments, bool *is_MSW_well) {

    int iwel_offset = rst_head.niwelz * well_nr;
    int segment_well_nr =
        rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_SEGMENTED_WELL_NR_INDEX) - 1;
    int segments_added = 0;

    if (segment_well_nr != IWEL_SEGMENTED_WELL_NR_NORMAL_VALUE) {
        *is_MSW_well = true;

        if (load_segments) {
            for (int segment_index = 0; segment_index < rst_head.nsegmx;
                 segment_index++) {
                int segment_id = segment_index + WELL_SEGMENT_OFFSET;
                auto segment = WellSegment::from_kw(iseg_kw, rseg_loader,
                                                    rst_head, segment_well_nr,
                                                    segment_index, segment_id);

                if (segment->is_active()) {
                    well_segment_collection_add(segment_collection, segment);
                    segments_added++;
                }
            }
        }
    }
    return segments_added;
}

void well_segment_collection_link(
    const well_segment_collection_type *segment_collection) {
    for (const auto &segment : segment_collection->__segment_storage) {
        int outlet_segment_id = segment->get_outlet_id();
        if (!segment->is_nearest_wellhead()) {
            auto target_segment = well_segment_collection_get(
                segment_collection, outlet_segment_id);
            segment->link(target_segment.get());
        }
    }
}

void well_segment_collection_add_connections(
    well_segment_collection_type *segment_collection, const char *grid_name,
    const std::vector<std::shared_ptr<WellConnection>> &connections) {
    for (const auto &conn : connections) {
        if (conn->is_MSW()) {
            int segment_id = conn->get_segment_id();
            auto segment =
                well_segment_collection_get(segment_collection, segment_id);
            segment->add_connection(grid_name, conn);
        }
    }
}

void well_segment_collection_add_branches(
    const well_segment_collection_type *segment_collection,
    well_branch_collection_type *branches) {
    for (const auto &segment : segment_collection->__segment_storage) {
        if (segment->get_link_count() == 0)
            well_branch_collection_add_start_segment(branches, segment);
    }
}
