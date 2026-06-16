#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstddef>
#include <cstdint>

#include <array>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

#include <resdata/FortIO.hpp>

#include "tmpdir.hpp"

TEST_CASE_METHOD(Tmpdir, "Basic FortIO operations") {
    GIVEN("A fortio instance opened in various modes") {
        auto filename = (dirname / "CASE.EGRID");
        auto mode =
            GENERATE(std::ios_base::out, std::ios_base::app, std::ios_base::in);
        if (mode == std::ios_base::in) {
            ERT::FortIO fortio(filename.string(), std::ios_base::out);
            fortio.fwrite_record("", 0);
        }

        ERT::FortIO fortio(filename.string(), mode);

        THEN("The stream is open") { REQUIRE(fortio.stream_is_open()); }
        THEN("Seeking with unknown whence fails") {
            if (mode == std::ios_base::in)
                REQUIRE_THROWS_AS(fortio.fseek(0, 2000), std::invalid_argument);
            else
                REQUIRE_FALSE(fortio.fseek(0, 2000));
        }
        THEN("Seeking data with negative count raises") {
            REQUIRE_THROWS_AS(fortio.data_fseek(0, 1, 1, -1, 1),
                              std::invalid_argument);
        }
        THEN("Seeking data beyond count raises") {
            REQUIRE_THROWS_AS(fortio.data_fseek(0, 6, 1, 5, 1),
                              std::invalid_argument);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "opening FortIO into non-existent entries") {
    GIVEN("A non-existent directory") {
        auto filename = (dirname / "does_not_exist" / "CASE.EGRID");
        bool fmt = GENERATE(false, true);
        auto mode =
            GENERATE(std::ios_base::out, std::ios_base::in, std::ios_base::app);
        THEN("Opening throws") {
            REQUIRE_THROWS_AS(ERT::FortIO(filename.string(), mode, fmt),
                              std::ios_base::failure);
        }
    }
    GIVEN("A non-existent file") {
        auto filename = (dirname / "does_not_exist.EGRID");
        bool fmt = GENERATE(false, true);
        SECTION("Using read mode throws") {
            REQUIRE_THROWS_AS(
                ERT::FortIO(filename.string(), std::ios_base::in, fmt),
                std::ios_base::failure);
        }
        SECTION("Append and write succeeds") {
            auto mode = GENERATE(std::ios_base::out, std::ios_base::app);
            REQUIRE_NOTHROW(ERT::FortIO(filename.string(), mode, fmt).get());
        }
    }
}

std::array<std::byte, 4> to_bytes_big(uint32_t value) {
    return {
        std::byte((value >> 24) & 0xFF),
        std::byte((value >> 16) & 0xFF),
        std::byte((value >> 8) & 0xFF),
        std::byte((value) & 0xFF),
    };
}
template <typename... Buffers>
std::vector<std::byte> concat(const Buffers &...buffers) {
    std::vector<std::byte> result;
    (result.insert(result.end(), buffers.begin(), buffers.end()), ...);
    return result;
}

TEST_CASE_METHOD(Tmpdir, "Reading data with FortIO") {
    GIVEN("A fortio file with a zero record") {
        auto filename = (dirname / "CASE.EGRID");
        {
            ERT::FortIO fortio(filename.string(), std::ios_base::out);
            fortio.fwrite_record("", 0);
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in);

        THEN("Reading zero sized buffer moves the stream past the markers") {
            auto pos = fortio.ftell();
            fortio.fread_buffer(nullptr, 0);
            REQUIRE(fortio.ftell() == pos + 2 * 4);
        }
    }
    GIVEN("A fortio file with a record of length 1") {
        auto filename = (dirname / "CASE.EGRID");
        {
            ERT::FortIO fortio(filename.string(), std::ios_base::out);
            fortio.fwrite_record("A", 1);
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in);

        THEN("Reading zero sized buffer moves the stream past the markers") {
            auto pos = fortio.ftell();
            std::array<char, 1> buffer = {0};
            bool ok = fortio.fread_buffer(buffer.data(), 1);
            REQUIRE(ok);
            REQUIRE(fortio.ftell() == pos + 2 * 4 + 1);
            REQUIRE(buffer[0] == 'A');
        }
    }
    GIVEN("A fortio file with two records") {
        auto filename = (dirname / "CASE.EGRID");
        {
            ERT::FortIO fortio(filename.string(), std::ios_base::out);
            fortio.fwrite_record("A", 1);
            fortio.fwrite_record("BB", 2);
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in);

        THEN("read_buffer will read records until size is filled") {
            auto pos = fortio.ftell();
            std::array<char, 1> buffer = {0};

            bool ok = fortio.fread_buffer(buffer.data(), 1);
            REQUIRE(ok);
            REQUIRE(fortio.ftell() == pos + 2 * 4 + 1);
            REQUIRE(buffer[0] == 'A');
        }

        THEN("read_buffer will read all records to fill size") {
            auto pos = fortio.ftell();
            std::array<char, 3> buffer = {0, 0, 0};

            bool ok = fortio.fread_buffer(buffer.data(), 3);
            REQUIRE(ok);
            REQUIRE(fortio.ftell() == pos + 4 * 4 + 3);
            REQUIRE(buffer[0] == 'A');
            REQUIRE(buffer[1] == 'B');
            REQUIRE(buffer[2] == 'B');
        }

        THEN("read_buffer will fail if not enough data in records") {
            std::array<char, 4> buffer = {0, 0, 0, 0};
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), 4));
        }

        THEN("read_buffer will fail if record exceeds remaining buffer size") {
            std::array<char, 2> buffer = {0, 0};
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), 2));
        }
    }

    GIVEN("A fortio file with a negative record") {
        auto filename = (dirname / "CASE.EGRID");
        {
            auto content = concat(to_bytes_big(-1), to_bytes_big(-1));
            std::ofstream file(filename, std::ios::binary);
            file.write(reinterpret_cast<const char *>(content.data()),
                       content.size());
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in);

        THEN("read_buffer will fail") {
            std::array<char, 1> buffer = {0};
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), 1));
        }
    }
    GIVEN("A fortio file with mismatched records") {
        auto filename = (dirname / "file.bin");
        {
            auto content = concat(to_bytes_big(1), to_bytes_big(3));
            std::ofstream file(filename, std::ios::binary);
            file.write(reinterpret_cast<const char *>(content.data()),
                       content.size());
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in);

        THEN("read_buffer will fail") {
            std::array<char, 1> buffer = {0};
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), 1));
        }
    }
}
