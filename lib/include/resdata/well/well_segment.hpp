#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_rseg_loader.hpp>

class WellSegment {
    int link_count = 0;
    int segment_id;
    int branch_id;
    int outlet_segment_id; // This is in the global index space given by the ISEG keyword.
    WellSegment *outlet_segment = nullptr;
    std::map<std::string, std::vector<std::shared_ptr<WellConnection>>>
        connections;

    double
        depth; // The depth of the segment node; furthest away from the wellhead.
    double length;
    double total_length; // Total length from wellhead.
    double diameter;     // The tube diametere available for flow.
public:
    WellSegment(int segment_id, int outlet_segment_id, int branch_id,
                double depth = 0.0, double length = 0.0,
                double total_length = 0.0, double diameter = 0.0)
        : segment_id(segment_id), branch_id(branch_id),
          outlet_segment_id(outlet_segment_id), depth(depth), length(length),
          total_length(total_length), diameter(diameter) {};
    static std::shared_ptr<WellSegment>
    from_kw(const rd_kw_type *iseg_kw, const well_rseg_loader_type *rseg_loader,
            const RSTHead &header, int well_nr, int segment_index,
            int segment_id);
    [[nodiscard]] bool is_active() const {
        return branch_id != WELL_SEGMENT_BRANCH_INACTIVE_VALUE;
    }
    [[nodiscard]] bool is_main_stem() const {
        return branch_id == WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE;
    }
    [[nodiscard]] bool is_nearest_wellhead() const {
        return outlet_segment_id == WELL_SEGMENT_OUTLET_END_VALUE;
    }
    [[nodiscard]] int get_link_count() const { return link_count; }
    [[nodiscard]] int get_branch_id() const { return branch_id; }
    [[nodiscard]] int get_outlet_id() const { return outlet_segment_id; }
    [[nodiscard]] WellSegment *get_outlet() const { return outlet_segment; }
    [[nodiscard]] int get_id() const { return segment_id; }
    [[nodiscard]] double get_depth() const { return depth; }
    [[nodiscard]] double get_length() const { return length; }
    [[nodiscard]] double get_total_length() const { return total_length; }
    [[nodiscard]] WellSegment *get_outlet_segment() const {
        return outlet_segment;
    }
    [[nodiscard]] double get_diameter() const { return diameter; }
    bool link(WellSegment *outlet_segment);
    [[nodiscard]] bool
    has_grid_connections(const std::string &grid_name) const {
        return connections.find(grid_name) != connections.end();
    }
    [[nodiscard]] bool has_global_grid_connections() const {
        return has_grid_connections(RD_GRID_GLOBAL_GRID);
    }
    bool add_connection(const std::string &grid_name,
                        std::shared_ptr<WellConnection> conn);
    const std::vector<std::shared_ptr<WellConnection>> *
    get_connections(const std::string &grid_name) {
        auto it = connections.find(grid_name);
        return it != connections.end() ? &it->second : nullptr;
    }
    const std::vector<std::shared_ptr<WellConnection>> *
    get_global_connections() {
        return get_connections(RD_GRID_GLOBAL_GRID);
    }
};

bool well_segment_well_is_MSW(int well_nr, const rd_kw_type *iwel_kw,
                              const RSTHead &rst_head);
