#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <ert/util/parser.hpp>
#include <ert/util/util.hpp>

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

TEST_CASE("Tokenizing a quoted string keeps or strips the quotation marks",
          "[unittest][parser]") {
    parser_ptr parser{basic_parser_alloc(" ", "'", NULL, NULL, NULL, NULL)};
    const char *buffer = "a 'my \\'doc.txt' b";

    CHECK(basic_parser_tokenize_buffer(parser.get(), buffer, false) ==
          std::vector<std::string>{"a", "'my \\'doc.txt'", "b"});
    CHECK(basic_parser_tokenize_buffer(parser.get(), buffer, true) ==
          std::vector<std::string>{"a", "my 'doc.txt", "b"});
}

TEST_CASE("Tokenizing an unterminated quotation throws invalid_argument",
          "[unittest][parser]") {
    parser_ptr parser{basic_parser_alloc(" ", "'", NULL, NULL, NULL, NULL)};

    CHECK_THROWS_AS(basic_parser_tokenize_buffer(parser.get(), "a 'b", true),
                    std::invalid_argument);
}

TEST_CASE("util_same_file identifies files by identity", "[unittest]") {
    namespace fs = std::filesystem;
    const fs::path dir =
        fs::temp_directory_path() / "resdata_util_same_file_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const fs::path a = dir / "a";
    const fs::path b = dir / "b";
    std::ofstream(a) << "x";
    std::ofstream(b) << "x";
    fs::create_symlink(a, dir / "link");

    CHECK(util_same_file(a.c_str(), a.c_str()));
    CHECK(util_same_file(a.c_str(), (dir / "link").c_str()));
    CHECK_FALSE(util_same_file(a.c_str(), b.c_str()));
    CHECK_FALSE(util_same_file(a.c_str(), (dir / "missing").c_str()));
    CHECK_FALSE(
        util_same_file((dir / "missing").c_str(), (dir / "missing").c_str()));

    fs::remove_all(dir);
}
