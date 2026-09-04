#include "resdata/rd_type.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>
#include <resdata/rd_kw.hpp>
#include <string>

int num_selected(rd_region_type *region) {
    return rd_region_get_global_list(region).size();
}

TEST_CASE("rd_region", "[rd_region]") {
    rd_grid_ptr grid = make_rectangular_grid(10, 10, 10, 1, 1, 1, NULL);

    SECTION("Allocate with preselect true") {
        rd_region_type *region = rd_region_alloc(grid.get(), true);
        REQUIRE(num_selected(region) == 1000);
        rd_region_free(region);
    }

    SECTION("Allocate with preselect false") {
        rd_region_type *region = rd_region_alloc(grid.get(), false);
        REQUIRE(num_selected(region) == 0);
        rd_region_free(region);
    }

    SECTION("Copy region") {
        rd_region_type *region1 = rd_region_alloc(grid.get(), false);
        rd_region_select_i1i2(region1, 0, 4);

        rd_region_type *region2 = rd_region_alloc_copy(region1);
        REQUIRE(rd_region_equal(region1, region2));

        rd_region_free(region1);
        rd_region_free(region2);
    }

    SECTION("Basic Region operations") {
        rd_region_type *region = rd_region_alloc(grid.get(), false);

        SECTION("Set and get name") {
            rd_region_set_name(region, "TestRegion");
            auto name = rd_region_get_name(region);
            REQUIRE(name.has_value());
            REQUIRE(*name == "TestRegion");
        }

        SECTION("Reset region") {
            rd_region_select_all(region);
            rd_region_reset(region);
            REQUIRE(num_selected(region) == 0);
        }

        SECTION("Select and deselect all") {
            rd_region_select_all(region);
            REQUIRE(num_selected(region) == 1000);

            rd_region_deselect_all(region);
            REQUIRE(num_selected(region) == 0);
        }

        SECTION("Invert selection") {
            rd_region_select_i1i2(region, 0, 4);
            int initial_size = num_selected(region);

            rd_region_invert_selection(region);
            int inverted_size = num_selected(region);
            REQUIRE(initial_size + inverted_size == 1000);
        }

        SECTION("Get global list") {
            REQUIRE(rd_region_get_global_list(region).empty());
        }

        SECTION("Get active list") {
            REQUIRE(rd_region_get_active_list(region).empty());
        }

        SECTION("Get global active list") {
            REQUIRE(rd_region_get_global_active_list(region).empty());
        }

        SECTION("contains functions") {
            rd_region_select_i1i2(region, 2, 5);

            SECTION("Contains ijk") {
                REQUIRE(rd_region_contains_ijk(region, 2, 0, 0) == true);
                REQUIRE(rd_region_contains_ijk(region, 0, 0, 0) == false);
            }

            SECTION("Contains global") {
                int global_idx = rd_grid_get_global_index3(grid.get(), 2, 0, 0);
                REQUIRE(rd_region_contains_global(region, global_idx) == true);
                global_idx = rd_grid_get_global_index3(grid.get(), 0, 0, 0);
                REQUIRE(rd_region_contains_global(region, global_idx) == false);
            }

            SECTION("Contains active") {
                int active_idx = rd_grid_get_active_index3(grid.get(), 2, 0, 0);
                REQUIRE(rd_region_contains_active(region, active_idx) == true);
                active_idx = rd_grid_get_active_index3(grid.get(), 0, 0, 0);
                REQUIRE(rd_region_contains_active(region, active_idx) == false);
            }
        }

        SECTION("select with int kw") {
            auto int_kw = make_rd_kw("INTGR", 1000, RD_INT);

            for (int i = 0; i < 500; i++)
                rd_kw_iset_int(int_kw.get(), i, 1);
            for (int i = 500; i < 1000; i++)
                rd_kw_iset_int(int_kw.get(), i, 2);

            rd_region_select_equal(region, int_kw.get(), 1);
            REQUIRE(num_selected(region) == 500);
            rd_region_deselect_equal(region, int_kw.get(), 1);
            REQUIRE(num_selected(region) == 0);
        }

        SECTION("select with bool kw") {
            auto bool_kw = make_rd_kw("BOOL", 1000, RD_BOOL);

            for (int i = 0; i < 500; i++)
                rd_kw_iset_bool(bool_kw.get(), i, true);
            for (int i = 500; i < 1000; i++)
                rd_kw_iset_bool(bool_kw.get(), i, false);

            rd_region_select_true(region, bool_kw.get());
            REQUIRE(num_selected(region) == 500);
            rd_region_select_false(region, bool_kw.get());
            REQUIRE(num_selected(region) == 1000);
        }

        SECTION("select with float kw") {
            auto float_kw = make_rd_kw("FLOAT", 1000, RD_FLOAT);

            for (int i = 0; i < 1000; i++)
                rd_kw_iset_float(float_kw.get(), i, i * 0.1f);

            SECTION("Select in interval") {
                rd_region_select_in_interval(region, float_kw.get(), 10.0,
                                             50.0);
                REQUIRE(num_selected(region) == 400);
                rd_region_deselect_in_interval(region, float_kw.get(), 10.0,
                                               50.0);
                REQUIRE(num_selected(region) == 0);
            }

            SECTION("Select comparison") {
                rd_region_select_smaller(region, float_kw.get(), 50.0f);
                REQUIRE(num_selected(region) == 500);
                rd_region_deselect_smaller(region, float_kw.get(), 50.0f);
                REQUIRE(num_selected(region) == 0);
            }

            SECTION("Select larger") {
                rd_region_select_larger(region, float_kw.get(), 49.9f);
                REQUIRE(num_selected(region) == 501);
                rd_region_deselect_larger(region, float_kw.get(), 49.9f);
                REQUIRE(num_selected(region) == 0);
            }

            SECTION("comparison select") {
                auto cmp_kw = make_rd_kw("KW2", 1000, RD_FLOAT);

                for (int i = 0; i < 1000; i++)
                    rd_kw_iset_float(cmp_kw.get(), i, 50.0f);

                SECTION("Compare less") {
                    rd_region_cmp_select_less(region, float_kw.get(),
                                              cmp_kw.get());
                    REQUIRE(num_selected(region) == 500);
                    rd_region_cmp_deselect_less(region, float_kw.get(),
                                                cmp_kw.get());
                    REQUIRE(num_selected(region) == 0);
                }

                SECTION("Compare more") {
                    rd_region_cmp_select_more(region, float_kw.get(),
                                              cmp_kw.get());
                    REQUIRE(num_selected(region) == 500);
                    rd_region_cmp_deselect_more(region, float_kw.get(),
                                                cmp_kw.get());
                    REQUIRE(num_selected(region) == 0);
                }
            }
        }

        SECTION("select active") {
            rd_region_select_active_cells(region);
            REQUIRE(num_selected(region) == 1000);

            rd_region_deselect_active_cells(region);
            REQUIRE(num_selected(region) == 0);

            rd_region_select_inactive_cells(region);
            REQUIRE(num_selected(region) == 0);

            rd_region_select_all(region);
            rd_region_deselect_inactive_cells(region);
            REQUIRE(num_selected(region) == 1000);
        }

        SECTION("Select from ijkbox") {
            rd_region_select_from_ijkbox(region, 0, 4, 0, 4, 0, 4);
            REQUIRE(num_selected(region) == 125);
        }

        SECTION("Deselect from ijkbox") {
            rd_region_select_all(region);
            rd_region_deselect_from_ijkbox(region, 0, 4, 0, 4, 0, 4);
            REQUIRE(num_selected(region) == 875);
        }

        SECTION("Select i1i2") {
            rd_region_select_i1i2(region, 2, 5);
            REQUIRE(num_selected(region) == 400);
        }

        SECTION("Deselect i1i2") {
            rd_region_select_all(region);
            rd_region_deselect_i1i2(region, 2, 5);
            REQUIRE(num_selected(region) == 600);
        }

        SECTION("Select j1j2") {
            rd_region_select_j1j2(region, 3, 6);
            REQUIRE(num_selected(region) == 400);
        }

        SECTION("Deselect j1j2") {
            rd_region_select_all(region);
            rd_region_deselect_j1j2(region, 3, 6);
            REQUIRE(num_selected(region) == 600);
        }

        SECTION("Select k1k2") {
            rd_region_select_k1k2(region, 1, 4);
            REQUIRE(num_selected(region) == 400);
        }

        SECTION("Deselect k1k2") {
            rd_region_select_all(region);
            rd_region_deselect_k1k2(region, 1, 4);
            REQUIRE(num_selected(region) == 600);
        }

        SECTION("Select shallow cells") {
            rd_region_select_shallow_cells(region, 5.0);
            REQUIRE(num_selected(region) == 500);
        }

        SECTION("Deselect shallow cells") {
            rd_region_select_all(region);
            rd_region_deselect_shallow_cells(region, 5.0);
            REQUIRE(num_selected(region) == 500);
        }

        SECTION("Select deep cells") {
            rd_region_select_deep_cells(region, 5.0);
            REQUIRE(num_selected(region) == 500);
        }

        SECTION("Deselect deep cells") {
            rd_region_select_all(region);
            rd_region_deselect_deep_cells(region, 5.0);
            REQUIRE(num_selected(region) == 500);
        }

        SECTION("Select thin cells") {
            rd_region_select_thin_cells(region, 2.0);
            REQUIRE(num_selected(region) == 1000);
        }

        SECTION("Deselect thin cells") {
            rd_region_select_all(region);
            rd_region_deselect_thin_cells(region, 0.5);
            REQUIRE(num_selected(region) == 1000);
        }

        SECTION("Select thick cells") {
            rd_region_select_thick_cells(region, 0.5);
            REQUIRE(num_selected(region) == 1000);
        }

        SECTION("Deselect thick cells") {
            rd_region_select_all(region);
            rd_region_deselect_thick_cells(region, 2.0);
            REQUIRE(num_selected(region) == 1000);
        }

        SECTION("Select small cells") {
            rd_region_select_small_cells(region, 2.0);
            REQUIRE(num_selected(region) >= 0);
        }

        SECTION("Deselect small cells") {
            rd_region_select_all(region);
            rd_region_deselect_small_cells(region, 0.5);
            REQUIRE(num_selected(region) >= 0);
        }

        SECTION("Select large cells") {
            rd_region_select_large_cells(region, 0.5);
            REQUIRE(num_selected(region) > 0);
        }

        SECTION("Deselect large cells") {
            rd_region_select_all(region);
            rd_region_deselect_large_cells(region, 2.0);
            REQUIRE(num_selected(region) >= 0);
        }

        SECTION("Region binary operations") {
            rd_region_type *region2 = rd_region_alloc(grid.get(), false);

            rd_region_select_i1i2(region, 0, 5);
            rd_region_select_i1i2(region2, 3, 9);

            SECTION("Equality") {
                REQUIRE(rd_region_equal(region, region));
                REQUIRE(rd_region_equal(region2, region2));
                REQUIRE(!rd_region_equal(region, region2));
            }

            SECTION("intersection") {
                rd_region_intersection(region, region2);
                REQUIRE(num_selected(region) == 300);
            }

            SECTION("union") {
                rd_region_union(region, region2);
                REQUIRE(num_selected(region) == 1000);
            }

            SECTION("subtract") {
                rd_region_subtract(region, region2);
                REQUIRE(num_selected(region) == 300);
            }

            rd_region_free(region2);
        }

        SECTION("Region plane operations") {
            double n[3] = {0.0, 0.0, 1.0};
            double p[3] = {5.0, 5.0, 5.0};

            SECTION("Select above plane") {
                rd_region_select_above_plane(region, n, p);
                REQUIRE(num_selected(region) == 500);
            }

            SECTION("Select below plane") {
                rd_region_select_below_plane(region, n, p);
                REQUIRE(num_selected(region) == 500);
            }

            SECTION("Deselect above plane") {
                rd_region_select_all(region);
                rd_region_deselect_above_plane(region, n, p);
                REQUIRE(num_selected(region) == 500);
            }

            SECTION("Deselect below plane") {
                rd_region_select_all(region);
                rd_region_deselect_below_plane(region, n, p);
                REQUIRE(num_selected(region) == 500);
            }
        }

        SECTION("Region operate on keyword") {
            rd_region_select_i1i2(region, 0, 4);

            SECTION("Set keyword int") {
                auto kw = make_rd_kw("TEST", 1000, RD_INT);
                rd_region_set_kw_int(region, kw.get(), 42, false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE(rd_kw_iget_int(kw.get(), list.at(0)) == 42);
            }

            SECTION("Set keyword float") {
                auto kw = make_rd_kw("TEST", 1000, RD_FLOAT);
                rd_region_set_kw_float(region, kw.get(), 3.14f, false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE_THAT(rd_kw_iget_float(kw.get(), list.at(0)),
                             Catch::Matchers::WithinAbs(3.14f, 0.01f));
            }

            SECTION("Set keyword double") {
                auto kw = make_rd_kw("TEST", 1000, RD_DOUBLE);
                rd_region_set_kw_double(region, kw.get(), 2.71, false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE_THAT(rd_kw_iget_double(kw.get(), list.at(0)),
                             Catch::Matchers::WithinAbs(2.71, 0.01));
            }

            SECTION("Shift keyword int") {
                auto kw = make_rd_kw("TEST", 1000, RD_INT);
                for (int i = 0; i < 1000; i++)
                    rd_kw_iset_int(kw.get(), i, 10);
                rd_region_shift_kw_int(region, kw.get(), 5, false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE(rd_kw_iget_int(kw.get(), list.at(0)) == 15);
            }

            SECTION("Scale keyword float") {
                auto kw = make_rd_kw("TEST", 1000, RD_FLOAT);
                for (int i = 0; i < 1000; i++)
                    rd_kw_iset_float(kw.get(), i, 10.0f);
                rd_region_scale_kw_float(region, kw.get(), 2.0f, false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE_THAT(rd_kw_iget_float(kw.get(), list.at(0)),
                             Catch::Matchers::WithinAbs(20.0f, 0.01f));
            }

            SECTION("Keyword copy") {
                auto kw_src = make_rd_kw("SRC", 1000, RD_INT);
                auto kw_dst = make_rd_kw("DST", 1000, RD_INT);
                for (int i = 0; i < 1000; i++)
                    rd_kw_iset_int(kw_src.get(), i, 99);
                rd_region_kw_copy(region, kw_dst.get(), kw_src.get(), false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE(rd_kw_iget_int(kw_dst.get(), list.at(0)) == 99);
            }

            SECTION("Keyword iadd") {
                auto kw1 = make_rd_kw("KW1", 1000, RD_FLOAT);
                auto kw2 = make_rd_kw("KW2", 1000, RD_FLOAT);
                for (int i = 0; i < 1000; i++) {
                    rd_kw_iset_float(kw1.get(), i, 10.0f);
                    rd_kw_iset_float(kw2.get(), i, 5.0f);
                }
                rd_region_kw_iadd(region, kw1.get(), kw2.get(), false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE_THAT(rd_kw_iget_float(kw1.get(), list.at(0)),
                             Catch::Matchers::WithinAbs(15.0f, 0.01));
            }

            SECTION("Keyword isub") {
                auto kw1 = make_rd_kw("KW1", 1000, RD_FLOAT);
                auto kw2 = make_rd_kw("KW2", 1000, RD_FLOAT);
                for (int i = 0; i < 1000; i++) {
                    rd_kw_iset_float(kw1.get(), i, 10.0f);
                    rd_kw_iset_float(kw2.get(), i, 3.0f);
                }
                rd_region_kw_isub(region, kw1.get(), kw2.get(), false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE_THAT(rd_kw_iget_float(kw1.get(), list.at(0)),
                             Catch::Matchers::WithinAbs(7.0f, 0.01f));
            }

            SECTION("Keyword imul") {
                auto kw1 = make_rd_kw("KW1", 1000, RD_FLOAT);
                auto kw2 = make_rd_kw("KW2", 1000, RD_FLOAT);
                for (int i = 0; i < 1000; i++) {
                    rd_kw_iset_float(kw1.get(), i, 10.0f);
                    rd_kw_iset_float(kw2.get(), i, 2.0f);
                }
                rd_region_kw_imul(region, kw1.get(), kw2.get(), false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE_THAT(rd_kw_iget_float(kw1.get(), list.at(0)),
                             Catch::Matchers::WithinAbs(20.0f, 0.01f));
            }

            SECTION("Keyword idiv") {
                auto kw1 = make_rd_kw("KW1", 1000, RD_FLOAT);
                auto kw2 = make_rd_kw("KW2", 1000, RD_FLOAT);
                for (int i = 0; i < 1000; i++) {
                    rd_kw_iset_float(kw1.get(), i, 20.0f);
                    rd_kw_iset_float(kw2.get(), i, 4.0f);
                }
                rd_region_kw_idiv(region, kw1.get(), kw2.get(), false);
                const auto list = rd_region_get_global_list(region);
                REQUIRE_THAT(rd_kw_iget_float(kw1.get(), list.at(0)),
                             Catch::Matchers::WithinAbs(5.0f, 0.01f));
            }

            SECTION("Sum keyword int") {
                auto kw = make_rd_kw("TEST", 1000, RD_INT);
                for (int i = 0; i < 1000; i++)
                    rd_kw_iset_int(kw.get(), i, 2);
                int sum = rd_region_sum_kw_int(region, kw.get(), false);
                REQUIRE(sum == 1000);
            }

            SECTION("Sum keyword float") {
                auto kw = make_rd_kw("TEST", 1000, RD_FLOAT);
                for (int i = 0; i < 1000; i++)
                    rd_kw_iset_float(kw.get(), i, 2.5f);
                float sum = rd_region_sum_kw_float(region, kw.get(), false);
                REQUIRE_THAT(sum, Catch::Matchers::WithinAbs(1250.0f, 0.01f));
            }

            SECTION("Sum keyword double") {
                auto kw = make_rd_kw("TEST", 1000, RD_DOUBLE);
                for (int i = 0; i < 1000; i++)
                    rd_kw_iset_double(kw.get(), i, 2.5);
                double sum = rd_region_sum_kw_double(region, kw.get(), false);
                REQUIRE_THAT(sum, Catch::Matchers::WithinAbs(1250.0, 0.01f));
            }
        }
        rd_region_free(region);
    }
}
