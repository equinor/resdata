#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <cstddef>
#include <ostream>
#include <resdata/layer.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include "detail/resdata/layer_cxx.hpp"
#include "resdata/rd_type.hpp"

#include <vector>
#include <tuple>
#include <memory>
#include <set>

using namespace Catch;
using namespace Matchers;

inline bool operator==(const int_point2d_type &lhs,
                       const int_point2d_type &rhs) {
    return lhs.i == rhs.i && lhs.j == rhs.j;
}

inline std::ostream &operator<<(std::ostream &os,
                                const int_point2d_type &point) {
    os << "{" << point.i << ", " << point.j << "}";
    return os;
}

static rd_grid_ptr generate_coordkw_grid(
    int num_x, int num_y, int num_z,
    const std::vector<std::tuple<int, int, int, int, double>> &z_vector) {
    auto coord_kw =
        make_rd_kw(COORD_KW, RD_GRID_COORD_SIZE(num_x, num_y), RD_FLOAT);
    auto zcorn_kw =
        make_rd_kw(ZCORN_KW, RD_GRID_ZCORN_SIZE(num_x, num_y, num_z), RD_FLOAT);

    for (int j = 0; j < num_y; j++) {
        for (int i = 0; i < num_x; i++) {
            size_t offset = static_cast<size_t>(6 * (i + j * num_x));
            rd_kw_iset_float(coord_kw.get(), offset, i);
            rd_kw_iset_float(coord_kw.get(), offset + 1, j);
            rd_kw_iset_float(coord_kw.get(), offset + 2, -1);

            rd_kw_iset_float(coord_kw.get(), offset + 3, i);
            rd_kw_iset_float(coord_kw.get(), offset + 4, j);
            rd_kw_iset_float(coord_kw.get(), offset + 5, -1);

            for (int k = 0; k < num_z; k++) {
                for (int c = 0; c < 4; c++) {
                    size_t zi1 =
                        rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
                    size_t zi2 =
                        rd_grid_zcorn_index__(num_x, num_y, i, j, k, c + 4);

                    double z1 = k;
                    double z2 = k + 1;

                    rd_kw_iset_float(zcorn_kw.get(), zi1, z1);
                    rd_kw_iset_float(zcorn_kw.get(), zi2, z2);
                }
            }
        }
    }

    for (const auto &[i, j, k, c, z] : z_vector) {
        auto index = rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
        rd_kw_iset_float(zcorn_kw.get(), index, z);
    }

    return {rd_grid_alloc_GRDECL_kw(num_x, num_y, num_z, zcorn_kw.get(),
                                    coord_kw.get(), NULL, NULL),
            &rd_grid_free};
}

TEST_CASE("Layer getters and setters", "[layer]") {
    GIVEN("A layer") {
        size_t nx = 10;
        size_t ny = 8;
        auto layer = make_layer(nx, ny);

        WHEN("Getting dimensions") {
            size_t result_nx = layer_get_nx(layer.get());
            size_t result_ny = layer_get_ny(layer.get());

            THEN("The dimensions match") {
                REQUIRE(result_nx == nx);
                REQUIRE(result_ny == ny);
            }
        }

        WHEN("Setting cell values") {
            layer_iset_cell_value(layer.get(), 2, 3, 42);
            layer_iset_cell_value(layer.get(), 5, 5, 100);

            THEN("The values can be retrieved") {
                REQUIRE(layer_iget_cell_value(layer.get(), 2, 3) == 42);
                REQUIRE(layer_iget_cell_value(layer.get(), 5, 5) == 100);
            }
        }

        WHEN("Setting cell values") {
            layer_iset_cell_value(layer.get(), 0, 0, 10);
            layer_iset_cell_value(layer.get(), 1, 1, 20);
            layer_iset_cell_value(layer.get(), 0, 0, 5);

            THEN("The cell sum is the sum of the set values") {
                REQUIRE(layer_get_cell_sum(layer.get()) == 20 + 5);
            }
        }

        THEN("All cells are active by default") {
            for (size_t i = 0; i < 5; i++) {
                for (size_t j = 0; j < 5; j++) {
                    REQUIRE(layer_iget_active(layer.get(), i, j));
                }
            }
        }

        WHEN("Assigning a value to all cells") {
            layer_assign(layer.get(), 7);

            THEN("All cells have the assigned value") {
                for (size_t i = 0; i < 5; i++) {
                    for (size_t j = 0; j < 5; j++) {
                        REQUIRE(layer_iget_cell_value(layer.get(), i, j) == 7);
                    }
                }
                REQUIRE(layer_get_cell_sum(layer.get()) ==
                        static_cast<int>(7 * nx * ny));
            }

            AND_WHEN("Clearing all cells") {
                layer_clear_cells(layer.get());

                THEN("All cells are zero") {
                    for (size_t i = 0; i < 5; i++) {
                        for (size_t j = 0; j < 5; j++) {
                            REQUIRE(layer_iget_cell_value(layer.get(), i, j) ==
                                    0);
                        }
                    }
                    REQUIRE(layer_get_cell_sum(layer.get()) == 0);
                }
            }
        }

        AND_GIVEN("A cell is set to some value") {
            layer_iset_cell_value(layer.get(), 2, 2, 10);

            THEN("RIGHT and BOTTOM have that value") {
                int right_edge =
                    layer_iget_edge_value(layer.get(), 2, 2, RIGHT_EDGE);
                int bottom_edge =
                    layer_iget_edge_value(layer.get(), 2, 2, BOTTOM_EDGE);

                REQUIRE(right_edge == 10);
                REQUIRE(bottom_edge == 10);
            }

            THEN("LEFT and TOP have the value negated") {
                int left_edge =
                    layer_iget_edge_value(layer.get(), 2, 2, LEFT_EDGE);
                int top_edge =
                    layer_iget_edge_value(layer.get(), 2, 2, TOP_EDGE);
                REQUIRE(left_edge == -10);
                REQUIRE(top_edge == -10);
            }

            WHEN("Checking if cell is on edge") {
                THEN("Cell with different neighbors is on edge") {
                    REQUIRE(layer_cell_on_edge(layer.get(), 2, 2));
                }
            }

            WHEN("All cells are set to the same value") {
                layer_assign(layer.get(), 10);
                THEN("Cell with same neighbors is not on edge") {
                    REQUIRE_FALSE(layer_cell_on_edge(layer.get(), 2, 2));
                }
                AND_WHEN("Updating connected cells from 10 to 5") {
                    layer_update_connected_cells(layer.get(), 2, 2, 10, 5);

                    THEN("All connected cells are updated") {
                        for (size_t i = 0; i < 5; i++) {
                            for (size_t j = 0; j < 5; j++) {
                                REQUIRE(layer_iget_cell_value(layer.get(), i,
                                                              j) == 5);
                            }
                        }
                    }
                }
            }
        }

        THEN("Adjacent cells have contact by default") {
            REQUIRE(layer_cell_contact(layer.get(), 1, 1, 2, 1));
            REQUIRE(layer_cell_contact(layer.get(), 1, 1, 1, 2));
        }

        THEN("Non-adjacent cells do not have contact by default") {
            REQUIRE_FALSE(layer_cell_contact(layer.get(), 1, 1, 3, 3));
        }

        AND_GIVEN("A vertical barrier") {
            layer_add_ijbarrier(layer.get(), 2, 0, 2, 3);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 1, 1, 2, 1));
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 1, 2, 2, 2));
            }

            WHEN("Checking for left barriers") {
                THEN("Barrier is present at expected locations") {
                    REQUIRE(layer_iget_left_barrier(layer.get(), 2, 1));
                    REQUIRE(layer_iget_left_barrier(layer.get(), 2, 2));
                    REQUIRE_FALSE(layer_iget_left_barrier(layer.get(), 3, 1));
                }
            }

            AND_WHEN("A horizontal barrier is added") {
                layer_add_ijbarrier(layer.get(), 0, 2, 3, 2);

                THEN("Barrier is present at expected locations") {
                    REQUIRE(layer_iget_bottom_barrier(layer.get(), 1, 2));
                    REQUIRE(layer_iget_bottom_barrier(layer.get(), 2, 2));
                    REQUIRE_FALSE(layer_iget_bottom_barrier(layer.get(), 1, 3));
                }
            }
        }

        AND_GIVEN("A horizontal barrier") {
            layer_add_ijbarrier(layer.get(), 0, 2, 3, 2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 1, 1, 1, 2));
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 2, 1, 2, 2));
            }
        }

        AND_GIVEN("A barrier using global cell indices") {
            size_t dimx = nx + 1;
            size_t c1 = 1 + 1 * dimx;
            size_t c2 = 1 + 3 * dimx;
            layer_add_barrier(layer.get(), c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 0, 1, 1, 1));
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 0, 2, 1, 2));
            }
        }

        AND_GIVEN("An interpolated diagonal barrier") {
            size_t dimx = nx + 1;
            size_t c1 = 2 + 2 * dimx;
            size_t c2 = 5 + 5 * dimx;
            layer_add_interp_barrier(layer.get(), c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 2, 2, 3, 2));
            }
        }
        AND_GIVEN("A layer of the same size") {
            auto dst = make_layer(nx, ny);

            WHEN("Source layer has some values set") {
                layer_iset_cell_value(layer.get(), 1, 1, 10);
                layer_iset_cell_value(layer.get(), 2, 2, 20);
                layer_iset_cell_value(layer.get(), 3, 3, 30);

                WHEN("Copying from source to destination") {
                    layer_memcpy(dst.get(), layer.get());

                    THEN("Destination has the same values") {
                        REQUIRE(layer_iget_cell_value(dst.get(), 1, 1) == 10);
                        REQUIRE(layer_iget_cell_value(dst.get(), 2, 2) == 20);
                        REQUIRE(layer_iget_cell_value(dst.get(), 3, 3) == 30);
                        REQUIRE(layer_get_cell_sum(dst.get()) ==
                                layer_get_cell_sum(layer.get()));
                    }
                }
            }
        }
        AND_GIVEN("Three cells are set to values 7") {
            layer_iset_cell_value(layer.get(), 1, 1, 7);
            layer_iset_cell_value(layer.get(), 2, 2, 7);
            layer_iset_cell_value(layer.get(), 3, 3, 7);

            AND_WHEN("Finding cells equal to value 7") {
                auto cells = layer_cells_equal(layer.get(), 7);

                THEN("Three cells are found") { REQUIRE(cells.size() == 3); }
            }

            THEN("The count of cells equal to 7 is three") {
                REQUIRE(layer_count_equal(layer.get(), 7) == 3);
            }
            THEN("The count of cells equal to 0 is nx*ny-3") {
                REQUIRE(layer_count_equal(layer.get(), 0) == nx * ny - 3);
            }
        }

        AND_GIVEN("The layer has a 3x3 block of cells set to value 42") {
            for (size_t i = 2; i < 5; i++) {
                for (size_t j = 2; j < 5; j++) {
                    layer_iset_cell_value(layer.get(), i, j, 42);
                }
            }

            AND_WHEN("Tracing block content") {
                AND_WHEN("Tracing block content without erasing") {
                    auto indices =
                        layer_trace_block_content(layer.get(), false, 3, 3, 42);

                    THEN("All 9 cells are found") {
                        REQUIRE(indices.size() == 9);
                        REQUIRE(indices.size() == 9);
                    }
                }

                AND_WHEN("Tracing and erasing block content") {
                    auto indices =
                        layer_trace_block_content(layer.get(), true, 3, 3, 42);

                    THEN("Cells are found and erased") {
                        REQUIRE(indices.size() == 9);

                        for (size_t i = 2; i < 5; i++) {
                            for (size_t j = 2; j < 5; j++) {
                                REQUIRE(layer_iget_cell_value(layer.get(), i,
                                                              j) == 0);
                            }
                        }
                    }
                }

                THEN("Tracing with value 0 from nonz-zero cell value traces") {
                    auto indices =
                        layer_trace_block_content(layer.get(), false, 3, 3, 0);
                    REQUIRE(indices.size() == 9);
                }

                THEN("Tracing from a cell with non-matching value fails") {
                    REQUIRE(
                        layer_trace_block_content(layer.get(), false, 6, 6, 0)
                            .empty());
                    REQUIRE(
                        layer_trace_block_content(layer.get(), false, 5, 5, 99)
                            .empty());
                }
            }

            AND_WHEN("Tracing edges") {
                std::vector<int_point2d_type> corner_list;
                std::vector<int> cell_list;
                THEN("The 3x3 block is traced when value is 42") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 2, 2, 42,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{{2, 2},
                                                          {3, 2},
                                                          {4, 2},
                                                          {5, 2},
                                                          {5, 3},
                                                          {5, 4},
                                                          {5, 5},
                                                          {4, 5},
                                                          {3, 5},
                                                          {2, 5},
                                                          {2, 4},
                                                          {2, 3}});
                    //    Cells form the floowing pattern
                    //    5    # # # #
                    //    4    #     #
                    // j  3    #     #
                    //    2    # # # #
                    //    1
                    //       1 2 3 4 5
                    //           i

                    AND_THEN("The outside cells are traced") {
                        REQUIRE(std::set(cell_list.begin(), cell_list.end())
                                    .size() == 8);
                    }
                }

                THEN("Tracing non-existent value returns false") {
                    REQUIRE_FALSE(layer_trace_block_edge(
                        layer.get(), 2, 2, 99, corner_list, cell_list));
                }

                THEN("Tracing from cell with value 0 returns false") {
                    REQUIRE_FALSE(layer_trace_block_edge(
                        layer.get(), 0, 0, 42, corner_list, cell_list));
                }
                THEN("Tracing from any cell traces the shape") {
                    for (size_t i = 2; i < 5; i++) {
                        for (size_t j = 2; j < 5; j++) {
                            if (i != 3 || j != 3) {
                                REQUIRE(layer_trace_block_edge(
                                    layer.get(), i, j, 42, corner_list,
                                    cell_list));
                                REQUIRE(
                                    std::set(cell_list.begin(), cell_list.end())
                                        .size() == 8);
                            }
                        }
                    }
                }
            }
        }

        WHEN("Tracing edges") {
            std::vector<int_point2d_type> corner_list;
            std::vector<int> cell_list;

            GIVEN("A single non-zero cell block") {
                layer_iset_cell_value(layer.get(), 5, 5, 7);
                THEN("single cell block is traced") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 5, 5, 7,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list == std::vector<int_point2d_type>{
                                               {5, 5}, {6, 5}, {6, 6}, {5, 6}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        1);
                }
            }
            AND_GIVEN("A 3x3 block starting at (0,0)") {
                for (size_t i = 0; i < 3; i++) {
                    for (size_t j = 0; j < 3; j++) {
                        layer_iset_cell_value(layer.get(), i, j, 10);
                    }
                }

                THEN("Starting from (0,0) traces the block") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 0, 0, 10,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{{0, 0},
                                                          {1, 0},
                                                          {2, 0},
                                                          {3, 0},
                                                          {3, 1},
                                                          {3, 2},
                                                          {3, 3},
                                                          {2, 3},
                                                          {1, 3},
                                                          {0, 3},
                                                          {0, 2},
                                                          {0, 1}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        8);
                }
            }

            AND_GIVEN("A 3x3 block at the far corner") {
                for (size_t i = nx - 3; i < nx; i++) {
                    for (size_t j = ny - 3; j < ny; j++) {
                        layer_iset_cell_value(layer.get(), i, j, 20);
                    }
                }

                THEN("Starting from (nx-1,ny-1) traces the block") {
                    REQUIRE(layer_trace_block_edge(layer.get(), nx - 1, ny - 1,
                                                   20, corner_list, cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{{10, 7},
                                                          {10, 8},
                                                          {9, 8},
                                                          {8, 8},
                                                          {7, 8},
                                                          {7, 7},
                                                          {7, 6},
                                                          {7, 5},
                                                          {8, 5},
                                                          {9, 5},
                                                          {10, 5},
                                                          {10, 6}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        8);
                }
            }
            AND_GIVEN("An L-shaped block") {
                layer_iset_cell_value(layer.get(), 3, 3, 15);
                layer_iset_cell_value(layer.get(), 4, 3, 15);
                layer_iset_cell_value(layer.get(), 5, 3, 15);
                layer_iset_cell_value(layer.get(), 3, 4, 15);
                layer_iset_cell_value(layer.get(), 3, 5, 15);

                THEN("the L-shaped edge is traced") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 3, 3, 15,
                                                   corner_list, cell_list));
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        5);
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{{3, 3},
                                                          {4, 3},
                                                          {5, 3},
                                                          {6, 3},
                                                          {6, 4},
                                                          {5, 4},
                                                          {4, 4},
                                                          {4, 5},
                                                          {4, 6},
                                                          {3, 6},
                                                          {3, 5},
                                                          {3, 4}});
                }
            }
            AND_GIVEN("A block with diagonal step pattern") {
                for (size_t i = 5; i < 8; i++) {
                    for (size_t j = 5; j < 8; j++) {
                        layer_iset_cell_value(layer.get(), i, j, 50);
                    }
                }

                THEN("Tracing from a starting point not on edge traces the "
                     "pattern") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 6, 6, 50,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{{8, 6},
                                                          {8, 7},
                                                          {8, 8},
                                                          {7, 8},
                                                          {6, 8},
                                                          {5, 8},
                                                          {5, 7},
                                                          {5, 6},
                                                          {5, 5},
                                                          {6, 5},
                                                          {7, 5},
                                                          {8, 5}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        8);
                }
            }
            AND_GIVEN("Block with concave shape") {
                for (size_t i = 0; i <= 7; i++) {
                    layer_iset_cell_value(layer.get(), i, 0, 60);
                    layer_iset_cell_value(layer.get(), i, 7, 60);
                }
                for (size_t j = 0; j <= 5; j++) {
                    layer_iset_cell_value(layer.get(), 0, j, 60);
                }

                THEN("The shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 0, 0, 60,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{
                                {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0},
                                {6, 0}, {7, 0}, {8, 0}, {8, 1}, {7, 1}, {6, 1},
                                {5, 1}, {4, 1}, {3, 1}, {2, 1}, {1, 1}, {1, 2},
                                {1, 3}, {1, 4}, {1, 5}, {1, 6}, {0, 6}, {0, 5},
                                {0, 4}, {0, 3}, {0, 2}, {0, 1}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        13);
                }
            }
            AND_GIVEN("A block with stair-step pattern") {
                layer_iset_cell_value(layer.get(), 0, 0, 70);
                layer_iset_cell_value(layer.get(), 1, 0, 70);
                layer_iset_cell_value(layer.get(), 1, 1, 70);
                layer_iset_cell_value(layer.get(), 2, 1, 70);
                layer_iset_cell_value(layer.get(), 2, 2, 70);

                THEN("The stair-step shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 0, 0, 70,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{{0, 0},
                                                          {1, 0},
                                                          {2, 0},
                                                          {2, 1},
                                                          {3, 1},
                                                          {3, 2},
                                                          {3, 3},
                                                          {2, 3},
                                                          {2, 2},
                                                          {1, 2},
                                                          {1, 1},
                                                          {0, 1}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        5);
                }
            }
            AND_GIVEN("A block with zigzag pattern") {
                layer_iset_cell_value(layer.get(), 0, 0, 80);
                layer_iset_cell_value(layer.get(), 1, 0, 80);
                layer_iset_cell_value(layer.get(), 1, 1, 80);
                layer_iset_cell_value(layer.get(), 0, 1, 80);
                layer_iset_cell_value(layer.get(), 0, 2, 80);
                layer_iset_cell_value(layer.get(), 1, 2, 80);

                THEN("The zigzag shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 0, 0, 80,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{{0, 0},
                                                          {1, 0},
                                                          {2, 0},
                                                          {2, 1},
                                                          {2, 2},
                                                          {2, 3},
                                                          {1, 3},
                                                          {0, 3},
                                                          {0, 2},
                                                          {0, 1}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        6);
                }
            }
            GIVEN("A block with T-shaped pattern") {
                for (size_t i = 0; i <= 4; i++) {
                    layer_iset_cell_value(layer.get(), i, 2, 90);
                }
                for (size_t j = 3; j <= 6; j++) {
                    layer_iset_cell_value(layer.get(), 2, j, 90);
                }

                THEN("The shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer.get(), 2, 2, 90,
                                                   corner_list, cell_list));
                    REQUIRE(corner_list == std::vector<int_point2d_type>{
                                               {2, 2}, {3, 2}, {4, 2}, {5, 2},
                                               {5, 3}, {4, 3}, {3, 3}, {3, 4},
                                               {3, 5}, {3, 6}, {3, 7}, {2, 7},
                                               {2, 6}, {2, 5}, {2, 4}, {2, 3},
                                               {1, 3}, {0, 3}, {0, 2}, {1, 2}});
                    REQUIRE(
                        std::set(cell_list.begin(), cell_list.end()).size() ==
                        9);
                }
            }
        }

        AND_GIVEN("A nx x ny x 3 grid") {
            auto grid = generate_coordkw_grid(nx, ny, 3, {});
            WHEN("Updating layer active status from grid") {
                layer_update_active(layer.get(), grid.get(), 0);

                THEN("Layer active status matches grid") {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t j = 0; j < ny; j++) {
                            bool grid_active =
                                rd_grid_cell_active3(grid.get(), i, j, 0);
                            bool layer_active =
                                layer_iget_active(layer.get(), i, j);
                            REQUIRE(layer_active == grid_active);
                        }
                    }
                }
            }
        }

        size_t dimx = nx + 1;
        WHEN("Adding a vertical interpolated barrier") {
            size_t c1 = 3 + 2 * dimx;
            size_t c2 = 3 + 7 * dimx;
            layer_add_interp_barrier(layer.get(), c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 2, 3, 3, 3));
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 2, 5, 3, 5));
            }
        }

        AND_WHEN("Adding a horizontal interpolated barrier") {
            size_t c1 = 2 + 4 * dimx;
            size_t c2 = 7 + 4 * dimx;
            layer_add_interp_barrier(layer.get(), c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 3, 3, 3, 4));
                REQUIRE_FALSE(layer_cell_contact(layer.get(), 5, 3, 5, 4));
            }
        }
    }
}
