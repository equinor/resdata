#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <optional>
#include <utility>
#include <stdexcept>

#include "detail/resdata/rd_sum_file_data.hpp"

TEST_CASE("TimeIndex maps report steps to their internal index range") {
    rd::TimeIndex index;

    index.add(100, 0.0, 1);
    index.add(200, 100.0, 1);
    index.add(300, 200.0, 2);

    REQUIRE(index.size() == 3);

    const auto first = index.report_range(1);
    REQUIRE(first.has_value());
    REQUIRE(first->first == 0);
    REQUIRE(first->last == 1);

    const auto second = index.report_range(2);
    REQUIRE(second.has_value());
    REQUIRE(second->first == 2);
    REQUIRE(second->last == 2);
}

TEST_CASE("TimeIndex reports holes in the report step sequence as empty") {
    rd::TimeIndex index;

    index.add(100, 0.0, 1);
    index.add(300, 200.0, 3);

    /* Report step 0 was never added, 2 is a hole and 4 is past the end; none
       of them may be confused with a valid internal index range. */
    REQUIRE_FALSE(index.has_report(0));
    REQUIRE_FALSE(index.has_report(2));
    REQUIRE_FALSE(index.has_report(4));
    REQUIRE(index.report_range(0) == std::nullopt);
    REQUIRE(index.report_range(2) == std::nullopt);
    REQUIRE(index.report_range(4) == std::nullopt);

    REQUIRE(index.has_report(1));
    REQUIRE(index.has_report(3));
}

TEST_CASE("TimeIndex clear() empties both the nodes and the report map") {
    rd::TimeIndex index;

    index.add(100, 0.0, 1);
    REQUIRE_FALSE(index.empty());

    index.clear();

    REQUIRE(index.empty());
    REQUIRE(index.size() == 0);
    REQUIRE_FALSE(index.has_report(1));
}
