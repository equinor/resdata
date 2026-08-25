#pragma once
#include <cstddef>
#include <memory>
#include <tuple>
#include <set>
#include <vector>

#include <ert/geometry/geo_polygon_collection.hpp>

#include <resdata/rd_grid.hpp>

/* fault_block_layer_type is fully defined in fault_block_layer.hpp; a
   FaultBlock only needs to keep a pointer back to its parent layer. */
typedef struct fault_block_layer_struct fault_block_layer_type;

/** A single (i,j,k) cell of a FaultBlock, together with its (x,y,z)
    cell-center coordinates. */
struct FaultBlockCell {
    size_t i, j, k;
    double x, y, z;
};

/** A FaultBlock instance is a collection of cells - identified by (i,j)
    pairs - in one particular layer (i.e. one k value) which are considered
    to be one connected unit.*/
class FaultBlock {
    void assert_center();
    [[nodiscard]] bool
    neighbour_xpolyline(size_t i1, size_t j1, size_t i2, size_t j2,
                        const geo_polygon_collection_type *polylines) const;
    [[nodiscard]] bool
    connected_neighbour(size_t i1, size_t j1, size_t i2, size_t j2,
                        bool connected_only,
                        const geo_polygon_collection_type *polylines) const;

    rd_grid_type *grid;
    fault_block_layer_type *parent_layer; // nullptr if detached
    std::vector<std::tuple<size_t, size_t>> index_list;
    std::set<int> regions;
    size_t block_id;
    size_t k;
    double xc = 0, yc = 0;
    bool valid_center = false;

    /** If the parent_layer no longer exists, the FaultBlock becomes
        detached and cannot be used for layer wide operations */
    void detach() { parent_layer = nullptr; };
    friend fault_block_layer_struct;

public:
    FaultBlock(fault_block_layer_type *parent_layer, size_t block_id);

    [[nodiscard]] bool is_detached() const { return parent_layer == nullptr; };
    [[nodiscard]] size_t get_size() const { return index_list.size(); }
    [[nodiscard]] size_t get_id() const;
    double get_xc();
    double get_yc();
    [[nodiscard]] FaultBlockCell export_cell(size_t index) const;
    void assign_to_region(int region_id);
    [[nodiscard]] const std::set<int> &get_regions() const { return regions; };
    /** add_cell cannot be called on a detached FaultBlock */
    void add_cell(size_t i, size_t j);
    [[nodiscard]] const std::vector<int> get_global_index_list() const;
    void copy_content(const FaultBlock &src_block);

    /** Traces the outer edge of this block and returns the list of
        (x, y, cell_index) corner points that make up its boundary polygon,
        where cell_index is the index of the cell at that corner.

        Cannot be called on a detached FaultBlock. */
    [[nodiscard]] std::vector<std::tuple<double, double, int>>
    trace_edge() const;
    [[nodiscard]] std::vector<std::shared_ptr<FaultBlock>>
    /** get_neighbours cannot be called on a detached FaultBlock */
    get_neighbours(bool connected_only,
                   const geo_polygon_collection_type *polylines) const;
};
