#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <ert/util/util.hpp>
#include <filesystem>

#include "tmpdir.hpp"

TEST_CASE_METHOD(Tmpdir, "Test getcwd after unlink cwd", "[unittest]") {
    GIVEN("Test directory") {
        auto subdir = dirname / "unlink";
        auto previous_cwd = fs::current_path();
        fs::create_directory(subdir);
        fs::current_path(subdir);

        THEN("Use normally") {
            auto path = util_alloc_cwd();
            std::free(path);
        }

        THEN("Unlink") {
            fs::remove(subdir);
            CHECK_THROWS(util_alloc_cwd());
        }

        fs::current_path(previous_cwd);
    }
}

TEST_CASE("util_endian_flip_vector reverses the bytes of each element",
          "[unittest][endian]") {
    SECTION("2 byte elements") {
        std::array<uint16_t, 3> values{0x1234, 0xABCD, 0x00FF};
        util_endian_flip_vector(values.data(), 2, values.size());
        CHECK(values == std::array<uint16_t, 3>{0x3412, 0xCDAB, 0xFF00});
    }

    SECTION("4 byte elements") {
        std::array<uint32_t, 3> values{0x12345678, 0xAABBCCDD, 0x000000FF};
        util_endian_flip_vector(values.data(), 4, values.size());
        CHECK(values ==
              std::array<uint32_t, 3>{0x78563412, 0xDDCCBBAA, 0xFF000000});
    }

    SECTION("8 byte elements") {
        std::array<uint64_t, 2> values{0x0123456789ABCDEF, 0x00000000000000FF};
        util_endian_flip_vector(values.data(), 8, values.size());
        CHECK(values ==
              std::array<uint64_t, 2>{0xEFCDAB8967452301, 0xFF00000000000000});
    }

    SECTION("1 byte elements are left alone") {
        std::array<uint8_t, 3> values{1, 2, 3};
        util_endian_flip_vector(values.data(), 1, values.size());
        CHECK(values == std::array<uint8_t, 3>{1, 2, 3});
    }

    SECTION("flipping twice restores the original") {
        const std::array<uint32_t, 5> original{1, 2, 0xDEADBEEF, 0, 0xFFFFFFFF};
        auto values = original;
        util_endian_flip_vector(values.data(), 4, values.size());
        util_endian_flip_vector(values.data(), 4, values.size());
        CHECK(values == original);
    }
}

TEST_CASE("util_endian_flip_vector does not require aligned buffers",
          "[unittest][endian]") {
    std::array<uint64_t, 4> storage{};
    auto *unaligned = reinterpret_cast<std::byte *>(storage.data()) + 4;

    const std::array<uint32_t, 3> input{0x12345678, 0xAABBCCDD, 0x000000FF};
    std::memcpy(unaligned, input.data(), sizeof(input));

    util_endian_flip_vector(unaligned, 4, input.size());

    std::array<uint32_t, 3> result{};
    std::memcpy(result.data(), unaligned, sizeof(result));
    CHECK(result ==
          std::array<uint32_t, 3>{0x78563412, 0xDDCCBBAA, 0xFF000000});
}

TEST_CASE("util_endian_flip_vector handles odd element counts",
          "[unittest][endian]") {
    auto elements = GENERATE(size_t{1}, 2, 3, 4, 5);
    std::vector<uint32_t> values(elements, 0x12345678);

    util_endian_flip_vector(values.data(), 4, elements);

    CHECK(values == std::vector<uint32_t>(elements, 0x78563412));
}
