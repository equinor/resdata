#include <catch2/catch.hpp>
#include <resdata/rd_util.hpp>

using namespace Catch;
using namespace Matchers;

TEST_CASE("Test file type format check", "[unittest]") {
    bool is_fmt = false;
    SECTION("A FUNRST file is a formatted file") {
        rd_get_file_type("CASE.FUNRST", &is_fmt, NULL);
        REQUIRE(is_fmt);
    }

    SECTION("A FMSPEC file is a formatted file") {
        rd_get_file_type("CASE.FMSPEC", &is_fmt, NULL);
        REQUIRE(is_fmt);
    }

    SECTION("A .F0001 file is a formatted file") {
        rd_get_file_type("CASE.F0001", &is_fmt, NULL);
        REQUIRE(is_fmt);
    }

    SECTION("A .X1234 file is an unformatted file") {
        rd_get_file_type("CASE.X1234", &is_fmt, NULL);
        REQUIRE(!is_fmt);
    }

    SECTION("A .UNSMRY file is an unformatted file") {
        rd_get_file_type("CASE.UNSMRY", &is_fmt, NULL);
        REQUIRE(!is_fmt);
    }
}
