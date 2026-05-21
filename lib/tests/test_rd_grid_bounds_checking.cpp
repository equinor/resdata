#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <resdata/rd_grid.hpp>
#include <vector>

#include "grid_fixtures.hpp"
#include "resdata/rd_kw_magic.hpp"
#include "tmpdir.hpp"

TEST_CASE_METHOD(Tmpdir, "Negative hostnum raises") {
    auto filename = dirname / "NEGATIVE_HOSTNUM.EGRID";

    auto hostnum_override = GENERATE(-1, -2);
    write_egrid_with_single_lgr(filename, 2, 2, 2, 1, 1, 1, 0, 0, 0, "LGR1",
                                nullptr, {}, {}, hostnum_override);

    REQUIRE_THROWS(read_grid(filename));
}

TEST_CASE_METHOD(Tmpdir, "Hostnum bounds are checked for EGRID", "[security]") {
    auto filename = dirname / "LARGE_HOSTNUM.EGRID";
    write_egrid_with_single_lgr(filename, 2, 2, 2, 1, 1, 1, 0, 0, 0, "LGR1",
                                nullptr, {}, {},
                                /*hostnum_override=*/1000000);

    REQUIRE_THROWS(read_grid(filename));
}

TEST_CASE_METHOD(Tmpdir, "Hostnum bounds are checked for GRID") {
    auto filename = dirname / "LARGE_HOSTNUM.GRID";
    write_grid_file_with_lgrs(
        filename, 1, 1, 1,
        {LgrGrid{"LGR1", "Global", true, 1, 1, 1, /*host_cell=*/10000}});

    REQUIRE_THROWS(read_grid(filename));
}

TEST_CASE_METHOD(Tmpdir, "Zero nncg raises") {
    auto filename = dirname / "EVIL_NNCG_ZERO.EGRID";
    write_egrid_with_single_lgr(filename, 2, 2, 2, 1, 1, 1, 0, 0, 0, "LGR1",
                                /*mapaxes=*/nullptr,
                                /*nncg=*/{0}, /*nncl=*/{1});

    REQUIRE_THROWS(read_grid(filename));
}

TEST_CASE_METHOD(Tmpdir, "Large nncg raises") {
    auto filename = dirname / "EVIL_NNCG_LARGE.EGRID";
    write_egrid_with_single_lgr(filename, 2, 2, 2, 1, 1, 1, 0, 0, 0, "LGR1",
                                nullptr, /*nncg=*/{1000000}, {1});

    REQUIRE_THROWS(read_grid(filename));
}

namespace {

void write_egrid_with_sized_mapaxes(const fs::path &filename, int nx, int ny,
                                    int nz, int mapaxes_size) {
    auto grid = make_rectangular_grid(nx, ny, nz, 1.0, 1.0, 1.0, nullptr);
    auto fortio = make_fortio_writer(filename);

    write_egrid_filehead(fortio);
    std::vector<float> mapaxes(mapaxes_size, 0.0f);
    write_float_kw(fortio.get(), MAPAXES_KW, mapaxes.data(), mapaxes_size);
    write_egrid_gridhead(fortio, nx, ny, nz, 0);
    write_egrid_grid_body(grid.get(), fortio);
    write_empty_kw(fortio, ENDGRID_KW);
}

void write_egrid_with_sized_corsnum(const fs::path &filename, int nx, int ny,
                                    int nz, int corsnum_size) {
    auto grid = make_rectangular_grid(nx, ny, nz, 1.0, 1.0, 1.0, nullptr);
    auto fortio = make_fortio_writer(filename);

    write_egrid_filehead(fortio);
    write_egrid_gridhead(fortio, nx, ny, nz, 0);
    write_egrid_grid_body(grid.get(), fortio);
    std::vector<int> corsnum(corsnum_size, 0);
    write_int_kw(fortio.get(), CORSNUM_KW, corsnum.data(), corsnum_size);
    write_empty_kw(fortio, ENDGRID_KW);
}

} // namespace

TEST_CASE_METHOD(Tmpdir, "Wrong size MAPAXES raises for EGRID") {
    auto filename = dirname / "BAD_MAPAXES.EGRID";
    int bad_size = 7;
    write_egrid_with_sized_mapaxes(filename, 2, 2, 2, bad_size);

    REQUIRE_THROWS_WITH(read_grid(filename),
                        Catch::Matchers::ContainsSubstring("MAPAXES"));
}

TEST_CASE_METHOD(Tmpdir, "Wrong size CORSNUM raises for EGRID") {
    auto filename = dirname / "BAD_CORSNUM.EGRID";
    const int nx = 2, ny = 2, nz = 2;
    int bad_size = 7;
    write_egrid_with_sized_corsnum(filename, nx, ny, nz, bad_size);

    REQUIRE_THROWS_WITH(read_grid(filename),
                        Catch::Matchers::ContainsSubstring("CORSNUM"));
}
