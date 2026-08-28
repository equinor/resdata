/**
 * test_rd_grid_misc.cpp
 * ---------------------
 * Miscellaneous grid tests that do not fit the other files cleanly.
 *
 * These tests exercise grid file I/O paths (rd_grid_exists, writing GRID
 * and EGRID files) using a simple rectangular grid as input. They ensure
 * that writing out a grid and reading it back yields a grid that compares
 * equal to the original.
 */

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <filesystem>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>

#include "grid_fixtures.hpp"
#include "resdata/rd_type.hpp"
#include "resdata/rd_util.hpp"
#include "tmpdir.hpp"

using namespace Catch;

TEST_CASE_METHOD(Tmpdir, "Test grid file I/O", "[unittest]") {
    GIVEN("A grid") {
        auto grid = make_rectangular_grid(3, 3, 3, 1, 1, 1, nullptr);

        SECTION("exists") {
            auto filename = (dirname / "TEST.EGRID");
            rd_grid_fwrite_EGRID(grid.get(), filename.c_str(), true);
            REQUIRE(rd_grid_exists(filename.c_str()));

            auto filename2 = (dirname / "TEST2.EGRID");
            rd_grid_fwrite_EGRID2(grid.get(), filename2.c_str(),
                                  UnitSystem::METRIC);
            REQUIRE(rd_grid_exists(filename2.c_str()));
        }

        SECTION("write as GRID") {
            auto filename = (dirname / "TEST.GRID");
            rd_grid_fwrite_GRID2(grid.get(), filename.c_str(),
                                 UnitSystem::METRIC);

            auto loaded = read_grid(filename);
            REQUIRE(loaded != nullptr);
            REQUIRE(
                rd_grid_compare(grid.get(), loaded.get(), false, false, false));
        }

        SECTION("load_case__") {
            auto filename = (dirname / "TEST3.EGRID");
            rd_grid_fwrite_EGRID2(grid.get(), filename.c_str(),
                                  UnitSystem::METRIC);

            auto loaded = rd_grid_ptr(
                rd_grid_load_case__(filename.c_str(), true), &rd_grid_free);
            REQUIRE(loaded != nullptr);
            REQUIRE(
                rd_grid_compare(grid.get(), loaded.get(), false, false, false));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Writing every grid to disk", "[unittest]") {
    auto grids = build_all_grids(dirname);

    for (auto &entry : grids) {
        INFO("writing grid: " << entry.label);

        SECTION(
            ("fwrite_EGRID then rd_grid_alloc (" + entry.label + ")").c_str()) {
            auto filename = dirname / ("WRITE_" + entry.label + ".EGRID");
            rd_grid_fwrite_EGRID(entry.grid.get(), filename.c_str(), true);
            REQUIRE(fs::exists(filename));
            auto reloaded = read_grid(filename);
            REQUIRE(reloaded != nullptr);
        }

        SECTION(("fwrite_EGRID2 then rd_grid_alloc (" + entry.label + ")")
                    .c_str()) {
            auto filename = dirname / ("WRITE2_" + entry.label + ".EGRID");
            rd_grid_fwrite_EGRID2(entry.grid.get(), filename.c_str(),
                                  UnitSystem::METRIC);
            REQUIRE(fs::exists(filename));
            auto reloaded = read_grid(filename);
            REQUIRE(reloaded != nullptr);
        }
    }
}
