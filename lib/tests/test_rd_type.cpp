#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <stdexcept>

#include <resdata/rd_type.hpp>

TEST_CASE("rd_type invalid input throws", "[unittest]") {
    SECTION("invalid enum throws invalid_argument") {
        REQUIRE_THROWS_AS(
            rd_type_create_from_type(static_cast<rd_type_enum>(6)),
            std::invalid_argument);
        REQUIRE_THROWS_WITH(
            rd_type_create_from_type(static_cast<rd_type_enum>(6)),
            Catch::Matchers::ContainsSubstring("invalid rd_type"));
    }

    SECTION("invalid type name throws invalid_argument") {
        REQUIRE_THROWS_AS(rd_type_create_from_name("NOPE"),
                          std::invalid_argument);
        REQUIRE_THROWS_WITH(
            rd_type_create_from_name("NOPE"),
            Catch::Matchers::ContainsSubstring("unrecognized type name"));
    }

    SECTION("string type from type-alone throws invalid_argument") {
        REQUIRE_THROWS_AS(rd_type_create_from_type(RD_STRING_TYPE),
                          std::invalid_argument);
        REQUIRE_THROWS_WITH(
            rd_type_create_from_type(RD_STRING_TYPE),
            Catch::Matchers::ContainsSubstring(
                "Variable length string type cannot be created"));
    }

    SECTION("invalid internal type in rd_type_name throws") {
        const rd_data_type invalid{static_cast<rd_type_enum>(6), 0};

        REQUIRE_THROWS_AS(rd_type_name(invalid), std::invalid_argument);
        REQUIRE_THROWS_WITH(
            rd_type_name(invalid),
            Catch::Matchers::ContainsSubstring("internal eclipse_type"));
    }
}
