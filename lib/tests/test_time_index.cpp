#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <utility>
#include <stdexcept>

#include "detail/resdata/rd_sum_file_data.hpp"

TEST_CASE("TimeIndex rejects negative report steps") {
    rd::TimeIndex index;

    REQUIRE_THROWS_WITH(
        index.add(0, 0.0, -1),
        Catch::Matchers::ContainsSubstring("report step cannot be negative"));
    REQUIRE(index.size() == 0);
}
