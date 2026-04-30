#include <catch2/catch.hpp>
#include <resdata/rd_grid.hpp>

#include "grid_fixtures.hpp"
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
