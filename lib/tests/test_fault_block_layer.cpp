#include <resdata/fault_block_layer.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/layer.hpp>

#include <memory>

#include <catch2/catch.hpp>

static std::unique_ptr<rd_grid_type, decltype(&rd_grid_free)>
make_grid(int nx, int ny, int nz) {
    return {rd_grid_alloc_rectangular(nx, ny, nz, 1, 1, 1, nullptr),
            rd_grid_free};
}

static std::unique_ptr<fault_block_layer_type,
                       decltype(&fault_block_layer_free)>
make_fb_layer(const rd_grid_type *grid, int k) {
    return {fault_block_layer_alloc(grid, k), fault_block_layer_free};
}

static std::unique_ptr<rd_kw_type, decltype(&rd_kw_free)>
make_kw(const char *header, int size, rd_data_type data_type) {
    return {rd_kw_alloc(header, size, data_type), rd_kw_free};
}

TEST_CASE("fault_block_layer alloc errors", "[fault_block_layer]") {
    GIVEN("A 5x5x3 grid") {
        auto grid = make_grid(5, 5, 3);
        WHEN("Allocating with k out of range") {
            THEN("Returns NULL for negative k") {
                REQUIRE(fault_block_layer_alloc(grid.get(), -1) == nullptr);
            }

            THEN("Returns NULL for k >= nz") {
                REQUIRE(fault_block_layer_alloc(grid.get(), 3) == nullptr);
            }
        }
    }
}

TEST_CASE("fault_block_layer methods", "[fault_block_layer]") {
    GIVEN("A fault_block_layer") {
        int nx = 5, ny = 5, nz = 3;
        auto grid = make_grid(nx, ny, nz);
        auto idx = [&](int i, int j) {
            return rd_grid_get_global_index3(grid.get(), i, j, 0);
        };
        auto layer = make_fb_layer(grid.get(), 0);
        layer_type *geo_layer = fault_block_layer_get_layer(layer.get());

        THEN("The k value matches") {
            REQUIRE(fault_block_layer_get_k(layer.get()) == 0);
        }
        THEN("The grid is the same") {
            REQUIRE(fault_block_layer_get_grid(layer.get()) == grid.get());
        }
        THEN("The layer is initially empty") {
            REQUIRE(fault_block_layer_get_size(layer.get()) == 0);
        }
        THEN("get_next_id returns 1 when empty") {
            REQUIRE(fault_block_layer_get_next_id(layer.get()) == 1);
        }
        THEN("get_max_id returns -1 when empty") {
            REQUIRE(fault_block_layer_get_max_id(layer.get()) == -1);
        }
        THEN("get_layer returns a non-null layer") {
            REQUIRE(fault_block_layer_get_layer(layer.get()) != nullptr);
        }

        WHEN("A block with id 4 is added") {
            fault_block_layer_add_block(layer.get(), 4);

            THEN("get_max_id returns 4") {
                REQUIRE(fault_block_layer_get_max_id(layer.get()) == 4);
            }

            THEN("get_next_id returns 5") {
                REQUIRE(fault_block_layer_get_next_id(layer.get()) == 5);
            }
        }

        WHEN("Adding a block") {
            fault_block_type *block =
                fault_block_layer_add_block(layer.get(), 3);

            THEN("The layer has one block") {
                REQUIRE(fault_block_layer_get_size(layer.get()) == 1);
            }

            THEN("has_block returns true for that id") {
                REQUIRE(fault_block_layer_has_block(layer.get(), 3));
            }

            THEN("has_block returns false for a different id") {
                REQUIRE_FALSE(fault_block_layer_has_block(layer.get(), 99));
            }

            THEN("get_block returns the same block") {
                REQUIRE(fault_block_layer_get_block(layer.get(), 3) == block);
            }

            THEN("iget_block returns the block at storage index 0") {
                REQUIRE(fault_block_layer_iget_block(layer.get(), 0) == block);
            }

            THEN("get_block_id matches") {
                REQUIRE(fault_block_get_id(block) == 3);
            }
        }

        WHEN("Adding the same block id twice") {
            fault_block_layer_add_block(layer.get(), 5);
            fault_block_type *dup = fault_block_layer_add_block(layer.get(), 5);

            THEN("The second add returns NULL") { REQUIRE(dup == nullptr); }

            THEN("The layer still has one block") {
                REQUIRE(fault_block_layer_get_size(layer.get()) == 1);
            }
        }

        WHEN("safe_get_block is called for a non-existing id") {
            fault_block_type *block =
                fault_block_layer_safe_get_block(layer.get(), 7);

            THEN("The block is created and returned") {
                REQUIRE(block != nullptr);
                REQUIRE(fault_block_layer_has_block(layer.get(), 7));
            }
        }

        WHEN("safe_get_block is called for an existing id") {
            fault_block_layer_add_block(layer.get(), 2);
            fault_block_type *block =
                fault_block_layer_safe_get_block(layer.get(), 2);

            THEN("The existing block is returned") {
                REQUIRE(block != nullptr);
                REQUIRE(fault_block_get_id(block) == 2);
                REQUIRE(fault_block_layer_get_size(layer.get()) == 1);
            }
        }
        AND_GIVEN("Two blocks in the layer") {
            fault_block_layer_add_block(layer.get(), 1);
            fault_block_layer_add_block(layer.get(), 2);

            WHEN("Deleting one of the blocks") {
                fault_block_layer_del_block(layer.get(), 1);

                THEN("The layer has one block") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 1);
                }

                THEN("has_block returns false for the deleted id") {
                    REQUIRE_FALSE(fault_block_layer_has_block(layer.get(), 1));
                }

                THEN("has_block still returns true for the remaining block") {
                    REQUIRE(fault_block_layer_has_block(layer.get(), 2));
                }
            }

            WHEN("Deleting a non-existing block") {
                fault_block_layer_del_block(layer.get(), 99);

                THEN("The layer still has two blocks") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 2);
                }
            }
        }

        AND_GIVEN("A fault block keyword") {
            /*
             * Layout of the layer (j outer, i inner):
             *   1 1 0 0 0
             *   0 2 2 0 0
             *   0 0 0 0 0
             *   0 0 0 0 0
             *   0 0 0 0 0
             */
            auto kw = make_kw("FAULTBLK", nx * ny * nz, RD_INT);
            rd_kw_iset_int(kw.get(), idx(0, 0), 1);
            rd_kw_iset_int(kw.get(), idx(1, 0), 1);
            rd_kw_iset_int(kw.get(), idx(1, 1), 2);
            rd_kw_iset_int(kw.get(),
                           rd_grid_get_global_index3(grid.get(), 2, 1, 0), 2);

            WHEN("scan_kw is called") {
                bool ok = fault_block_layer_scan_kw(layer.get(), kw.get());

                THEN("scan_kw returns true") { REQUIRE(ok); }

                THEN("Three blocks are created (two non-zero ids plus one "
                     "for zero cells)") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 3);
                }
                THEN("The first block contains the 1 cells") {
                    const fault_block_type *block =
                        fault_block_layer_iget_block(layer.get(), 0);
                    REQUIRE(fault_block_get_size(block) == 2);
                    REQUIRE(layer_iget_cell_value(geo_layer, 0, 0) == 1);
                    REQUIRE(layer_iget_cell_value(geo_layer, 1, 0) == 1);
                }
                THEN("The second block contains the 0 cells") {
                    const fault_block_type *block =
                        fault_block_layer_iget_block(layer.get(), 1);
                    REQUIRE(fault_block_get_size(block) == 21);
                    REQUIRE(layer_iget_cell_value(geo_layer, 3, 3) == 2);
                }
                THEN("The third block contains the 2 cells") {
                    const fault_block_type *block =
                        fault_block_layer_iget_block(layer.get(), 2);
                    REQUIRE(fault_block_get_size(block) == 2);
                    REQUIRE(layer_iget_cell_value(geo_layer, 1, 1) == 3);
                    REQUIRE(layer_iget_cell_value(geo_layer, 2, 1) == 3);
                }
            }

            WHEN("load_kw is called") {
                bool ok = fault_block_layer_load_kw(layer.get(), kw.get());

                THEN("load_kw returns true") { REQUIRE(ok); }

                THEN("Two blocks are created (only non-zero ids)") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 2);
                }
                THEN("The first block contains the 1 cells") {
                    const fault_block_type *block =
                        fault_block_layer_iget_block(layer.get(), 0);
                    REQUIRE(fault_block_get_size(block) == 2);
                    REQUIRE(layer_iget_cell_value(geo_layer, 0, 0) == 1);
                    REQUIRE(layer_iget_cell_value(geo_layer, 1, 0) == 1);
                }
                THEN("The second block contains the 2 cells") {
                    const fault_block_type *block =
                        fault_block_layer_iget_block(layer.get(), 1);
                    REQUIRE(fault_block_get_size(block) == 2);
                    REQUIRE(layer_iget_cell_value(geo_layer, 1, 1) == 2);
                    REQUIRE(layer_iget_cell_value(geo_layer, 2, 1) == 2);
                }
                THEN("Zero cells are not assigned a block") {
                    REQUIRE(layer_iget_cell_value(geo_layer, 3, 3) == 0);
                }
            }
            WHEN("Exporting to a correctly sized integer keyword") {
                fault_block_layer_load_kw(layer.get(), kw.get());
                auto out_kw = make_kw("OUT", nx * ny * nz, RD_INT);
                bool ok = fault_block_layer_export(layer.get(), out_kw.get());

                THEN("export returns true") { REQUIRE(ok); }

                THEN("exported cell values match the original keyword") {
                    REQUIRE(rd_kw_iget_int(out_kw.get(), idx(0, 0)) == 1);
                    REQUIRE(rd_kw_iget_int(out_kw.get(), idx(1, 0)) == 1);
                    REQUIRE(rd_kw_iget_int(out_kw.get(), idx(1, 1)) == 2);
                    REQUIRE(rd_kw_iget_int(out_kw.get(), idx(2, 1)) == 2);
                    REQUIRE(rd_kw_iget_int(out_kw.get(), idx(2, 0)) == 0);
                }
            }

            WHEN("Exporting to a keyword with wrong size") {
                auto bad_kw = make_kw("OUT", 1, RD_INT);
                THEN("export returns false") {
                    REQUIRE_FALSE(
                        fault_block_layer_export(layer.get(), bad_kw.get()));
                }
            }

            WHEN("Exporting to a float keyword") {
                auto float_kw = make_kw("OUT", nx * ny * nz, RD_FLOAT);
                THEN("export returns false") {
                    REQUIRE_FALSE(
                        fault_block_layer_export(layer.get(), float_kw.get()));
                }
            }
        }
        WHEN("scan_kw is called with non-integer keyword") {
            auto float_kw = make_kw("FAULTBLK", nx * ny * nz, RD_FLOAT);
            THEN("scan_kw returns false") {
                REQUIRE_FALSE(
                    fault_block_layer_scan_kw(layer.get(), float_kw.get()));
            }
        }
        WHEN("scan_kw is called with wrong size keyword") {
            auto bad_kw = make_kw("FAULTBLK", 1, RD_INT);
            THEN("scan_kw returns false") {
                REQUIRE_FALSE(
                    fault_block_layer_scan_kw(layer.get(), bad_kw.get()));
            }
        }

        WHEN("load_kw is called with wrong size keyword") {
            auto bad_kw = make_kw("FAULTBLK", 1, RD_INT);
            THEN("load_kw returns false") {
                REQUIRE_FALSE(
                    fault_block_layer_load_kw(layer.get(), bad_kw.get()));
            }
        }
        WHEN("load_kw is called with non-integer keyword") {
            auto float_kw = make_kw("FAULTBLK", nx * ny * nz, RD_FLOAT);
            THEN("load_kw returns false") {
                REQUIRE_FALSE(
                    fault_block_layer_load_kw(layer.get(), float_kw.get()));
            }
        }
        AND_GIVEN("A layer with block containing two cells") {
            fault_block_type *src_block =
                fault_block_layer_add_block(layer.get(), 1);
            fault_block_add_cell(src_block, 0, 0);
            fault_block_add_cell(src_block, 1, 0);

            WHEN("insert_block_content is called on another layer") {
                auto dst_layer = make_fb_layer(grid.get(), 0);
                fault_block_layer_insert_block_content(dst_layer.get(),
                                                       src_block);

                THEN("The destination layer has one block") {
                    REQUIRE(fault_block_layer_get_size(dst_layer.get()) == 1);
                }

                THEN("The inserted block has two cells") {
                    fault_block_type *inserted =
                        fault_block_layer_iget_block(dst_layer.get(), 0);
                    REQUIRE(fault_block_get_size(inserted) == 2);
                }
            }
        }
        AND_GIVEN("blocks at ids 1 and 10") {
            fault_block_layer_add_block(layer.get(), 1);
            fault_block_layer_add_block(layer.get(), 10);

            THEN("get_size is 2") {
                REQUIRE(fault_block_layer_get_size(layer.get()) == 2);
            }

            THEN("has_block is false for an intermediate id") {
                REQUIRE_FALSE(fault_block_layer_has_block(layer.get(), 5));
            }

            THEN("get_block returns NULL for an intermediate id") {
                REQUIRE(fault_block_layer_get_block(layer.get(), 5) == nullptr);
            }

            THEN("get_max_id is 10") {
                REQUIRE(fault_block_layer_get_max_id(layer.get()) == 10);
            }
        }
        AND_GIVEN("three blocks at ids 1, 2, 3") {
            fault_block_layer_add_block(layer.get(), 1);
            fault_block_layer_add_block(layer.get(), 2);
            fault_block_layer_add_block(layer.get(), 3);

            WHEN("The first block (id=1) is deleted") {
                fault_block_layer_del_block(layer.get(), 1);

                THEN("get_size is 2") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 2);
                }

                THEN("iget_block(0) returns block id 2") {
                    fault_block_type *b =
                        fault_block_layer_iget_block(layer.get(), 0);
                    REQUIRE(fault_block_get_id(b) == 2);
                }

                THEN("iget_block(1) returns block id 3") {
                    fault_block_type *b =
                        fault_block_layer_iget_block(layer.get(), 1);
                    REQUIRE(fault_block_get_id(b) == 3);
                }

                THEN("blocks 2 and 3 are still accessible by id") {
                    REQUIRE(fault_block_layer_get_block(layer.get(), 2) !=
                            nullptr);
                    REQUIRE(fault_block_layer_get_block(layer.get(), 3) !=
                            nullptr);
                }
            }

            WHEN("The middle block (id=2) is deleted") {
                fault_block_layer_del_block(layer.get(), 2);

                THEN("get_size is 2") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 2);
                }

                THEN("blocks 1 and 3 are still accessible by id") {
                    REQUIRE(fault_block_layer_get_block(layer.get(), 1) !=
                            nullptr);
                    REQUIRE(fault_block_layer_get_block(layer.get(), 3) !=
                            nullptr);
                }

                THEN("block 2 is no longer accessible") {
                    REQUIRE(fault_block_layer_get_block(layer.get(), 2) ==
                            nullptr);
                }
            }

            WHEN("The last block (id=3) is deleted") {
                fault_block_layer_del_block(layer.get(), 3);

                THEN("get_size is 2") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 2);
                }

                THEN("blocks 1 and 2 are still accessible by id") {
                    REQUIRE(fault_block_layer_get_block(layer.get(), 1) !=
                            nullptr);
                    REQUIRE(fault_block_layer_get_block(layer.get(), 2) !=
                            nullptr);
                }
            }
            WHEN("All blocks are deleted") {
                fault_block_layer_del_block(layer.get(), 1);
                fault_block_layer_del_block(layer.get(), 2);
                fault_block_layer_del_block(layer.get(), 3);

                THEN("get_size returns 0") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 0);
                }
            }
        }
        GIVEN("A block with id=5") {
            fault_block_layer_add_block(layer.get(), 5);

            WHEN("Block 5 is deleted and then re-added") {
                fault_block_layer_del_block(layer.get(), 5);
                fault_block_type *new_block =
                    fault_block_layer_add_block(layer.get(), 5);

                THEN("The new add succeeds") { REQUIRE(new_block != nullptr); }

                THEN("get_size is 1") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 1);
                }

                THEN("has_block returns true for id 5") {
                    REQUIRE(fault_block_layer_has_block(layer.get(), 5));
                }
            }
            WHEN("Block 5 is deleted") {
                fault_block_layer_del_block(layer.get(), 5);

                // block_map is not shrunk on deletion, so get_next_id remains
                // at max+1 rather than reverting to the pre-add value.
                THEN("get_next_id is still 6 (block_map is not shrunk)") {
                    REQUIRE(fault_block_layer_get_next_id(layer.get()) == 6);
                }

                THEN("get_size is 0") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 0);
                }
            }
        }
        AND_GIVEN("A kw with nonzero data in k=2") {
            auto kw = make_kw("FAULTBLK", nx * ny * nz, RD_INT);

            // Two cells with block id=1 in the last layer only
            rd_kw_iset_int(kw.get(),
                           rd_grid_get_global_index3(grid.get(), 0, 0, 2), 1);
            rd_kw_iset_int(kw.get(),
                           rd_grid_get_global_index3(grid.get(), 1, 0, 2), 1);

            WHEN("scan_kw is called on layer k=2 (last layer, where data "
                 "lives)") {
                auto layer_2 = make_fb_layer(grid.get(), 2);
                bool ok = fault_block_layer_scan_kw(layer_2.get(), kw.get());

                THEN("scan_kw returns true") { REQUIRE(ok); }

                // The two non-zero cells form one block; the remaining 7 zero
                // cells are grouped into a second block.
                THEN("Two blocks are created") {
                    REQUIRE(fault_block_layer_get_size(layer_2.get()) == 2);
                }
            }

            WHEN("scan_kw is called on layer k=0 (no data in this layer)") {
                fault_block_layer_scan_kw(layer.get(), kw.get());

                // All cells in k=0 are zero; assign_zero groups them into one
                // single connected block.
                THEN("Only one block is created (all cells are zero)") {
                    REQUIRE(fault_block_layer_get_size(layer.get()) == 1);
                }
            }
        }
    }
}
