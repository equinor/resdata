#include <cstddef>
#include <ert/util/type_macros.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/util.hpp>

#include <fmt/core.h>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/fault_block_layer.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/layer.hpp>
#include <resdata/rd_type.hpp>

#include <memory>
#include <stdexcept>
#include <vector>
#include <fmt/format.h>

#define FAULT_BLOCK_LAYER_ID 2297476

struct fault_block_layer_struct {
    UTIL_TYPE_ID_DECLARATION;
    rd_grid_type *grid;
    std::vector<int> block_map;
    layer_ptr layer{nullptr, &layer_free};
    int k;
    /* Blocks are stored as shared_ptr so that a reference to a
       FaultBlock can outlive its removal from this vector via
       remove_block(). */
    std::vector<std::shared_ptr<FaultBlock>> blocks;

    ~fault_block_layer_struct() {
        for (auto &b : blocks)
            b->detach();
    }

    void remove_block(int block_id) {
        int storage_index = get_block(block_id);
        if (storage_index >= 0) {
            block_map.at(block_id) = -1;
            blocks.at(storage_index)->detach();
            blocks.erase(blocks.begin() + storage_index);
            for (int &index : block_map) {
                if (index > storage_index)
                    index -= 1;
            }
        }
    }

    [[nodiscard]] int get_block(int index) const {
        if (index >= static_cast<int>(block_map.size()))
            return -1;
        else {
            if (index >= 0)
                return block_map[index];
            else {
                throw std::out_of_range(
                    fmt::format("index:{} is invalid - only accepts positive "
                                "indices",
                                index));
            }
        }
    }
};

UTIL_IS_INSTANCE_FUNCTION(fault_block_layer, FAULT_BLOCK_LAYER_ID);

std::shared_ptr<FaultBlock>
fault_block_layer_add_block(fault_block_layer_type *layer, int block_id) {
    if (layer->get_block(block_id) < 0) {
        auto block = std::make_shared<FaultBlock>(layer, block_id);
        int storage_index = layer->blocks.size();

        if (block_id >= static_cast<int>(layer->block_map.size()))
            layer->block_map.resize(block_id + 1, -1);
        layer->blocks.push_back(block);
        layer->block_map[block_id] = storage_index;

        return block;
    } else
        return nullptr;
}

void fault_block_layer_scan_layer(fault_block_layer_type *fault_layer,
                                  layer_type *layer) {
    auto i_list = make_int_vector(0, 0);
    auto j_list = make_int_vector(0, 0);

    for (int j = 0; j < layer_get_ny(layer); j++) {
        for (int i = 0; i < layer_get_nx(layer); i++) {
            int cell_value = layer_iget_cell_value(layer, i, j);
            if (cell_value != 0) {
                layer_trace_block_content(layer, true, i, j, cell_value,
                                          i_list.get(), j_list.get());
                {
                    int block_id = fault_block_layer_get_next_id(fault_layer);
                    auto fault_block =
                        fault_block_layer_add_block(fault_layer, block_id);
                    for (int c = 0; c < int_vector_size(i_list.get()); c++)
                        fault_block->add_cell(int_vector_iget(i_list.get(), c),
                                              int_vector_iget(j_list.get(), c));
                }
            }
        }
    }
}

/*
  Observe that the id values from the rd_kw instance are not
  retained; the fault_block_layer instance gets new block id numbers,
  including a nonzero value for the cells which have value zero in the
  keyword.


   - The blocks in the fault_block_layer instance are guaranteed to be
     singly connected.

   - Observe that the numbering of the blocks is implicitly given by
     the spatial distribution of the values, and hence *completely
     random* with respect to the original values in the keyword.

*/

bool fault_block_layer_scan_kw(fault_block_layer_type *layer,
                               const rd_kw_type *fault_block_kw) {
    bool assign_zero = true;

    if (rd_kw_get_size(fault_block_kw) != rd_grid_get_global_size(layer->grid))
        return false;
    else if (!rd_type_is_int(rd_kw_get_data_type(fault_block_kw)))
        return false;
    else {
        int max_block_id = 0;
        auto work_layer = make_layer(rd_grid_get_nx(layer->grid),
                                     rd_grid_get_ny(layer->grid));

        for (int j = 0; j < rd_grid_get_ny(layer->grid); j++) {
            for (int i = 0; i < rd_grid_get_nx(layer->grid); i++) {
                int g = rd_grid_get_global_index3(layer->grid, i, j, layer->k);
                int block_id = rd_kw_iget_int(fault_block_kw, g);

                if (block_id > 0) {
                    layer_iset_cell_value(work_layer.get(), i, j, block_id);
                    max_block_id = util_int_max(block_id, max_block_id);
                }
            }
        }

        if (assign_zero)
            layer_replace_cell_values(work_layer.get(), 0, max_block_id + 1);

        fault_block_layer_scan_layer(layer, work_layer.get());
        return true;
    }
}

/**
   This function will just load the fault block distribution from
   fault_block_kw; it will not do any reordering or assign block ids
   to the regions with ID == 0.
*/

bool fault_block_layer_load_kw(fault_block_layer_type *layer,
                               const rd_kw_type *fault_block_kw) {
    if (rd_kw_get_size(fault_block_kw) != rd_grid_get_global_size(layer->grid))
        return false;
    else if (!rd_type_is_int(rd_kw_get_data_type(fault_block_kw)))
        return false;
    else {
        for (int j = 0; j < rd_grid_get_ny(layer->grid); j++) {
            for (int i = 0; i < rd_grid_get_nx(layer->grid); i++) {
                int g = rd_grid_get_global_index3(layer->grid, i, j, layer->k);
                int block_id = rd_kw_iget_int(fault_block_kw, g);
                if (block_id > 0) {
                    fault_block_layer_add_block(layer, block_id);
                    {
                        auto fault_block =
                            fault_block_layer_get_block(layer, block_id);
                        fault_block->add_cell(i, j);
                    }
                }
            }
        }

        return true;
    }
}

fault_block_layer_type *fault_block_layer_alloc(rd_grid_type *grid, int k) {
    if ((k < 0) || (k >= rd_grid_get_nz(grid)))
        return NULL;
    else {
        std::unique_ptr<fault_block_layer_type> layer(
            new fault_block_layer_type);
        UTIL_TYPE_ID_INIT(layer.get(), FAULT_BLOCK_LAYER_ID);
        layer->grid = grid;
        layer->k = k;
        layer->block_map = std::vector<int>(0);
        layer->blocks = std::vector<std::shared_ptr<FaultBlock>>();
        layer->layer.reset(
            layer_alloc(rd_grid_get_nx(grid), rd_grid_get_ny(grid)));

        return layer.release();
    }
}

std::shared_ptr<FaultBlock>
fault_block_layer_iget_block(const fault_block_layer_type *layer,
                             int storage_index) {
    return layer->blocks.at(storage_index);
}

std::shared_ptr<FaultBlock>
fault_block_layer_get_block(const fault_block_layer_type *layer, int block_id) {
    int storage_index = layer->get_block(block_id);
    if (storage_index < 0)
        return nullptr;
    else
        return layer->blocks.at(storage_index);
}

std::shared_ptr<FaultBlock>
fault_block_layer_safe_get_block(fault_block_layer_type *layer, int block_id) {
    int storage_index = layer->get_block(block_id);
    if (storage_index < 0)
        return fault_block_layer_add_block(layer, block_id);
    else
        return layer->blocks.at(storage_index);
}

void fault_block_layer_del_block(fault_block_layer_type *layer, int block_id) {
    layer->remove_block(block_id);
}

bool fault_block_layer_has_block(const fault_block_layer_type *layer,
                                 int block_id) {
    if (layer->get_block(block_id) >= 0)
        return true;
    else
        return false;
}

int fault_block_layer_get_max_id(const fault_block_layer_type *layer) {
    return static_cast<int>(layer->block_map.size()) - 1;
}

int fault_block_layer_get_next_id(const fault_block_layer_type *layer) {
    if (layer->block_map.size() == 0)
        return 1;
    else
        return static_cast<int>(layer->block_map.size());
}

int fault_block_layer_get_size(const fault_block_layer_type *layer) {
    return static_cast<int>(layer->blocks.size());
}

int fault_block_layer_get_k(const fault_block_layer_type *layer) {
    return layer->k;
}

void fault_block_layer_free(fault_block_layer_type *layer) { delete layer; }

void fault_block_layer_insert_block_content(fault_block_layer_type *layer,
                                            const FaultBlock &src_block) {
    int next_block_id = fault_block_layer_get_next_id(layer);
    auto target_block = fault_block_layer_add_block(layer, next_block_id);
    target_block->copy_content(src_block);
}

bool fault_block_layer_export(const fault_block_layer_type *layer,
                              rd_kw_type *faultblock_kw) {
    if (rd_type_is_int(rd_kw_get_data_type(faultblock_kw)) &&
        (rd_kw_get_size(faultblock_kw) ==
         rd_grid_get_global_size(layer->grid))) {
        for (int j = 0; j < rd_grid_get_ny(layer->grid); j++) {
            for (int i = 0; i < rd_grid_get_nx(layer->grid); i++) {
                int g = rd_grid_get_global_index3(layer->grid, i, j, layer->k);
                int cell_value =
                    layer_iget_cell_value(layer->layer.get(), i, j);
                rd_kw_iset_int(faultblock_kw, g, cell_value);
            }
        }
        return true;
    } else
        return false;
}

rd_grid_type *fault_block_layer_get_grid(const fault_block_layer_type *layer) {
    return layer->grid;
}

layer_type *fault_block_layer_get_layer(const fault_block_layer_type *layer) {
    return layer->layer.get();
}
