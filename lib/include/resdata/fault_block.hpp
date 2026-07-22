#pragma once
#include <memory>
#include <tuple>
#include <vector>

#include <ert/util/int_vector.hpp>
#include <ert/util/double_vector.hpp>

#include <ert/geometry/geo_polygon_collection.hpp>

#include <resdata/rd_grid.hpp>

/* fault_block_layer_type is fully defined in fault_block_layer.hpp; a
   FaultBlock only needs to keep a pointer back to its parent layer. */
typedef struct fault_block_layer_struct fault_block_layer_type;

/** A single (i,j,k) cell of a FaultBlock, together with its (x,y,z)
    cell-center coordinates. */
struct FaultBlockCell {
    int i, j, k;
    double x, y, z;
};

/** A FaultBlock instance is a collection of cells - identified by (i,j)
    pairs - in one particular layer (i.e. one k value) which are considered
    to be one connected unit.*/
class FaultBlock {
    void assert_center();
    [[nodiscard]] int iget_i(int index) const;
    [[nodiscard]] int iget_j(int index) const;
    [[nodiscard]] bool
    neighbour_xpolyline(int i1, int j1, int i2, int j2,
                        const geo_polygon_collection_type *polylines) const;
    [[nodiscard]] bool
    connected_neighbour(int i1, int j1, int i2, int j2, bool connected_only,
                        const geo_polygon_collection_type *polylines) const;

    rd_grid_type *grid;
    fault_block_layer_type *parent_layer; // nullptr if detached
    int_vector_ptr i_list = make_int_vector(0, 0);
    int_vector_ptr j_list = make_int_vector(0, 0);
    int_vector_ptr global_index_list = make_int_vector(0, 0);
    int_vector_ptr region_list = make_int_vector(0, 0);
    int block_id;
    int k;
    double xc = 0, yc = 0;
    bool valid_center = false;

    /** If the parent_layer no longer exists, the FaultBlock becomes
        detached and cannot be used for layer wide operations */
    void detach() { parent_layer = nullptr; };
    friend fault_block_layer_struct;

public:
    FaultBlock(fault_block_layer_type *parent_layer, int block_id);

    [[nodiscard]] bool is_detached() const { return parent_layer == nullptr; };
    [[nodiscard]] int get_size() const;
    [[nodiscard]] int get_id() const;
    double get_xc();
    double get_yc();
    [[nodiscard]] FaultBlockCell export_cell(int index) const;
    void assign_to_region(int region_id);
    [[nodiscard]] const int_vector_type *get_region_list() const;
    /** add_cell cannot be called on a detached FaultBlock */
    void add_cell(int i, int j);
    [[nodiscard]] const int_vector_type *get_global_index_list() const;
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
