/**
 * test_rd_grid_misc.cpp
 * ---------------------
 * Miscellaneous grid tests that do not fit the other files cleanly.
 *
 * These tests exercise grid file I/O paths (rd_grid_exists, writing GRID
 * and GRDECL files, printing keywords as GRDECL text) using a simple
 * rectangular grid as input. They ensure that writing out a grid and
 * reading it back yields a grid that compares equal to the original.
 */

#include <catch2/catch.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_grdecl.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <vector>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

TEST_CASE_METHOD(Tmpdir, "Test grid file I/O", "[unittest]") {
    GIVEN("A grid") {
        auto grid =
            rd_grid_ptr(rd_grid_alloc_rectangular(3, 3, 3, 1, 1, 1, nullptr),
                        &rd_grid_free);

        SECTION("exists") {
            auto filename = (dirname / "TEST.EGRID");
            rd_grid_fwrite_EGRID(grid.get(), filename.c_str(), true);
            REQUIRE(rd_grid_exists(filename.c_str()));

            auto filename2 = (dirname / "TEST2.EGRID");
            rd_grid_fwrite_EGRID2(grid.get(), filename2.c_str(),
                                  RD_METRIC_UNITS);
            REQUIRE(rd_grid_exists(filename2.c_str()));
        }

        SECTION("write as GRID") {
            auto filename = (dirname / "TEST.GRID");
            rd_grid_fwrite_GRID2(grid.get(), filename.c_str(), RD_METRIC_UNITS);

            auto loaded =
                rd_grid_ptr(rd_grid_alloc(filename.c_str()), &rd_grid_free);
            REQUIRE(loaded != nullptr);
            REQUIRE(
                rd_grid_compare(grid.get(), loaded.get(), false, false, false));
        }

        SECTION("write as GRDECL") {
            auto filename = (dirname / "TEST.GRDECL");
            FILE *fp = fopen(filename.c_str(), "w");
            REQUIRE(fp != nullptr);
            rd_grid_fprintf_grdecl2(grid.get(), fp, RD_METRIC_UNITS);
            fclose(fp);
        }

        SECTION("load_case__") {
            auto filename = (dirname / "TEST3.EGRID");
            rd_grid_fwrite_EGRID2(grid.get(), filename.c_str(),
                                  RD_METRIC_UNITS);

            auto loaded = rd_grid_ptr(
                rd_grid_load_case__(filename.c_str(), true), &rd_grid_free);
            REQUIRE(loaded != nullptr);
            REQUIRE(
                rd_grid_compare(grid.get(), loaded.get(), false, false, false));
        }

        SECTION("print kw as GRDECL") {
            auto kw = make_rd_kw("PORO", rd_grid_get_global_size(grid.get()),
                                 RD_FLOAT);

            for (int i = 0; i < rd_grid_get_global_size(grid.get()); i++) {
                rd_kw_iset_float(kw.get(), i, 0.2f + i * 0.01f);
            }
            auto filename = (dirname / "KW.GRDECL");
            FILE *fp = fopen(filename.c_str(), "w");
            REQUIRE(fp != nullptr);
            rd_grid_grdecl_fprintf_kw(grid.get(), kw.get(), nullptr, fp,
                                      -999.0);
            fclose(fp);
        }
    }

    // When the keyword has one entry per active cell rather than one per
    // global cell, rd_grid_grdecl_fprintf_kw fills inactive slots with a
    // the type's default.
    GIVEN("A grid with inactive cells") {
        const int nx = 2, ny = 2, nz = 2;
        const int global_size = nx * ny * nz;
        std::vector<int> actnum(global_size, 1);
        actnum[0] = 0;
        actnum[3] = 0;
        auto grid = rd_grid_ptr(
            rd_grid_alloc_rectangular(nx, ny, nz, 1, 1, 1, actnum.data()),
            &rd_grid_free);
        const int nactive = rd_grid_get_active_size(grid.get());
        REQUIRE(nactive < global_size);

        auto print_kw = [&](const rd_kw_ptr &kw, const char *fname) {
            auto filename = (dirname / fname);
            auto fp = std::unique_ptr<FILE, void (*)(FILE *)>(
                fopen(filename.c_str(), "w"), [](FILE *f) {
                    if (f)
                        fclose(f);
                });
            REQUIRE(fp != nullptr);
            rd_grid_grdecl_fprintf_kw(grid.get(), kw.get(), nullptr, fp.get(),
                                      -999.0);
        };
        auto read_kw = [&](const char *fname, const char *kw, int size,
                           rd_data_type data_type) {
            auto filename = (dirname / fname);
            auto fp = std::unique_ptr<FILE, void (*)(FILE *)>(
                fopen(filename.c_str(), "r"), [](FILE *f) {
                    if (f)
                        fclose(f);
                });
            return rd_kw_ptr(
                rd_kw_fscanf_alloc_grdecl(fp.get(), kw, size, data_type),
                &rd_kw_free);
        };

        SECTION("active-sized FLOAT keyword sets default in inactive indices") {
            auto kw = make_rd_kw("PORO", nactive, RD_FLOAT);
            for (int a = 0; a < nactive; ++a)
                rd_kw_iset_float(kw.get(), a, 0.2f + a * 0.01f);
            print_kw(kw, "KW_ACTIVE_FLOAT.GRDECL");
            auto kw_read = read_kw("KW_ACTIVE_FLOAT.GRDECL", "PORO",
                                   global_size, RD_FLOAT);
            for (int i = 0, a = 0; i < global_size; ++i) {
                if (actnum[i])
                    REQUIRE(rd_kw_iget_float(kw_read.get(), i) ==
                            rd_kw_iget_float(kw.get(), a++));
                else
                    REQUIRE(rd_kw_iget_float(kw_read.get(), i) == -999.0f);
            }
        }

        SECTION("active-sized INT keyword sets default in inactive indices") {
            auto kw = make_rd_kw("SATNUM", nactive, RD_INT);
            for (int a = 0; a < nactive; ++a)
                rd_kw_iset_int(kw.get(), a, a + 1);
            print_kw(kw, "KW_ACTIVE_INT.GRDECL");
            auto kw_read =
                read_kw("KW_ACTIVE_INT.GRDECL", "SATNUM", global_size, RD_INT);
            for (int i = 0, a = 0; i < global_size; ++i) {
                if (actnum[i])
                    REQUIRE(rd_kw_iget_int(kw_read.get(), i) ==
                            rd_kw_iget_int(kw.get(), a++));
                else
                    REQUIRE(rd_kw_iget_int(kw_read.get(), i) == -999);
            }
        }

        SECTION("active-sized DOUBLE keyword sets 0.0 in inactive indices") {
            auto kw = make_rd_kw("PORO", nactive, RD_DOUBLE);
            for (int a = 0; a < nactive; ++a)
                rd_kw_iset_double(kw.get(), a, 0.2 + a * 0.01);
            print_kw(kw, "KW_ACTIVE_DOUBLE.GRDECL");
            auto kw_read = read_kw("KW_ACTIVE_DOUBLE.GRDECL", "PORO",
                                   global_size, RD_DOUBLE);
            for (int i = 0, a = 0; i < global_size; ++i) {
                if (actnum[i])
                    REQUIRE_THAT(
                        rd_kw_iget_double(kw_read.get(), i),
                        WithinAbs(rd_kw_iget_double(kw.get(), a++), 0.001));
                else
                    REQUIRE(rd_kw_iget_double(kw_read.get(), i) == -0.999);
            }
        }

        SECTION("active-sized BOOL keyword is written") {
            auto kw = make_rd_kw("FLAG", nactive, RD_BOOL);
            for (int a = 0; a < nactive; ++a)
                rd_kw_iset_bool(kw.get(), a, (a % 2) == 0);
            auto filename = (dirname / "KW_ACTIVE_BOOL.GRDECL");
            FILE *fp = fopen(filename.c_str(), "w");
            double default_val = GENERATE(0.0, 1.0);
            REQUIRE(fp != nullptr);
            rd_grid_grdecl_fprintf_kw(grid.get(), kw.get(), nullptr, fp,
                                      default_val);
            fclose(fp);
            // No support for reading grdecl bool
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Writing every grid to disk", "[unittest]") {
    auto grids = build_all_grids(dirname);

    for (auto &entry : grids) {
        INFO("writing grid: " << entry.label);
        SECTION(
            ("fprintf_grdecl2 produces a non-empty file (" + entry.label + ")")
                .c_str()) {
            auto filename = dirname / ("WRITE_" + entry.label + ".GRDECL");
            FILE *fp = fopen(filename.c_str(), "w");
            REQUIRE(fp != nullptr);
            rd_grid_fprintf_grdecl2(entry.grid.get(), fp, RD_METRIC_UNITS);
            fclose(fp);
            REQUIRE(fs::exists(filename));
            REQUIRE(fs::file_size(filename) > 0);
        }
    }
}
