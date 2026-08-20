#include <stdexcept>
#include <vector>
#include <set>

#include <ert/util/test_util.hpp>

#include <resdata/layer.hpp>
#include "detail/resdata/layer_cxx.hpp"

void test_get_cell() {
    layer_type *layer = layer_alloc(10, 10);
    test_assert_int_equal(0, layer_iget_cell_value(layer, 0, 0));
    test_assert_int_equal(0, layer_get_cell_sum(layer));
    layer_iset_cell_value(layer, 0, 0, 77);
    test_assert_int_equal(77, layer_get_cell_sum(layer));
    test_assert_int_equal(77, layer_iget_cell_value(layer, 0, 0));
    layer_iset_cell_value(layer, 1, 1, 23);
    test_assert_int_equal(100, layer_get_cell_sum(layer));
    layer_iset_cell_value(layer, 0, 0, 0);
    test_assert_int_equal(23, layer_get_cell_sum(layer));
    layer_free(layer);
}

void test_get_invalid_edge() {
    layer_ptr layer = make_layer(10, 10);
    test_assert_throw(layer_iget_edge_value(layer.get(), 10, 10, RIGHT_EDGE),
                      std::out_of_range);
    test_assert_throw(layer_iget_edge_value(layer.get(), 10, 0, RIGHT_EDGE),
                      std::out_of_range);
    test_assert_throw(layer_iget_edge_value(layer.get(), 10, 0, BOTTOM_EDGE),
                      std::out_of_range);
    test_assert_throw(layer_iget_edge_value(layer.get(), 0, 10, TOP_EDGE),
                      std::out_of_range);
    test_assert_throw(layer_iget_edge_value(layer.get(), 0, 10, RIGHT_EDGE),
                      std::out_of_range);
    test_assert_throw(layer_iget_edge_value(layer.get(), 0, 10, LEFT_EDGE),
                      std::out_of_range);
    test_assert_throw(layer_iget_edge_value(layer.get(), 10, 10, TOP_EDGE),
                      std::out_of_range);
}

void test_edge() {
    layer_type *layer = layer_alloc(10, 10);

    layer_iset_cell_value(layer, 2, 2, 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, LEFT_EDGE), -100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, TOP_EDGE), -100);
    test_assert_true(layer_cell_on_edge(layer, 2, 2));

    layer_iset_cell_value(layer, 3, 2, 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, LEFT_EDGE), -100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, TOP_EDGE), -100);

    test_assert_int_equal(layer_iget_edge_value(layer, 3, 2, LEFT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 3, 2, BOTTOM_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 3, 2, RIGHT_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 3, 2, TOP_EDGE), -100);

    layer_iset_cell_value(layer, 1, 2, 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, LEFT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, TOP_EDGE), -100);

    test_assert_int_equal(layer_iget_edge_value(layer, 1, 2, LEFT_EDGE), -100);
    test_assert_int_equal(layer_iget_edge_value(layer, 1, 2, BOTTOM_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 1, 2, RIGHT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 1, 2, TOP_EDGE), -100);

    layer_iset_cell_value(layer, 2, 3, 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, LEFT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, TOP_EDGE), 0);

    test_assert_int_equal(layer_iget_edge_value(layer, 2, 3, LEFT_EDGE), -100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 3, BOTTOM_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 3, RIGHT_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 3, TOP_EDGE), -100);

    layer_iset_cell_value(layer, 2, 1, 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, LEFT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, TOP_EDGE), 0);

    test_assert_int_equal(layer_iget_edge_value(layer, 2, 1, LEFT_EDGE), -100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 1, BOTTOM_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 1, RIGHT_EDGE), 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 1, TOP_EDGE), 0);

    layer_iset_cell_value(layer, 2, 2, 100);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, LEFT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE), 0);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, TOP_EDGE), 0);
    test_assert_false(layer_cell_on_edge(layer, 2, 2));

    layer_iset_cell_value(layer, 2, 2, 200);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, LEFT_EDGE), -200);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE), 200);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE), 200);
    test_assert_int_equal(layer_iget_edge_value(layer, 2, 2, TOP_EDGE), -200);

    layer_free(layer);
}

void test_walk() {
    layer_type *layer = layer_alloc(10, 10);
    std::vector<int_point2d_type> corner_list;
    std::vector<int> cell_list;

    test_assert_false(
        layer_trace_block_edge(layer, 4, 4, 100, corner_list, cell_list));
    layer_iset_cell_value(layer, 4, 4, 100);
    test_assert_false(
        layer_trace_block_edge(layer, 4, 4, 200, corner_list, cell_list));

    test_assert_true(
        layer_trace_block_edge(layer, 4, 4, 100, corner_list, cell_list));
    test_assert_int_equal(corner_list.size(), 4);
    test_assert_size_t_equal(
        std::set(cell_list.begin(), cell_list.end()).size(), 1);
    {
        test_assert_int_equal(4, corner_list[0].i);
        test_assert_int_equal(4, corner_list[0].j);

        test_assert_int_equal(5, corner_list[1].i);
        test_assert_int_equal(4, corner_list[1].j);

        test_assert_int_equal(5, corner_list[2].i);
        test_assert_int_equal(5, corner_list[2].j);

        test_assert_int_equal(4, corner_list[3].i);
        test_assert_int_equal(5, corner_list[3].j);
    }

    {
        int i, j;
        std::set<int> true_cell_set;
        for (j = 3; j < 7; j++) {
            for (i = 3; i < 7; i++) {
                layer_iset_cell_value(layer, i, j, 100);

                if (i == 3 || j == 3)
                    true_cell_set.insert(i + j * layer_get_nx(layer));

                if (i == 6 || j == 6)
                    true_cell_set.insert(i + j * layer_get_nx(layer));
            }
        }

        test_assert_true(
            layer_trace_block_edge(layer, 3, 3, 100, corner_list, cell_list));
        test_assert_int_equal(16, corner_list.size());
        std::set<int> cell_set(cell_list.begin(), cell_list.end());
        test_assert_size_t_equal(12, cell_set.size());

        test_assert_true(cell_set == true_cell_set);
    }

    layer_free(layer);
}

void test_content1() {
    layer_type *layer = layer_alloc(10, 10);
    int i, j;
    for (j = 4; j < 8; j++)
        for (i = 4; i < 8; i++)
            layer_iset_cell_value(layer, i, j, 1);

    test_assert_int_equal(16, layer_get_cell_sum(layer));
    test_assert_true(layer_trace_block_content(layer, false, 4, 4, 10).empty());
    test_assert_size_t_equal(
        16, layer_trace_block_content(layer, false, 4, 4, 1).size());

    test_assert_size_t_equal(
        16, layer_trace_block_content(layer, false, 4, 4, 0).size());

    test_assert_size_t_equal(
        16, layer_trace_block_content(layer, true, 4, 4, 0).size());
    test_assert_int_equal(0, layer_get_cell_sum(layer));

    layer_free(layer);
}

void test_content2() {
    layer_type *layer = layer_alloc(5, 5);
    int i, j;
    for (j = 0; j < 5; j++)
        layer_iset_cell_value(layer, 2, j, 1);

    for (i = 0; i < 5; i++)
        layer_iset_cell_value(layer, i, 2, 1);

    test_assert_int_equal(9, layer_get_cell_sum(layer));
    {
        std::vector<int> cell_list;
        std::vector<int_point2d_type> corner_list;

        for (j = 0; j < 5; j++) {
            for (i = 0; i < 5; i++) {
                int cell_value = layer_iget_cell_value(layer, i, j);

                if (cell_value != 0) {
                    test_assert_true(layer_trace_block_edge(
                        layer, i, j, cell_value, corner_list, cell_list));
                    test_assert_true(
                        layer_trace_block_content(layer, true, i, j, cell_value)
                            .size() > 0);
                }
            }
        }
    }
    test_assert_int_equal(0, layer_get_cell_sum(layer));

    layer_free(layer);
}

void test_replace() {
    layer_type *layer = layer_alloc(10, 10);
    int i, j;
    for (j = 0; j < 5; j++)
        for (i = 0; i < 5; i++)
            layer_iset_cell_value(layer, i, j, 1);

    test_assert_int_equal(25, layer_get_cell_sum(layer));
    test_assert_int_equal(layer_replace_cell_values(layer, 1, 2), 25);
    test_assert_int_equal(50, layer_get_cell_sum(layer));
    test_assert_int_equal(layer_replace_cell_values(layer, 1, 2), 0);

    layer_free(layer);
}

void test_interp_barrier() {
    layer_type *layer = layer_alloc(10, 10);

    layer_add_interp_barrier(layer, 0, 22);

    layer_free(layer);
}

void test_copy() {
    layer_type *layer1 = layer_alloc(10, 10);
    layer_type *layer2 = layer_alloc(10, 10);

    layer_iset_cell_value(layer1, 5, 5, 10);
    layer_memcpy(layer2, layer1);

    test_assert_int_equal(10, layer_iget_edge_value(layer2, 5, 5, BOTTOM_EDGE));
    test_assert_int_equal(10, layer_iget_edge_value(layer2, 5, 5, RIGHT_EDGE));
    test_assert_int_equal(-10, layer_iget_edge_value(layer2, 5, 5, TOP_EDGE));
    test_assert_int_equal(-10, layer_iget_edge_value(layer2, 5, 5, LEFT_EDGE));

    layer_free(layer2);
    layer_free(layer1);
}

int main(int argc, char **argv) {
    test_get_cell();
    test_get_invalid_edge();
    test_edge();
    test_walk();
    test_content1();
    test_content2();
    test_replace();
    test_interp_barrier();
    test_copy();
}
