#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstddef>
#include <cstdint>

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <ios>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <resdata/FortIO.hpp>

#include <ert/util/util.hpp>

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
        THEN("The stream cycles correctly through close and reopen") {
            REQUIRE(fortio.stream_is_open());
            REQUIRE(fortio.fclose_stream());
            REQUIRE_FALSE(fortio.stream_is_open());
            REQUIRE_FALSE(fortio.fclose_stream());
            REQUIRE(fortio.fopen_stream());
            REQUIRE(fortio.stream_is_open());
            REQUIRE_FALSE(fortio.fopen_stream());
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
            REQUIRE_NOTHROW(ERT::FortIO(filename.string(), mode, fmt));
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

template <typename T> const char *as_char(const T *p) {
    return reinterpret_cast<const char *>(p);
}

void write_records(
    const std::string &filename,
    std::initializer_list<std::pair<const char *, int>> records) {
    ERT::FortIO fortio(filename, std::ios_base::out);
    for (const auto &[data, size] : records)
        fortio.fwrite_record(data, size);
}

TEST_CASE_METHOD(Tmpdir, "Reading data with FortIO") {
    auto filename = (dirname / "CASE.EGRID").string();
    GIVEN("A fortio file with a zero record") {
        write_records(filename, {{"", 0}});

        ERT::FortIO fortio(filename, std::ios_base::in);

        THEN("Reading zero sized buffer moves the stream past the markers") {
            auto pos = fortio.ftell();
            fortio.fread_buffer(nullptr, 0);
            REQUIRE(fortio.ftell() == pos + 2 * 4);
        }
    }
    GIVEN("A fortio file with a record of length 1") {
        write_records(filename, {{"A", 1}});

        ERT::FortIO fortio(filename, std::ios_base::in);

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
        write_records(filename, {{"A", 1}, {"BB", 2}});

        ERT::FortIO fortio(filename, std::ios_base::in);

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

    GIVEN("A fortio file with invalid record markers") {
        auto [head, tail] = GENERATE(std::pair{-1, -1}, std::pair{1, 3});
        {
            auto content = concat(to_bytes_big(head), to_bytes_big(tail));
            std::ofstream file(filename, std::ios::binary);
            file.write(as_char(content.data()), content.size());
        }

        ERT::FortIO fortio(filename, std::ios_base::in);

        THEN("read_buffer will fail") {
            std::array<char, 1> buffer = {0};
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), 1));
        }
    }
    GIVEN("An externally managed FILE*") {
        write_records(filename, {{"", 0}});
        std::unique_ptr<FILE, decltype(&fclose)> stream(
            fopen(filename.c_str(), "r"), fclose);
        REQUIRE(stream);
        WHEN("Constructing a FortIO from the FILE*") {
            ERT::FortIO fortio(filename, false, false, stream.get(), false);

            THEN("fclose_stream returns false since the stream is not owned") {
                REQUIRE_FALSE(fortio.fclose_stream());
            }
            THEN("fopen_stream returns false since the stream is already set") {
                REQUIRE_FALSE(fortio.fopen_stream());
            }
            THEN("stream_is_open returns true") {
                REQUIRE(fortio.stream_is_open());
            }
        }
    }
    GIVEN("A file with two records where the second is truncated mid-data") {
        const int record_size = 1000;
        std::vector<char> buffer(record_size, 0);
        {
            ERT::FortIO fortio(filename, std::ios_base::out, false, false);
            fortio.fwrite_record(buffer.data(), record_size);
            fortio.fwrite_record(buffer.data(), record_size);
            fortio.ftruncate(2 * record_size - 100);
        }

        REQUIRE(util_file_size(filename.c_str()) == 2 * record_size - 100);

        THEN("The first record reads successfully but the second fails") {
            ERT::FortIO fortio(filename, std::ios_base::in, false, false);
            REQUIRE(fortio.fread_buffer(buffer.data(), record_size));
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), record_size));
        }
    }
    GIVEN("An empty file") {
        std::ofstream(filename).close();

        ERT::FortIO fortio(filename, std::ios_base::in, false, true);
        char buf = 0;

        THEN("fread_buffer fails and the stream is at EOF") {
            REQUIRE_FALSE(fortio.fread_buffer(&buf, 1));
            REQUIRE(fortio.read_at_eof());
        }
    }
    GIVEN("A file with a record whose tail marker is truncated") {
        const int record_size = 1000;
        std::vector<char> buffer(record_size, 0);
        {
            ERT::FortIO fortio(filename, std::ios_base::out, false, false);
            fortio.fwrite_record(buffer.data(), record_size);
            fortio.ftruncate(record_size + 4);
        }

        THEN("fread_buffer returns false") {
            ERT::FortIO fortio(filename, std::ios_base::in, false, false);
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), record_size));
        }
    }
    GIVEN("A file with a second record whose tail does not match its head") {
        const int record_size = 10;
        std::vector<char> buffer(record_size, 0);
        {
            std::ofstream file(filename, std::ios::binary);
            // First record: head=10, data, tail=10 (valid)
            file.write(as_char(&record_size), sizeof record_size);
            file.write(buffer.data(), record_size);
            file.write(as_char(&record_size), sizeof record_size);
            // Second record: head=10, data, tail=11 (mismatched)
            file.write(as_char(&record_size), sizeof record_size);
            file.write(buffer.data(), record_size);
            const int bad_tail = record_size + 1;
            file.write(as_char(&bad_tail), sizeof bad_tail);
        }

        ERT::FortIO fortio(filename, std::ios_base::in, false, false);

        THEN("The first record reads successfully but the second fails") {
            REQUIRE(fortio.fread_buffer(buffer.data(), record_size));
            REQUIRE_FALSE(fortio.fread_buffer(buffer.data(), record_size));
        }
    }

    GIVEN("A fortio file with one record") {
        {
            ERT::FortIO fortio(filename, std::ios_base::out, false, false);
            std::vector<char> buffer(100, 0);
            fortio.fwrite_record(buffer.data(), 100);
        }
        ERT::FortIO fortio(filename, std::ios_base::in, false, false);

        THEN("read_at_eof is false at the start and middle, true at the end") {
            REQUIRE_FALSE(fortio.read_at_eof());
            fortio.fseek(50, SEEK_SET);
            REQUIRE_FALSE(fortio.read_at_eof());
            fortio.fseek(0, SEEK_END);
            REQUIRE(fortio.read_at_eof());
        }
        THEN("Seeking to start and end of file succeeds") {
            REQUIRE(fortio.fseek(0, SEEK_SET));
            REQUIRE(fortio.fseek(0, SEEK_END));
        }
        THEN("Seeking far beyond the end of file fails") {
            REQUIRE_FALSE(fortio.fseek(100000, SEEK_END));
            REQUIRE_FALSE(fortio.fseek(100000, SEEK_SET));
        }
    }
    GIVEN("A writeable fortio file with one record written") {
        ERT::FortIO fortio(filename, std::ios_base::out, false, false);
        std::vector<char> buffer(100, 0);
        fortio.fwrite_record(buffer.data(), 100);

        THEN("After fwrite_error the file is deleted and writes are no-ops") {
            REQUIRE(std::filesystem::exists(filename));
            fortio.fwrite_error();
            REQUIRE_FALSE(std::filesystem::exists(filename));
            fortio.fwrite_record(buffer.data(), 100);
            REQUIRE_FALSE(std::filesystem::exists(filename));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Reading and writing with FortIO") {
    ERT::FortIO fortio("new_file", std::fstream::out);
    {
        std::vector<int> data(1000);
        std::iota(data.begin(), data.end(), 0);

        fortio.fwrite_record(as_char(data.data()), 1000 * 4);
    }
    fortio.close();

    fortio.open("new_file", std::fstream::app);
    {
        std::vector<int> data(1000);
        std::iota(data.begin(), data.end(), 0);

        fortio.fwrite_record(as_char(data.data()), 1000 * 4);
    }
    fortio.close();

    fortio.open("new_file", std::fstream::in);
    {
        std::vector<int> data(1000, 99);

        std::vector<int> expected(1000);
        std::iota(expected.begin(), expected.end(), 0);

        bool ok = fortio.fread_buffer(reinterpret_cast<char *>(data.data()),
                                      1000 * 4);
        REQUIRE(ok);
        REQUIRE(data == expected);

        ok = fortio.fread_buffer(reinterpret_cast<char *>(data.data()),
                                 1000 * 4);
        REQUIRE(ok);
        REQUIRE(data == expected);
    }
    REQUIRE_FALSE(fortio.ftruncate(0));
    fortio.close();
}
