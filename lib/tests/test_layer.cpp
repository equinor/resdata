#include <catch2/catch.hpp>
#include <resdata/layer.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <ert/util/int_vector.hpp>
#include <vector>

#include "tmpdir.hpp"
#include "detail/resdata/layer_cxx.hpp"

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

static rd_grid_type *generate_coordkw_grid(
    int num_x, int num_y, int num_z,
    const std::vector<std::tuple<int, int, int, int, double>> &z_vector) {
    rd_kw_type *coord_kw =
        rd_kw_alloc(COORD_KW, RD_GRID_COORD_SIZE(num_x, num_y), RD_FLOAT);
    rd_kw_type *zcorn_kw = rd_kw_alloc(
        ZCORN_KW, RD_GRID_ZCORN_SIZE(num_x, num_y, num_z), RD_FLOAT);

    for (int j = 0; j < num_y; j++) {
        for (int i = 0; i < num_x; i++) {
            int offset = 6 * (i + j * num_x);
            rd_kw_iset_float(coord_kw, offset, i);
            rd_kw_iset_float(coord_kw, offset + 1, j);
            rd_kw_iset_float(coord_kw, offset + 2, -1);

            rd_kw_iset_float(coord_kw, offset + 3, i);
            rd_kw_iset_float(coord_kw, offset + 4, j);
            rd_kw_iset_float(coord_kw, offset + 5, -1);

            for (int k = 0; k < num_z; k++) {
                for (int c = 0; c < 4; c++) {
                    int zi1 = rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
                    int zi2 =
                        rd_grid_zcorn_index__(num_x, num_y, i, j, k, c + 4);

                    double z1 = k;
                    double z2 = k + 1;

                    rd_kw_iset_float(zcorn_kw, zi1, z1);
                    rd_kw_iset_float(zcorn_kw, zi2, z2);
                }
            }
        }
    }

    for (const auto &[i, j, k, c, z] : z_vector) {
        auto index = rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
        rd_kw_iset_float(zcorn_kw, index, z);
    }

    rd_grid_type *grid = rd_grid_alloc_GRDECL_kw(num_x, num_y, num_z, zcorn_kw,
                                                 coord_kw, NULL, NULL);
    rd_kw_free(coord_kw);
    rd_kw_free(zcorn_kw);

    return grid;
}

TEST_CASE("Layer getters and setters", "[layer]") {
    GIVEN("A layer") {
        int nx = 10;
        int ny = 8;
        layer_type *layer = layer_alloc(nx, ny);

        WHEN("Getting dimensions") {
            int result_nx = layer_get_nx(layer);
            int result_ny = layer_get_ny(layer);

            THEN("The dimensions match") {
                REQUIRE(result_nx == nx);
                REQUIRE(result_ny == ny);
            }
        }

        WHEN("Setting cell values") {
            layer_iset_cell_value(layer, 2, 3, 42);
            layer_iset_cell_value(layer, 5, 5, 100);

            THEN("The values can be retrieved") {
                REQUIRE(layer_iget_cell_value(layer, 2, 3) == 42);
                REQUIRE(layer_iget_cell_value(layer, 5, 5) == 100);
            }
        }

        WHEN("Setting cell values") {
            layer_iset_cell_value(layer, 0, 0, 10);
            layer_iset_cell_value(layer, 1, 1, 20);
            layer_iset_cell_value(layer, 0, 0, 5);

            THEN("The cell sum is the sum of the set values") {
                REQUIRE(layer_get_cell_sum(layer) == 20 + 5);
            }
        }

        THEN("All cells are active by default") {
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    REQUIRE(layer_iget_active(layer, i, j) == true);
                }
            }
        }

        WHEN("Assigning a value to all cells") {
            layer_assign(layer, 7);

            THEN("All cells have the assigned value") {
                for (int i = 0; i < 5; i++) {
                    for (int j = 0; j < 5; j++) {
                        REQUIRE(layer_iget_cell_value(layer, i, j) == 7);
                    }
                }
                REQUIRE(layer_get_cell_sum(layer) == 7 * 10 * 8);
            }

            AND_WHEN("Clearing all cells") {
                layer_clear_cells(layer);

                THEN("All cells are zero") {
                    for (int i = 0; i < 5; i++) {
                        for (int j = 0; j < 5; j++) {
                            REQUIRE(layer_iget_cell_value(layer, i, j) == 0);
                        }
                    }
                    REQUIRE(layer_get_cell_sum(layer) == 0);
                }
            }
        }

        AND_GIVEN("A cell is set to some value") {
            layer_iset_cell_value(layer, 2, 2, 10);

            THEN("RIGHT and BOTTOM have that value for, negated for the "
                 "others") {
                int left_edge = layer_iget_edge_value(layer, 2, 2, LEFT_EDGE);
                int right_edge = layer_iget_edge_value(layer, 2, 2, RIGHT_EDGE);
                int top_edge = layer_iget_edge_value(layer, 2, 2, TOP_EDGE);
                int bottom_edge =
                    layer_iget_edge_value(layer, 2, 2, BOTTOM_EDGE);

                REQUIRE(left_edge == -10);
                REQUIRE(right_edge == 10);
                REQUIRE(top_edge == -10);
                REQUIRE(bottom_edge == 10);
            }

            WHEN("Checking if cell is on edge") {
                THEN("Cell with different neighbors is on edge") {
                    REQUIRE(layer_cell_on_edge(layer, 2, 2));
                }
            }

            WHEN("All cells are set to the same value") {
                layer_assign(layer, 10);
                THEN("Cell with same neighbors is not on edge") {
                    REQUIRE_FALSE(layer_cell_on_edge(layer, 2, 2));
                }
                AND_WHEN("Updating connected cells from 10 to 5") {
                    layer_update_connected_cells(layer, 2, 2, 10, 5);

                    THEN("All connected cells are updated") {
                        for (int i = 0; i < 5; i++) {
                            for (int j = 0; j < 5; j++) {
                                REQUIRE(layer_iget_cell_value(layer, i, j) ==
                                        5);
                            }
                        }
                    }
                }
            }
        }

        THEN("Adjacent cells have contact by default") {
            REQUIRE(layer_cell_contact(layer, 1, 1, 2, 1));
            REQUIRE(layer_cell_contact(layer, 1, 1, 1, 2));
        }

        THEN("Non-adjacent cells do not have contact by default") {
            REQUIRE_FALSE(layer_cell_contact(layer, 1, 1, 3, 3));
        }

        AND_GIVEN("A vertical barrier") {
            layer_add_ijbarrier(layer, 2, 0, 2, 3);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer, 1, 1, 2, 1));
                REQUIRE_FALSE(layer_cell_contact(layer, 1, 2, 2, 2));
            }

            WHEN("Checking for left barriers") {
                THEN("Barrier is present at expected locations") {
                    REQUIRE(layer_iget_left_barrier(layer, 2, 1) == true);
                    REQUIRE(layer_iget_left_barrier(layer, 2, 2) == true);
                    REQUIRE(layer_iget_left_barrier(layer, 3, 1) == false);
                }
            }

            AND_WHEN("A horizontal barrier is added") {
                layer_add_ijbarrier(layer, 0, 2, 3, 2);

                THEN("Barrier is present at expected locations") {
                    REQUIRE(layer_iget_bottom_barrier(layer, 1, 2));
                    REQUIRE(layer_iget_bottom_barrier(layer, 2, 2));
                    REQUIRE_FALSE(layer_iget_bottom_barrier(layer, 1, 3));
                }
            }
        }

        AND_GIVEN("A horizontal barrier") {
            layer_add_ijbarrier(layer, 0, 2, 3, 2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer, 1, 1, 1, 2));
                REQUIRE_FALSE(layer_cell_contact(layer, 2, 1, 2, 2));
            }
        }

        AND_GIVEN("A barrier using global cell indices") {
            int dimx = nx + 1;
            int c1 = 1 + 1 * dimx;
            int c2 = 1 + 3 * dimx;
            layer_add_barrier(layer, c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer, 0, 1, 1, 1));
                REQUIRE_FALSE(layer_cell_contact(layer, 0, 2, 1, 2));
            }
        }

        AND_GIVEN("An interpolated diagonal barrier") {
            int dimx = nx + 1;
            int c1 = 2 + 2 * dimx;
            int c2 = 5 + 5 * dimx;
            layer_add_interp_barrier(layer, c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE_FALSE(layer_cell_contact(layer, 2, 2, 3, 2));
            }
        }
        AND_GIVEN("A layer of the same size") {
            layer_type *dst = layer_alloc(nx, ny);

            WHEN("Source layer has some values set") {
                layer_iset_cell_value(layer, 1, 1, 10);
                layer_iset_cell_value(layer, 2, 2, 20);
                layer_iset_cell_value(layer, 3, 3, 30);

                WHEN("Copying from source to destination") {
                    layer_memcpy(dst, layer);

                    THEN("Destination has the same values") {
                        REQUIRE(layer_iget_cell_value(dst, 1, 1) == 10);
                        REQUIRE(layer_iget_cell_value(dst, 2, 2) == 20);
                        REQUIRE(layer_iget_cell_value(dst, 3, 3) == 30);
                        REQUIRE(layer_get_cell_sum(dst) ==
                                layer_get_cell_sum(layer));
                    }
                }
            }

            layer_free(dst);
        }
        AND_GIVEN("Three cells are set to values 7") {
            layer_iset_cell_value(layer, 1, 1, 7);
            layer_iset_cell_value(layer, 2, 2, 7);
            layer_iset_cell_value(layer, 3, 3, 7);

            AND_WHEN("Finding cells equal to value 7") {
                int_vector_type *i_list = int_vector_alloc(0, 0);
                int_vector_type *j_list = int_vector_alloc(0, 0);
                layer_cells_equal(layer, 7, i_list, j_list);

                THEN("Three cells are found") {
                    REQUIRE(int_vector_size(i_list) == 3);
                    REQUIRE(int_vector_size(j_list) == 3);
                }

                int_vector_free(i_list);
                int_vector_free(j_list);
            }

            THEN("The count of cells equal to 7 is three") {
                REQUIRE(layer_count_equal(layer, 7) == 3);
            }
            THEN("The count of cells equal to 0 is nx*ny-3") {
                REQUIRE(layer_count_equal(layer, 0) == nx * ny - 3);
            }
        }

        AND_GIVEN("The layer has a 3x3 block of cells set to value 42") {
            for (int i = 2; i < 5; i++) {
                for (int j = 2; j < 5; j++) {
                    layer_iset_cell_value(layer, i, j, 42);
                }
            }

            AND_WHEN("Tracing block content") {
                int_vector_type *i_list = int_vector_alloc(0, 0);
                int_vector_type *j_list = int_vector_alloc(0, 0);

                AND_WHEN("Tracing block content without erasing") {
                    bool traced = layer_trace_block_content(layer, false, 3, 3,
                                                            42, i_list, j_list);

                    THEN("All 9 cells are found") {
                        REQUIRE(traced == true);
                        REQUIRE(int_vector_size(i_list) == 9);
                        REQUIRE(int_vector_size(j_list) == 9);
                    }
                }

                AND_WHEN("Tracing and erasing block content") {
                    bool traced = layer_trace_block_content(layer, true, 3, 3,
                                                            42, i_list, j_list);

                    THEN("Cells are found and erased") {
                        REQUIRE(traced == true);
                        REQUIRE(int_vector_size(i_list) == 9);

                        for (int i = 2; i < 5; i++) {
                            for (int j = 2; j < 5; j++) {
                                REQUIRE(layer_iget_cell_value(layer, i, j) ==
                                        0);
                            }
                        }
                    }
                }

                THEN("Tracing from a cell with non-matching value fails") {
                    REQUIRE_FALSE(layer_trace_block_content(layer, false, 6, 6,
                                                            0, i_list, j_list));
                    REQUIRE_FALSE(layer_trace_block_content(
                        layer, false, 5, 5, 99, i_list, j_list));
                }
                int_vector_free(i_list);
                int_vector_free(j_list);
            }

            AND_WHEN("Tracing edges") {
                std::vector<int_point2d_type> corner_list;
                int_vector_type *cell_list = int_vector_alloc(0, 0);
                THEN("The 3x3 block is traced when value is 42") {
                    REQUIRE(layer_trace_block_edge(layer, 2, 2, 42, corner_list,
                                                   cell_list));
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

                    AND_THEN("The outside cells are traced") {
                        REQUIRE(int_vector_size(cell_list) == 8);
                    }
                }

                THEN("Tracing non-existent value returns false") {
                    REQUIRE_FALSE(layer_trace_block_edge(
                        layer, 2, 2, 99, corner_list, cell_list));
                }

                THEN("Tracing from cell with value 0 returns false") {
                    REQUIRE_FALSE(layer_trace_block_edge(
                        layer, 0, 0, 42, corner_list, cell_list));
                }
                THEN("Tracing from any cell traces the shape") {
                    for (int i = 2; i < 5; i++) {
                        for (int j = 2; j < 5; j++) {
                            if (i != 3 || j != 3) {
                                REQUIRE(layer_trace_block_edge(
                                    layer, i, j, 42, corner_list, cell_list));
                                REQUIRE(int_vector_size(cell_list) == 8);
                            }
                        }
                    }
                }
                int_vector_free(cell_list);
            }
        }

        WHEN("Tracing edges") {
            std::vector<int_point2d_type> corner_list;
            int_vector_type *cell_list = int_vector_alloc(0, 0);

            GIVEN("A single non-zero cell block") {
                layer_iset_cell_value(layer, 5, 5, 7);
                THEN("single cell block is traced") {
                    REQUIRE(layer_trace_block_edge(layer, 5, 5, 7, corner_list,
                                                   cell_list));
                    REQUIRE(corner_list == std::vector<int_point2d_type>{
                                               {5, 5}, {6, 5}, {6, 6}, {5, 6}});
                    REQUIRE(int_vector_size(cell_list) == 1);
                }
            }
            AND_GIVEN("A 3x3 block starting at (0,0)") {
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        layer_iset_cell_value(layer, i, j, 10);
                    }
                }

                THEN("Starting from (0,0) traces the block") {
                    REQUIRE(layer_trace_block_edge(layer, 0, 0, 10, corner_list,
                                                   cell_list));
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
                    REQUIRE(int_vector_size(cell_list) == 8);
                }
            }

            AND_GIVEN("A 3x3 block at the far corner") {
                for (int i = nx - 3; i < nx; i++) {
                    for (int j = ny - 3; j < ny; j++) {
                        layer_iset_cell_value(layer, i, j, 20);
                    }
                }

                THEN("Starting from (nx-1,ny-1) traces the block") {
                    REQUIRE(layer_trace_block_edge(layer, nx - 1, ny - 1, 20,
                                                   corner_list, cell_list));
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
                    REQUIRE(int_vector_size(cell_list) == 8);
                }
            }
            AND_GIVEN("An L-shaped block") {
                layer_iset_cell_value(layer, 3, 3, 15);
                layer_iset_cell_value(layer, 4, 3, 15);
                layer_iset_cell_value(layer, 5, 3, 15);
                layer_iset_cell_value(layer, 3, 4, 15);
                layer_iset_cell_value(layer, 3, 5, 15);

                THEN("the L-shaped edge is traced") {
                    REQUIRE(layer_trace_block_edge(layer, 3, 3, 15, corner_list,
                                                   cell_list));
                    REQUIRE(int_vector_size(cell_list) == 5);
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
                for (int i = 5; i < 8; i++) {
                    for (int j = 5; j < 8; j++) {
                        layer_iset_cell_value(layer, i, j, 50);
                    }
                }

                THEN("Tracing from a starting point not on edge traces the "
                     "pattern") {
                    REQUIRE(layer_trace_block_edge(layer, 6, 6, 50, corner_list,
                                                   cell_list));
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
                    REQUIRE(int_vector_size(cell_list) == 8);
                }
            }
            AND_GIVEN("Block with concave shape") {
                for (int i = 0; i <= 7; i++) {
                    layer_iset_cell_value(layer, i, 0, 60);
                    layer_iset_cell_value(layer, i, 7, 60);
                }
                for (int j = 0; j <= 5; j++) {
                    layer_iset_cell_value(layer, 0, j, 60);
                }

                THEN("The shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer, 0, 0, 60, corner_list,
                                                   cell_list));
                    REQUIRE(corner_list ==
                            std::vector<int_point2d_type>{
                                {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0},
                                {6, 0}, {7, 0}, {8, 0}, {8, 1}, {7, 1}, {6, 1},
                                {5, 1}, {4, 1}, {3, 1}, {2, 1}, {1, 1}, {1, 2},
                                {1, 3}, {1, 4}, {1, 5}, {1, 6}, {0, 6}, {0, 5},
                                {0, 4}, {0, 3}, {0, 2}, {0, 1}});
                    REQUIRE(int_vector_size(cell_list) == 13);
                }
            }
            AND_GIVEN("A block with stair-step pattern") {
                layer_iset_cell_value(layer, 0, 0, 70);
                layer_iset_cell_value(layer, 1, 0, 70);
                layer_iset_cell_value(layer, 1, 1, 70);
                layer_iset_cell_value(layer, 2, 1, 70);
                layer_iset_cell_value(layer, 2, 2, 70);

                THEN("The stair-step shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer, 0, 0, 70, corner_list,
                                                   cell_list));
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
                    REQUIRE(int_vector_size(cell_list) == 5);
                }
            }
            AND_GIVEN("A block with zigzag pattern") {
                layer_iset_cell_value(layer, 0, 0, 80);
                layer_iset_cell_value(layer, 1, 0, 80);
                layer_iset_cell_value(layer, 1, 1, 80);
                layer_iset_cell_value(layer, 0, 1, 80);
                layer_iset_cell_value(layer, 0, 2, 80);
                layer_iset_cell_value(layer, 1, 2, 80);

                THEN("The zigzag shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer, 0, 0, 80, corner_list,
                                                   cell_list));
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
                    REQUIRE(int_vector_size(cell_list) == 6);
                }
            }
            GIVEN("A block with T-shaped pattern") {
                for (int i = 0; i <= 4; i++) {
                    layer_iset_cell_value(layer, i, 2, 90);
                }
                for (int j = 3; j <= 6; j++) {
                    layer_iset_cell_value(layer, 2, j, 90);
                }

                THEN("The shape is traced") {
                    REQUIRE(layer_trace_block_edge(layer, 2, 2, 90, corner_list,
                                                   cell_list));
                    REQUIRE(corner_list == std::vector<int_point2d_type>{
                                               {2, 2}, {3, 2}, {4, 2}, {5, 2},
                                               {5, 3}, {4, 3}, {3, 3}, {3, 4},
                                               {3, 5}, {3, 6}, {3, 7}, {2, 7},
                                               {2, 6}, {2, 5}, {2, 4}, {2, 3},
                                               {1, 3}, {0, 3}, {0, 2}, {1, 2}});
                    REQUIRE(int_vector_size(cell_list) == 9);
                }
            }

            int_vector_free(cell_list);
        }

        AND_GIVEN("A nx x ny x 3 grid") {
            rd_grid_type *grid = generate_coordkw_grid(nx, ny, 3, {});
            WHEN("Updating layer active status from grid") {
                layer_update_active(layer, grid, 0);

                THEN("Layer active status matches grid") {
                    for (int i = 0; i < nx; i++) {
                        for (int j = 0; j < ny; j++) {
                            bool grid_active =
                                rd_grid_cell_active3(grid, i, j, 0);
                            bool layer_active = layer_iget_active(layer, i, j);
                            REQUIRE(layer_active == grid_active);
                        }
                    }
                }
            }
            rd_grid_free(grid);
        }

        int dimx = nx + 1;
        WHEN("Adding a vertical interpolated barrier") {
            int c1 = 3 + 2 * dimx;
            int c2 = 3 + 7 * dimx;
            layer_add_interp_barrier(layer, c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE(layer_cell_contact(layer, 2, 3, 3, 3) == false);
                REQUIRE(layer_cell_contact(layer, 2, 5, 3, 5) == false);
            }
        }

        AND_WHEN("Adding a horizontal interpolated barrier") {
            int c1 = 2 + 4 * dimx;
            int c2 = 7 + 4 * dimx;
            layer_add_interp_barrier(layer, c1, c2);

            THEN("Cells are separated by the barrier") {
                REQUIRE(layer_cell_contact(layer, 3, 3, 3, 4) == false);
                REQUIRE(layer_cell_contact(layer, 5, 3, 5, 4) == false);
            }
        }

        layer_free(layer);
    }
}
