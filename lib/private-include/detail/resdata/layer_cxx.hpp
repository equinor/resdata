#pragma once
#include <vector>
#include <resdata/layer.hpp>

bool layer_trace_block_edge(const layer_type *layer, int i, int j, int value,
                            std::vector<int_point2d_type> &corner_list,
                            std::vector<int> &cell_list);
