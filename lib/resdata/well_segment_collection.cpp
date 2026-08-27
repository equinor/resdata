
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

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
    /** Maps a segment id onto its position in __segment_storage. Segment ids
        originate from the ISEG/ICON keywords and are signed values read from
        file, so they are used as a key rather than as an index. */
    std::unordered_map<int, size_t> segment_index_map;
    std::vector<std::shared_ptr<WellSegment>> __segment_storage;
};

namespace {
/** Look up the storage index of @segment_id, if such a segment has been
    added. */
std::optional<size_t>
lookup_index(const well_segment_collection_type *segment_collection,
             int segment_id) {
    auto iter = segment_collection->segment_index_map.find(segment_id);
    if (iter == segment_collection->segment_index_map.end())
        return std::nullopt;
    return iter->second;
}
} // namespace

well_segment_collection_type *well_segment_collection_alloc() {
    return new well_segment_collection_type();
}

void well_segment_collection_free(
    well_segment_collection_type *segment_collection) {
    delete segment_collection;
}

size_t well_segment_collection_get_size(
    const well_segment_collection_type *segment_collection) {
    return segment_collection->__segment_storage.size();
}

void well_segment_collection_add(
    well_segment_collection_type *segment_collection,
    std::shared_ptr<WellSegment> segment) {
    const int segment_id = segment->get_id();

    auto current_index = lookup_index(segment_collection, segment_id);
    if (current_index) {
        segment_collection->__segment_storage[*current_index] = segment;
    } else {
        size_t new_index = segment_collection->__segment_storage.size();
        segment_collection->__segment_storage.push_back(segment);
        segment_collection->segment_index_map[segment_id] = new_index;
    }
}

std::shared_ptr<WellSegment> well_segment_collection_iget(
    const well_segment_collection_type *segment_collection, size_t index) {
    return segment_collection->__segment_storage.at(index);
}

std::shared_ptr<WellSegment> well_segment_collection_get(
    const well_segment_collection_type *segment_collection, int segment_id) {
    auto internal_index = lookup_index(segment_collection, segment_id);
    if (internal_index)
        return well_segment_collection_iget(segment_collection,
                                            *internal_index);
    else
        return {nullptr};
}

bool well_segment_collection_has_segment(
    const well_segment_collection_type *segment_collection, int segment_id) {
    return lookup_index(segment_collection, segment_id).has_value();
}

int well_segment_collection_load_from_kw(
    well_segment_collection_type *segment_collection, size_t well_nr,
    const rd_kw_type *iwel_kw, const rd_kw_type *iseg_kw,
    well_rseg_loader_type *rseg_loader, const RSTHead &rst_head,
    bool load_segments, bool *is_MSW_well) {
    size_t iwel_offset = rst_head.get_niwelz() * well_nr;
    int segment_well_nr =
        rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_SEGMENTED_WELL_NR_INDEX) - 1;
    int segments_added = 0;

    if (segment_well_nr != IWEL_SEGMENTED_WELL_NR_NORMAL_VALUE) {
        *is_MSW_well = true;

        if (segment_well_nr < 0)
            throw std::invalid_argument(
                fmt::format("Invalid segmented well number {} read from {}",
                            segment_well_nr + 1, IWEL_KW));

        if (load_segments) {
            for (size_t segment_index = 0;
                 segment_index < rst_head.get_nsegmx(); segment_index++) {
                int segment_id =
                    static_cast<int>(segment_index) + WELL_SEGMENT_OFFSET;
                auto segment =
                    WellSegment::from_kw(iseg_kw, rseg_loader, rst_head,
                                         static_cast<size_t>(segment_well_nr),
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
        if (!segment->is_nearest_wellhead()) {
            auto outlet_index =
                lookup_index(segment_collection, segment->get_outlet_id());
            segment->link(
                outlet_index
                    ? segment_collection->__segment_storage[*outlet_index].get()
                    : nullptr);
        }
    }
}

void well_segment_collection_add_connections(
    well_segment_collection_type *segment_collection, const char *grid_name,
    const std::vector<std::shared_ptr<WellConnection>> &connections) {
    for (const auto &conn : connections) {
        if (conn->is_MSW()) {
            auto index =
                lookup_index(segment_collection, conn->get_segment_id());
            if (index)
                segment_collection->__segment_storage[*index]->add_connection(
                    grid_name, conn);
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
