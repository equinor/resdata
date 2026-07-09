#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <resdata/rd_util.hpp>

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

TEST_CASE("Test checked allocation size_t overflow guard", "[unittest]") {
    // A count that, when multiplied by sizeof(T), overflows size_t.
    const size_t overflowing = SIZE_MAX / sizeof(int) + 1;

    SECTION("checked_malloc throws bad_alloc on overflow") {
        REQUIRE_THROWS_AS(rd::checked_malloc<int>(overflowing), std::bad_alloc);
    }

    SECTION("checked_calloc throws bad_alloc on overflow") {
        REQUIRE_THROWS_AS(rd::checked_calloc<int>(overflowing), std::bad_alloc);
    }

    SECTION("checked_realloc throws bad_alloc on overflow") {
        auto ptr = rd::checked_malloc<int>(1);
        REQUIRE_THROWS_AS(rd::checked_realloc<int>(ptr, overflowing),
                          std::bad_alloc);
        // The original allocation must be preserved when realloc guard trips.
        REQUIRE(ptr != nullptr);
    }

    SECTION("Non-overflowing allocations succeed") {
        auto malloc_ptr = rd::checked_malloc<int>(4);
        REQUIRE(malloc_ptr != nullptr);

        auto calloc_ptr = rd::checked_calloc<int>(4);
        REQUIRE(calloc_ptr != nullptr);
        for (int i = 0; i < 4; ++i) {
            REQUIRE(calloc_ptr[i] == 0);
        }

        rd::checked_realloc<int>(malloc_ptr, 8);
        REQUIRE(malloc_ptr != nullptr);
    }
}

TEST_CASE("Test checked allocation of zero elements returns nullptr",
          "[unittest]") {
    SECTION("checked_malloc returns nullptr") {
        REQUIRE(rd::checked_malloc<int>(0) == nullptr);
    }

    SECTION("checked_calloc returns nullptr") {
        REQUIRE(rd::checked_calloc<int>(0) == nullptr);
    }

    SECTION("checked_realloc releases the existing allocation") {
        auto ptr = rd::checked_malloc<int>(4);
        REQUIRE(ptr != nullptr);
        rd::checked_realloc<int>(ptr, 0);
        REQUIRE(ptr == nullptr);
    }

    SECTION("checked_realloc of an empty pointer stays null") {
        std::unique_ptr<int[], void (*)(void *)> ptr{nullptr, &std::free};
        rd::checked_realloc<int>(ptr, 0);
        REQUIRE(ptr == nullptr);
    }
}
