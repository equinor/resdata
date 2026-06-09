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
            ERT::FortIO fortio(filename.string(), std::ios_base::out, false);
            fortio_fwrite_record(fortio.get(), "", 0);
        }

        ERT::FortIO fortio(filename.string(), mode, false);

        THEN("The stream is open") {
            REQUIRE(fortio_stream_is_open(fortio.get()));
        }
        THEN("Seeking with unknown whence fails") {
            if (mode == std::ios_base::in)
                REQUIRE_THROWS_AS(fortio_fseek(fortio.get(), 0, 2000),
                                  std::invalid_argument);
            else
                REQUIRE_FALSE(fortio_fseek(fortio.get(), 0, 2000));
        }
        THEN("Seeking data with negative count raises") {
            REQUIRE_THROWS_AS(fortio_data_fseek(fortio.get(), 0, 1, 1, -1, 1),
                              std::invalid_argument);
        }
        THEN("Seeking data beyond count raises") {
            REQUIRE_THROWS_AS(fortio_data_fseek(fortio.get(), 0, 6, 1, 5, 1),
                              std::invalid_argument);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "opening FortIO into non-existent entries") {
    GIVEN("A non-existent directory") {
        auto filename = (dirname / "does_not_exist" / "CASE.EGRID");
        bool fmt = GENERATE(false, true);
        THEN("Using read mode throws") {
            REQUIRE_THROWS_AS(
                ERT::FortIO(filename.string(), std::ios_base::in, fmt),
                std::invalid_argument);
        }
        THEN("Other modes will return nullptr") {
            auto mode = GENERATE(std::ios_base::out, std::ios_base::app);
            REQUIRE(ERT::FortIO(filename.string(), mode, fmt).get() == nullptr);
        }
    }
    GIVEN("A non-existent file") {
        auto filename = (dirname / "does_not_exist.EGRID");
        bool fmt = GENERATE(false, true);
        SECTION("Using read mode throws") {
            REQUIRE_THROWS_AS(
                ERT::FortIO(filename.string(), std::ios_base::in, fmt),
                std::invalid_argument);
        }
        SECTION("Append and write succeeds") {
            auto mode = GENERATE(std::ios_base::out, std::ios_base::app);
            REQUIRE(ERT::FortIO(filename.string(), mode, fmt).get());
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
            ERT::FortIO fortio(filename.string(), std::ios_base::out, false);
            fortio_fwrite_record(fortio.get(), "", 0);
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in, false);

        THEN("Reading zero sized buffer moves the stream past the markers") {
            auto pos = fortio_ftell(fortio.get());
            fortio_fread_buffer(fortio.get(), nullptr, 0);
            REQUIRE(fortio_ftell(fortio.get()) == pos + 2 * 4);
        }
    }
    GIVEN("A fortio file with a record of length 1") {
        auto filename = (dirname / "CASE.EGRID");
        {
            ERT::FortIO fortio(filename.string(), std::ios_base::out, false);
            fortio_fwrite_record(fortio.get(), "A", 1);
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in, false);

        THEN("Reading zero sized buffer moves the stream past the markers") {
            auto pos = fortio_ftell(fortio.get());
            std::array<char, 1> buffer = {0};
            bool ok = fortio_fread_buffer(fortio.get(), buffer.data(), 1);
            REQUIRE(ok);
            REQUIRE(fortio_ftell(fortio.get()) == pos + 2 * 4 + 1);
            REQUIRE(buffer[0] == 'A');
        }
    }
    GIVEN("A fortio file with two records") {
        auto filename = (dirname / "CASE.EGRID");
        {
            ERT::FortIO fortio(filename.string(), std::ios_base::out, false);
            fortio_fwrite_record(fortio.get(), "A", 1);
            fortio_fwrite_record(fortio.get(), "BB", 2);
        }

        ERT::FortIO fortio(filename.string(), std::ios_base::in, false);

        THEN("read_buffer will read records until size is filled") {
            auto pos = fortio_ftell(fortio.get());
            std::array<char, 1> buffer = {0};

            bool ok = fortio_fread_buffer(fortio.get(), buffer.data(), 1);
            REQUIRE(ok);
            REQUIRE(fortio_ftell(fortio.get()) == pos + 2 * 4 + 1);
            REQUIRE(buffer[0] == 'A');
        }

        THEN("read_buffer will read all records to fill size") {
            auto pos = fortio_ftell(fortio.get());
            std::array<char, 3> buffer = {0, 0, 0};

            bool ok = fortio_fread_buffer(fortio.get(), buffer.data(), 3);
            REQUIRE(ok);
            REQUIRE(fortio_ftell(fortio.get()) == pos + 4 * 4 + 3);
            REQUIRE(buffer[0] == 'A');
            REQUIRE(buffer[1] == 'B');
            REQUIRE(buffer[2] == 'B');
        }

        THEN("read_buffer will fail if not enough data in records") {
            std::array<char, 4> buffer = {0, 0, 0, 0};
            REQUIRE_FALSE(fortio_fread_buffer(fortio.get(), buffer.data(), 4));
        }

        THEN("read_buffer will fail if record exceeds remaining buffer size") {
            std::array<char, 2> buffer = {0, 0};
            REQUIRE_FALSE(fortio_fread_buffer(fortio.get(), buffer.data(), 2));
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

        ERT::FortIO fortio(filename.string(), std::ios_base::in, false);

        THEN("read_buffer will fail") {
            std::array<char, 1> buffer = {0};
            REQUIRE_FALSE(fortio_fread_buffer(fortio.get(), buffer.data(), 1));
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

        ERT::FortIO fortio(filename.string(), std::ios_base::in, false);

        THEN("read_buffer will fail") {
            std::array<char, 1> buffer = {0};
            REQUIRE_FALSE(fortio_fread_buffer(fortio.get(), buffer.data(), 1));
        }
    }
}
