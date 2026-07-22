#pragma once
#include <memory>
#include <ert/util/type_macros.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/layer.hpp>

UTIL_IS_INSTANCE_HEADER(fault_block_layer);

typedef struct fault_block_layer_struct fault_block_layer_type;

fault_block_layer_type *fault_block_layer_alloc(rd_grid_type *grid, int k);
void fault_block_layer_free(fault_block_layer_type *layer);
bool fault_block_layer_has_block(const fault_block_layer_type *layer,
                                 int block_id);
void fault_block_layer_del_block(fault_block_layer_type *layer, int block_id);

/** FaultBlocks are returned as shared_ptr which may outlive
    the fault_block_layer. However, if the fault_block_layer is freed,
    or the internal FaultBlock is deleted through del_block, it
    will become "detached" and it will not be possible to use it for
    eg. getting neighbours.
*/
std::shared_ptr<FaultBlock>
fault_block_layer_add_block(fault_block_layer_type *layer, int block_id);
std::shared_ptr<FaultBlock>
fault_block_layer_get_block(const fault_block_layer_type *layer, int block_id);
std::shared_ptr<FaultBlock>
fault_block_layer_iget_block(const fault_block_layer_type *layer,
                             int storage_index);
std::shared_ptr<FaultBlock>
fault_block_layer_safe_get_block(fault_block_layer_type *layer, int block_id);
int fault_block_layer_get_max_id(const fault_block_layer_type *layer);
int fault_block_layer_get_next_id(const fault_block_layer_type *layer);
int fault_block_layer_get_size(const fault_block_layer_type *layer);
bool fault_block_layer_scan_kw(fault_block_layer_type *layer,
                               const rd_kw_type *fault_block_kw);
bool fault_block_layer_load_kw(fault_block_layer_type *layer,
                               const rd_kw_type *fault_block_kw);
int fault_block_layer_get_k(const fault_block_layer_type *layer);
void fault_block_layer_scan_layer(fault_block_layer_type *fault_layer,
                                  layer_type *layer);
void fault_block_layer_insert_block_content(fault_block_layer_type *layer,
                                            const FaultBlock &src_block);
bool fault_block_layer_export(const fault_block_layer_type *layer,
                              rd_kw_type *faultblock_kw);
rd_grid_type *fault_block_layer_get_grid(const fault_block_layer_type *layer);
layer_type *fault_block_layer_get_layer(const fault_block_layer_type *layer);

using fault_block_layer_ptr =
    std::unique_ptr<fault_block_layer_type, decltype(&fault_block_layer_free)>;
